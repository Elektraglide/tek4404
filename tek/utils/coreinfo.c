#include <stdio.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <time.h>
#include "kernutils.h"
#include "ph.h"
#include "uniflex/task.h"
#include "uniflex/userbl.h"

#ifndef TEK4404
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netinet/ip.h>
#include "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/lib/include/sys/sir.h"
#include "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/lib/include/sys/fdn.h"

#define FDNSIZ 64
#define FMSZ 10      /* fdn direct map size */

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

#else
#include <net/in.h>
#include <sys/sir.h>
#include <sys/fdn.h>
#endif

/* S_IFDIR block table, 32 per block */
struct dirblk {
	unsigned short d_fdn;
	char d_name[14];
};

struct fdn {
    ptr32 cache_next;       // +0x00 struct fdn *
    ptr32 cache_prev;       // +0x04 struct fdn *
    ptr32 hash_next;        // +0x08 struct fdn *
    ptr32 hash_prev;        // +0x0c struct fdn *
    unsigned int    unknown_10;       // +0x10

    unsigned short    device_key;       // +0x14
    unsigned short    inode_key;        // +0x16
    unsigned char     state_flags;      // +0x18
    unsigned char     state_count;      // +0x19, role uncertain
    short     				refcount;         // +0x1a
    unsigned char     buffer_flags;     // +0x1c

    unsigned short    mode;             // +0x1d, packed/unaligned
    unsigned char     link_count;       // +0x1f
    unsigned short    owner_metadata;   // +0x20, meaning uncertain
    unsigned int    	size;             // +0x22
    unsigned short    device_value;     // +0x26, used by device-close paths

    unsigned char     unknown_28_4e[0x27];
    unsigned char     minimum_value_hi; // +0x4f
    unsigned char     minimum_value_lo; // +0x50
    unsigned char     unknown_51_57[7];
    unsigned char     tracked_count;    // +0x58
    unsigned short    tracked_values[50]; // +0x59, packed/unaligned
};

struct filedes {

    unsigned char  mode;       // +0x00: access/mode byte; checked against the operation's permission mask
    unsigned char  flags;      // +0x01: allocation/state bits; allocation code sets bit 7
    unsigned short  refcount;   // +0x02: reference count
    ptr32 fdn;									// +0x04: struct fdn *fdn;  pointer to the associated FDNSIZE record
    int  position;   // +0x08: current file position
};

#ifdef __clang__
#pragma pack(push, 1)
#endif
struct tmat
{
	ushort unknown1;
	ptr32 memqueue;		/* 0x02 */
	ptr32 unknown2;
	ptr32 pooledpages;	/* 0x0a */
	ptr32 privpages;	/* 0x0e */
	ushort tasksize; 	/* 0x12 */
	ushort taskordinal;	/* 0x14 */	
	ptr32 unknown3;		/* 0x16 */
};
#ifdef __clang__
#pragma pack(pop)
#endif

int header[1024];

#ifdef __clang__
#define DISKNAME "/Users/adambillyard/projects/tek4404/mame/bar.chd"
#else
#define DISKNAME "/dev/disk"
#endif
int fd_sir;

int stackoffset(sp,map)
int sp;
struct mt *map;
{
	return sp - ntohl(map[2].vaddr);
}

#define MC_DIRTY 0x80000000
#define MC_WRITE 0x40000000
#define MC_SWAP 0x20000000
#define MC_SHARED 0x10000000
#define MC_PAGE 0x000fffff

void printpages(map_cell_pages, len)
unsigned int *map_cell_pages;
int len;
{
int i;
char c1,c2,c3;

	for(i=0; i<len/4; i++)
	{
		if(map_cell_pages[i])
		{
			unsigned int v = ntohl(map_cell_pages[i]);
			c1 = '.';
			c2 = '.';
			c3 = '.';
			if (v & MC_DIRTY)
				c1 = 'D';
			if (v & MC_WRITE)
				c2 = 'W';
			if (v & MC_SHARED)
				c3 = 'S';

			printf("[%c%c%c]",c1,c2,c3);
		}
	}
	printf("\n");
}

int fd_sir;
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
	
	workingpath[0] = '/';
	workingpath[1] = '\0';
	rdfdn(1, workingnode);
	enum_files(workingnode, workingpath, needle, needlepath);
	return 0;
}

/* parsing core files base on reverse engineering function 0xa814 which dumps a core file for kernel */
int main(argc,argv)
int argc;
char **argv;
{
	int fd;
	int i;

	fd = open(argc > 1 ? argv[1] : "core", O_RDONLY);
	if (fd > 0)
	{
		struct userbl* userbl;
		struct task atask;
		struct mt* map;
		unsigned int* regs;
		struct filedes afile;
		unsigned char buffer[128];
		struct tmat utmat;
		unsigned int map_cell_pages[1024];
		char *status;
		unsigned long timestamp;
		struct dirblk *fdn;
		time_t t;

		/* first 4k */
		read(fd, header, sizeof(header));

		userbl = (struct userbl *)(((char*)header) + ntohl(header[0]));

		i = 1 + ntohs(userbl->usizet) + ntohs(userbl->usized) + ntohs(userbl->usizes);
		lseek(fd, i * 4096, SEEK_SET);
		read(fd, &atask, sizeof(atask));
		printf("PROC:\n");
		t = ntohl(userbl->ustart);
#ifdef __clang__
		// 10 years of seconds; UniFLEX epoch starts 1980 not 1970!
		
#define SECONDS_IN_DAY (60*60*24)
#define NUM_OF_LEAP_DAYS_IN_10_YEARS 2
		t += (int)(SECONDS_IN_DAY * 365 * 10 + SECONDS_IN_DAY * NUM_OF_LEAP_DAYS_IN_10_YEARS);
#endif
		status = ctime(&t);
#ifndef __clang__
        /* UniFlex adds CR! */
		status[24] = '\0';
#endif		
		printf("started: %s\n", status);

		printf("utimu(%d) utims(%d)\n", ntohl(userbl->utimu),ntohl(userbl->utims));

		printf("uid(%d) uuid(%d) pid(%d) ppid(%d)\n", ntohs(atask.tsuid), ntohs(userbl->uuid), ntohs(atask.tstid),ntohs(atask.tstidp));

		status = "";
		if (atask.tsstat == TRUN)  status = "RUN";
		if (atask.tsstat == TSLEEP) status = "SLEEP";
		if (atask.tsstat == TWAIT) status = "WAIT";
		if (atask.tsstat == TTERM) status = "TERM";
		if (atask.tsstat == TTRACE) status = "TRACE";
		printf("tsstat(%d) %s\n", atask.tsstat, status);

		buffer[0] = '\0';
		if (atask.tsmode & TCORE) strcat(buffer, "CORE ");
		if (atask.tsmode & TLOCK) strcat(buffer, "LOCK ");
		if (atask.tsmode & TSYSTM) strcat(buffer, "SYSTM ");
		if (atask.tsmode & TTRACP) strcat(buffer, "TRACE ");
		if (atask.tsmode & TSWAPO) strcat(buffer, "SWAP ");
		if (atask.tsmode & TARGX) strcat(buffer, "ARGX ");
		printf("tsmode(%d) %s\n", atask.tsmode, buffer);

		fdn = (struct dirblk *)userbl->ufdn;
		printf("cwd entry: \"%s\"\n", fdn->d_name);
		printf("bin flags: 0x%4.4x\n", ntohs(userbl->ubin_flags));
		printf("uhltpri value: %d\n", ntohs(userbl->uhltpri));


		/* reading VM page map? */
		printf("PAGE MAP CELLS:\n");
		{
			read(fd, &utmat, 0x1a);
			/* printf("tmat %d for task %d => 0x%X\n", ntohs(utmat.tasksize), ntohs(utmat.taskordinal), 0x80 << (ntohs(utmat.tasksize) & 0x3f)); */
			/* based on task size, writes more or less */
			i = ntohs(utmat.tasksize);
			read(fd, map_cell_pages, 0x80 << (i & 0x3f));
			printpages(map_cell_pages, 0x80 << (i & 0x3f));
			read(fd, map_cell_pages, 0x80 << (i & 0x3f));
		}

		/* reading open files */
		printf("FILES:\n");
		{
			/* we will search inodes for filename */
			fd_sir = open(DISKNAME, O_RDONLY);
			
			for (i = 0; i < UNFILS; i++)
			{
				if (userbl->ufiles[i])
				{
					struct fdn afdn;

					read(fd, &afile, sizeof(afile));
					read(fd, &afdn, 0x5e);	// coredump dumps only start of FDN

					/* disk file */
					if (ntohs(afdn.device_key) == 4)
					{
						char *kind = "unknown";
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
						}
	
						printf("    %d: %s %s, %c%c flags:0x%2.2x seek:%d\n", i,
							kind, filepath, afile.mode & 1 ? 'r':'-', afile.mode & 2 ? 'w':'-', afile.flags, ntohl(afile.position));
					}
					else
					{
						printf("** unknown device\n");
					}
				}
			}
			
			close(fd_sir);
		}


		printf("MAP:\n");
		{
			map = (struct mt*)userbl->umem;
			while (ntohs(map->numpages) > 0)
			{
				printf("    %8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
				map++;
			}
			if (userbl->udep_chunks > 0)
			{
				printf("EXTENDED MAP:\n");
				map = (struct mt*)userbl->umdep_segs;
				while (ntohs(map->numpages) > 0)
				{
					printf("    %8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
					map++;
				}
			}
		}

		/* relies on userbl being on a 4k boundary.. */
		printf("REGISTERS:\n");
		{
			regs = (unsigned int*)((char*)header + (ntohl(userbl->uregs) & 0xfff));
			printf("    Dn ");
			for (i = 0; i < 8; i++)
				printf("%8.8x ", ntohl(regs[i]));
			printf("\n");
			printf("    An ");
			for (i = 0; i < 8; i++)
				printf("%8.8x ", ntohl(regs[8 + i]));
			printf("\n");
		}

		printf("STACKTRACE:\n");
		{
			int argc, argv, envp;
			int sp;
			unsigned int spoff;
			unsigned char* stack;

			i = 1 + ntohs(userbl->usizet) + ntohs(userbl->usized);
			lseek(fd, i * 4096, SEEK_SET);
			stack = (char*)malloc(4096 * ntohs(userbl->usizes));
			read(fd, stack, 4096 * ntohs(userbl->usizes));

			sp = ntohl(regs[15]);
			map = (struct mt*)userbl->umem;
			i = 0;
			while (sp)
			{
				/* TODO: lookup addr from cmd executable symbol table */

				/* SP, Return Addr, locals */
				spoff = stackoffset(sp, map);
				printf("    %d: 0x%8.8x\n", i++, ntohl(*(int*)(stack + spoff + 4)));
				sp = ntohl(*(int*)(stack + spoff));
			}

			spoff += 8;
			argc = ntohl(*(int*)(stack + spoff));
			argv = ntohl(*(int*)(stack + spoff + 4));
			envp = ntohl(*(int*)(stack + spoff + 8));

			/* walk argv array */
			printf("CMDLINE: ");
			for (i = 0; i < argc; i++)
			{
				char* p = ntohl(*(int*)(stack + stackoffset(argv, map)));
				printf("%s ", stack + stackoffset(p, map));

				argv += 4;
			}
			printf("\n");

			printf("ENVIRON:\n");
			while (envp)
			{
				char* p;

				spoff = stackoffset(envp, map);
				p = ntohl(*(int*)(stack + spoff));
				if (!p)
					break;

				printf("    %s\n", stack + stackoffset(p, map));

				envp += 4;
			}
		}

		close(fd);
	}
}

