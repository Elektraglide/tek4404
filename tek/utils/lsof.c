#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include "ph.h"
#include "kernutils.h"

/* recovered headers */
#include "uniflex/task.h"
#include "uniflex/userbl.h"

#ifdef TEK4404
#include <sys/modes.h>
#include <sys/sgtty.h>
#include <sys/sir.h>
#include <sys/fdn.h>
#include <net/netdev.h>
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#else

// cc -std=c89 top.c ../kernutils.c -I../uniflex -I.. -o top


#define TRUN   '\1'   /*  running  */
#define TSLEEP '\2'   /*  sleeping (high priority)  */
#define TWAIT  '\3'   /*  waiting (low priority)  */
#define TCREAT '\4'   /*  creating new task  */
#define TTERM  '\5'   /*  termination process  */
#define TTRACE '\6'   /*  trace mode  */

#define TCORE  0x01   /*  task is in core  */
#define TLOCK  0x02   /*  task is locked in core  */
#define TSYSTM 0x04   /*  task is system scheduler  */
#define TTRACP 0x08   /*  task is being traced  */
#define TSWAPO 0x10   /*  task is being swapped  */
#define TARGX  0x20   /*  task is in argument expansion  */

#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl_compat.h>
#include <sgtty.h>
#include "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/lib/include/sys/sir.h"
#include "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/lib/include/sys/fdn.h"
#define S_IFMT     0xCF      /* mask for type of file */
#define S_IFREG    0x01      /* normal file */
#define S_IFBLK    0x03      /* block special */
#define S_IFCHR    0x05      /* character special */
#define S_IFPTY    0x07      /* pseudo-terminal */
#define S_IFDIR    0x09      /* directory */
#define S_IFPIPE   0x41      /* pipe */
#define S_IFCON    0x81      /* contiguous file */
#define S_SLAVE_PTY  0x07    /* Slave pseudo-terminal device */
#define S_MASTER_PTY 0x87    /* Master pseudo-terminal device */

extern int open(const char *filename, int mode, ...);

void _l2tos(unsigned char *blocks,char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = sindblk[1];
		*blocks++ = sindblk[0];
		sindblk += 2;
	}
}

void ltol3(unsigned char *dst, long *blocks, int count)
{
	while(count--)
	{
		// LE to BE
		*dst++ = (*blocks>>16);
		*dst++ = (*blocks>>8);
		*dst++ = (*blocks>>0);
		blocks++;
	}
}

void l3tol(long *blocks,unsigned char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = ((unsigned)sindblk[0]<<16) | ((unsigned)sindblk[1]<<8) | ((unsigned)sindblk[2]<<0);
		sindblk += 3;
	}
}

void _l4tol(long *blocks,unsigned char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = ((unsigned)sindblk[0]<<24) | ((unsigned)sindblk[1]<<16) | ((unsigned)sindblk[2]<<8) | ((unsigned)sindblk[3]<<0);
		sindblk += 4;
	}
}
#endif

#ifdef TEK4404
#define DISKNAME "/dev/disk"
#else
#define DISKNAME "/Users/adambillyard/projects/tek4404/development/uniflexsnap/bar.chd"
#endif
int fd_sir;

#define FDNSIZ 64
#define FMSZ 10      /* fdn direct map size */

/* S_IFDIR block table, 32 per block */
struct dirblk {
	unsigned short d_fdn;
	char d_name[14];
};

struct fdn {
    ptr32 cache_next;       /* +0x00 struct fdn */
    ptr32 cache_prev;       /* +0x04 struct fdn */
    ptr32 hash_next;        /* +0x08 struct fdn */
    ptr32 hash_prev;        /* +0x0c struct fdn */
    unsigned int    unknown_10;       /* +0x10 */

    unsigned short    device_key;       /* +0x14 */
    unsigned short    inode_key;        /* +0x16 */
    unsigned char     state_flags;      /* +0x18 */
    unsigned char     state_count;      /* +0x19, role uncertain */
    short     				refcount;         /* +0x1a */
    unsigned char     buffer_flags;     /* +0x1c */

    unsigned short    mode;             /* +0x1d, packed/unaligned */
    unsigned char     link_count;       /* +0x1f */
    unsigned short    owner_metadata;   /* +0x20, meaning uncertain */
    unsigned int    	size;             /* +0x22 */
    unsigned short    device_value;     /* +0x26, used by device-close paths */

    unsigned char     unknown_28_4e[0x27];
    unsigned char     minimum_value_hi; /* +0x4f */
    unsigned char     minimum_value_lo; /* +0x50 */
    unsigned char     unknown_51_57[7];
    unsigned char     tracked_count;    /* +0x58 */
    unsigned short    tracked_values[50]; /* +0x59, packed/unaligned */
};

struct filedes {

    unsigned char  mode;        /* +0x00: access/mode byte; checked against the operation's permission mask */
    unsigned char  flags;       /* +0x01: allocation/state bits; allocation code sets bit 7 */
    unsigned short  refcount;   /* +0x02: reference count */
    ptr32 fdn;                  /* +0x04: struct fdn *fdn;  pointer to the associated FDNSIZE record */
    int  position;              /* +0x08: current file position */
};


char tty[] = "tty00";
char numbers[16] = "000";
struct sgttyb orig_term_settings;

void printbuffer(buffer, len)
unsigned char *buffer;
int len;
{
	int i,j;
	
	for(i=0; i<len; i+=16)
	{
		for(j=0; j<16; j++)
		{
			printf("%2.2x ", buffer[j]);			
		} 
		for(j=0; j<16; j++)
		{
			if (buffer[j] >= ' ' && buffer[j] <= 'z')
				printf("%c", buffer[j]);
			else
			    printf(".");
		}
		buffer += 16;

		printf("\n");
	}
	printf("\n");
}

#define NODECACHESIZE 64
typedef struct {
  int inode;
  struct inode filenode;
  char path[32];
} nodecache_t;

int lastcache = 0;
nodecache_t nodecache[NODECACHESIZE];

int addcache(fdnptr, needle, needlepath)
struct inode *fdnptr;
int needle;
char *needlepath;
{
  nodecache[lastcache].inode = needle;
  nodecache[lastcache].filenode = *fdnptr;
  strcpy(nodecache[lastcache].path, needlepath);
  lastcache++;
  lastcache &= (NODECACHESIZE-1);
  return lastcache;
}

#define offsetof(st, m) ((int)&(((st *)0)->m))

void rdfdn(fdn,buffer)
int fdn;
struct inode *buffer;
{
	long offset;

	offset = ((((fdn-1L)>>3L)+2L)<<9L) + (((fdn-1)&0x7)<<6L);
	lseek(fd_sir,offset,0);
	read(fd_sir,buffer,FDNSIZ);
}

int enum_files(fdnptr, fullpath, needle, needlepath)
struct inode *fdnptr;
char *fullpath;
int needle;
char *needlepath;
{
	long blocks[FMSZ];
	int i,j;
	char subpath[128];
	
	/* just read direct blocks */
	l3tol(blocks,(unsigned char *)fdnptr->fd_blk,FMSZ);
	for(j=0;j<FMSZ;j++)
	{
		if (blocks[j])
		{
			struct dirblk dentries[32];
			int originallen;
			
			originallen = strlen(fullpath);
			
			/* read 32 dir entries */
			lseek(fd_sir, blocks[j]<<9, 0);
			read(fd_sir, dentries, 512);
			for (i=0; i<32; i++)
			{
				if (dentries[i].d_fdn == 0)
					continue;

				if (dentries[i].d_name[0] == '.' && dentries[i].d_name[1] == '\0')
					continue;
					
				if (dentries[i].d_name[0] == '.' && dentries[i].d_name[1] == '.')
					continue;
					
				/* keep it in host endian */
				dentries[i].d_fdn = ntohs(dentries[i].d_fdn);

				/* get entry fdn (before potential double incremented for long filenames) */
				rdfdn(dentries[i].d_fdn, fdnptr);

#if 0
				/* ignore linked files */
				if (currfdn.fd_cnt > 3)
					continue;
#endif

				/* long filename handling */
				sprintf(subpath, "%c%.13s", dentries[i].d_name[0] & 0x7f, dentries[i].d_name+1);
				if (dentries[i].d_name[0] & 0x80)
				{
					char *append = subpath;
					
					do
					{
						append += sizeof(dentries[0].d_name);
						i++;
						sprintf(append, "%c%.13s", dentries[i].d_name[0] & 0x7f,dentries[i].d_name+1);
					} while ((dentries[i].d_name[0] & 0x80) && dentries[i].d_fdn == 0);
				}

				if(dentries[i].d_fdn == needle)
				{
					sprintf(needlepath, "%s%s", fullpath, subpath);
          addcache(fdnptr, needle, needlepath);
					return 1;
				}

				if ((fdnptr->fd_mod & S_IFMT) == S_IFDIR)
				{
					/* recurse on subdir */
					strcat(fullpath, subpath);
					strcat(fullpath, "/");
					if (enum_files(fdnptr, fullpath, needle, needlepath))
						return 1;

					/* truncate back to original length and continue */
					fullpath[originallen] = '\0';
				}
			}
		}
	}
	
	return 0;
}

int getfilename(needle, needlepath, workingnode)
int needle;
char *needlepath;
struct inode *workingnode;
{
	char workingpath[256];
	struct stat info;
  int i;
  
  /* check our small cache in last-in order */
  for(i=0; i<NODECACHESIZE; i++)
  {
    int j = (lastcache - i) & (NODECACHESIZE-1);
    if (needle == nodecache[j].inode)
    {
      *workingnode = nodecache[j].filenode;
      strcpy(needlepath, nodecache[j].path);
      return 0;
    }
  }

	/* this is brute force search, so check a couple of likely places before searching *everything* */
#ifdef TEK4404
	strcpy(workingpath, "/dev/");
	stat(workingpath, &info);
	rdfdn(info.st_ino, workingnode);
	if (enum_files(workingnode, workingpath, needle, needlepath))
		return 0;

	strcpy(workingpath, "/etc/");
	stat(workingpath, &info);
	rdfdn(info.st_ino, workingnode);
	if (enum_files(workingnode, workingpath, needle, needlepath))
		return 0;
#endif

	strcpy(workingpath, "/");
	rdfdn(1, workingnode);
	enum_files(workingnode, workingpath, needle, needlepath);

	return 0;
}


unsigned int getSP(pmem, atask, userbl)
int pmem;
struct task *atask;
struct userbl *userbl;
{
	int i,regs[16];

	/* get userblock page */
	i = ntohl(atask->tsutop) & 0xfffff000;
	/* add in uregs offset into that page */
	i |= (ntohl((int)userbl->uregs) & 0xfff);
	
	/* get task register A7 (Stack Pointer) */
	lseek(pmem, i + 15*4, 0);
	read(pmem, regs+15, 4);
	return ntohl(regs[15]);
}

int lastsegment = -1;
int lastpageindex = -1;
unsigned int lastpa;


unsigned int va2pa(arun, pmem, va)
struct mt *arun;
int pmem;
unsigned int va;
{
unsigned int pa;
int i;

	for(i=2; i>=0; i--)
	{
		unsigned int pageindex,vmoff;

		vmoff = va - ntohl(arun[i].vaddr);
		pageindex = vmoff >> 12;
		if (pageindex >= 0 && pageindex < ntohs(arun[2].numpages))
		{
			/* do we need to fetch paddr indirection? */
			if (i != lastsegment || pageindex != lastpageindex)
			{
				lastpageindex = pageindex;
				lastsegment = i;
			
				lseek(pmem, ntohl(arun[i].paddr) + 4 * lastpageindex, 0);
				read(pmem, &lastpa, 4);
				
				/* remove permissions bits and make page address */
				lastpa = (ntohl(lastpa) & 0xfffff) << 12;
			}

			return lastpa + (vmoff & 0xfff);
		}
	}
	return 0;
}

unsigned int readvaddr(arun, pmem, va)
struct mt *arun;
int pmem;
unsigned int va;
{
		unsigned int pa,data;

		/* get the paddr and read 4 bytes */
		pa = va2pa(arun, pmem, va);
		if (lseek(pmem, pa, 0) < 0)
		{
			fprintf(stderr,"\nlseek: failed 0x%6.6x\n",pa);
		}
		data = 0;
		if (read(pmem, &data, 4) != 4)
		{
			fprintf(stderr,"\nread: failed 0x%6.6x\n",pa);
		}

		return ntohl(data);
}

/* tweaking OS settings */
int main(argc, argv)
int argc;
char **argv;
{
	int i, j, len, rc, pmem;
	char bootfile[256];
	char *needle;
	char *symbols;
	int symbolsize;
	unsigned char buffer[256];
	int  tsktab,tskend,tskcount;
	struct task atask;
	struct userbl userbl;
	struct passwd *pentry;
	int lastuid;
	time_t bootstamp,timestamp,beginstamp,nowstamp;
	int cpu_pers,dsk_pers,tty_pers,pip_pers;
	int system_calls,usedmem,memsize,mempages,corcnt,lbolt,stadsk,stablk;

  char mode[64];
  char *status;
  char *priority;
  int tname,swapsize;
  unsigned int personality;
  
  short devnum;
	int pcount;
	int prunning;
	int putime;
	int pstime;
 	int openfd;
	int framenum = 0;
	short displaymode = 0;
  char cmdline[64];
  
#ifdef __clang__
	strcpy(buffer, "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/system.boot");
#else
	kernbootfile(buffer);

#endif

	sprintf(bootfile, "/%s", buffer);
	buffer[0] = 0;

  /* so we can show filenames for open files */
  fd_sir = open(DISKNAME, O_RDONLY);

#ifdef __clang__
	pmem = open("/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/tek/dump_pmem/allmem.bin", O_RDWR);
#else
	pmem = open("/dev/pmem", O_RDWR);
#endif
	if (pmem < 0)
	{
		fprintf(stderr, "%s: failed to open /dev/pmem\n", argv[0]);
		exit(1);
	}

	symbols = getsymbols(bootfile, &symbolsize);
	bootstamp = getkint32(symbols, symbolsize, "sbttim", pmem);

	/* boost our priority */
	if (argc > 1 && !strcmp(argv[1], "high"))
		nice(-16);

	/* get to task table */
	tsktab = getkint32(symbols, symbolsize, "tsktab", pmem);
	tskend = getkint32(symbols, symbolsize, "tskend", pmem);

	/* gather stats over all processes */
	lastuid = -1;
	pcount = 0;
	openfd = 0;
	
	printf("COMMAND.....PID......USER........ TYPE.......SIZE......NAME\n");

	rc = lseek(pmem, tsktab, SEEK_SET);
	if (rc < 0)
		fprintf(stderr, "failed to seek\n");

	/* skip pid0 as it is not a Uniflex task */
	rc = read(pmem, &atask, sizeof(atask));

	tskcount = (tskend - tsktab + 1) / sizeof(atask);
	while(tskcount--)
	{
		rc = read(pmem, &atask, sizeof(atask));
		if (rc != sizeof(atask))
			fprintf(stderr, "failed to read\n");

		/* status name */
		status = NULL;
		if (atask.tsstat == TRUN)  status = "run";
		if (atask.tsstat == TSLEEP) status = "sleep";
		if (atask.tsstat == TWAIT) status = "wait";
		if (atask.tsstat == TTERM) status = "term";
		if (atask.tsstat == TTRACE) status = "trace";
		
		/* is it alive? */
		if (status != NULL)
		{

		/* worth skipping if unchanged? */
		if (lastuid != ntohs(atask.tsuid))
		{
			lastuid = ntohs(atask.tsuid);
			pentry = getpwuid(lastuid);
		}

		/* controlling tty */
		if (atask.tstty)
		{
			rc = lseek(pmem, 0, SEEK_CUR);
			/* struct is TTYSIZ (0x2a) */
      i = offsetof(struct kerntty, t_dev);
			lseek(pmem, ntohl(atask.tstty) + i, 0);
			read(pmem, &devnum, 2);
			lseek(pmem, rc, SEEK_SET);
			tty[0] = (devnum & 0x100) ? 'p' : 't';  /* PTYs begin at 0x100 */
      devnum &= 0xff;
			tty[3] = '0' + (devnum / 10);
			tty[4] = '0' + (devnum % 10);
		}

		/* read userblock */
		rc = lseek(pmem, 0, SEEK_CUR);
		i = offsetof(struct userbl, utimu);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.utimu, 8);						/* NB reads utimu,utims */
		i = offsetof(struct userbl, ufiles);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.ufiles, sizeof(userbl.ufiles));
		i = offsetof(struct userbl, usizet);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.usizet, 6);				/* NB reads usizet,usized,usizes */
		i = offsetof(struct userbl, ustart);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.ustart, 4);
		i = offsetof(struct userbl, uregs);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.uregs, 4);					/* NB for getting A7 (stack pointer) */
		i = offsetof(struct userbl, uquantum);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.uquantum, 10);			/* NB reading uquantum,ucpu,usys_ratio,upersonality*/

		i = offsetof(struct userbl, umem) + 2 * MTSIZE;
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, userbl.umem + 2 * MTSIZE, 10);	/* NB just reading stack descriptor (not text & data) */

#ifndef TEK4404
		lseek(pmem, ntohl(atask.tsutop), 0);
		read(pmem, &userbl, sizeof(userbl));
#endif

		lseek(pmem, rc, SEEK_SET);

		/* not sure how to identify a non-task */
		if (atask.tstid != 0 && swapsize == 0)
		{
			continue;
		}

			/* if its core.. */
			if (atask.tsmode & TCORE)
			{
        /* find cmdline */
        if (ntohs(atask.tstid) == 1)
        {
          /* pid1 is launched differently so fake its argvee */
          /* fwiw, it shows as being called tty_wait */
          strcpy(cmdline, "/etc/init");
        }
        else
        if (userbl.usizes)
        {
          struct mt *arun;
          int remain;

          unsigned int sp;
          unsigned int mainargc;
          unsigned int mainargv;
          unsigned int mainenvp;

          int args[3];
          unsigned int array[16];

          /* get last known SP */
          sp = getSP(pmem, &atask, &userbl);

          /* we need to read virtual addresses, so get segment table */
          arun = (struct mt *)(userbl.umem);

          /* invalidate cached info */
          lastsegment = -1;
          
          /* walk up stack until transfer address (assuming 0x0) */
          while(sp > ntohl(arun[2].vaddr))
          {
            sp = readvaddr(arun, pmem, sp);
          }

          /* if we failed to get to TOS, skip cmdline parsing */
          if (!sp)
          {
            unsigned int datapage;
            int datapageindex = -1;
            char cmdarg[32];
            
            /* skip */
            lseek(pmem, 4, SEEK_CUR);
    
            /* read argc, argv and envp */
            /* NB assuming same page for argv,envp as last SP */
            mainargc = 0;
            read(pmem, args, sizeof(args));
            mainargc = ntohl(args[0]);
            mainargv = ntohl(args[1]);
            mainenvp = ntohl(args[2]);
            
            remain = 22;
            cmdline[0] = '\0';
            for (i=0; i<mainargc; i++)
            {
              unsigned int stringvaddr;
              unsigned int stringpaddr;
              
              stringvaddr = readvaddr(arun, pmem, mainargv + i * 4);
              if (!stringvaddr)
                break;
                
              stringpaddr = va2pa(arun, pmem, stringvaddr);
              lseek(pmem, stringpaddr, 0);
              read(pmem, cmdarg, remain);
              cmdarg[remain] = 0;
              strcat(cmdline, cmdarg);
              strcat(cmdline, " ");
              remain -= strlen(cmdarg);
              if (remain <= 0)
                break;
            }
          }
          
          lseek(pmem, rc, SEEK_SET);
        }
			}

			pcount++;
			for(i=0;i<UNFILS; i++)
      {
				if (userbl.ufiles[i])
        {
          struct filedes afile;
					struct fdn afdn;
          char *kind;
          
          rc = lseek(pmem, 0, SEEK_CUR);
          lseek(pmem, ntohl(userbl.ufiles[i]), 0);
          read(pmem, &afile, sizeof(afile));
          lseek(pmem, ntohl(afile.fdn), 0);
          read(pmem, &afdn, sizeof(afdn));
          lseek(pmem, rc, SEEK_SET);

					/* disk file */
					if (ntohs(afdn.device_key) == 4)
					{
						char filepath[256];
						struct inode afilenode;

						getfilename(ntohs(afdn.inode_key), filepath, &afilenode);
						switch(afilenode.fd_mod & S_IFMT)
						{
							case S_IFREG:
									kind = "FILE";
									break;
							case S_IFDIR:
									kind = "DIR ";
									break;
							case S_IFCHR:
									kind = "CDEV";
									break;
							case S_IFBLK:
									kind = "BDEV";
									break;
							case S_IFPTY:
									kind = "PTY ";
									break;
							case S_IFPIPE:
									kind = "PIPE";
									break;
							case S_IFCON:
									kind = "CNTG";
									break;
							default:
									kind = "??? ";
									break;
						}
      
            cmdline[16] = '\0';
            printf("%-16s%8d%10s%6s%10d %s\033[K\n", cmdline, ntohs(atask.tstid), pentry->pw_name, kind, ntohl(afile.position), filepath);
          }

					openfd++;
        }
      }
		}
	}

	close(pmem);

	return 0;
}
