#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include "ph.h"
#include "kernutils.h"

/* recovered headers */
#include "uniflex/task.h"
#include "uniflex/userbl.h"

#ifndef __clang__
#include <net/netdev.h>
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#else

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
#endif

char tty[] = "tty00";
char numbers[16] = "000";

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

#define offsetof(st, m) ((int)&(((st *)0)->m))

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
  
	int pcount;
	int prunning;
	int putime;
	int pstime;
  int totalcpu;
  int openfd;
  int framenum = 0;
  
#ifdef __clang__
	strcpy(buffer, "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/system.boot");
#else
	kernbootfile(buffer);
#endif

	sprintf(bootfile, "/%s", buffer);
	buffer[0] = 0;


#ifdef __clang__
	pmem = open("/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/tek/dump_pmem2/allmem.bin", O_RDWR);
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


	/* these are task scheduling profiles depending on current work */
	len = getkconstant(symbols, symbolsize, "PERSONALITY_SIZE", pmem);
	cpu_pers = getkconstant(symbols, symbolsize, "CPU_personality", pmem);
	dsk_pers = getkconstant(symbols, symbolsize, "DISK_personality", pmem);
	tty_pers = getkconstant(symbols, symbolsize, "TTY_personality", pmem);
	pip_pers = getkconstant(symbols, symbolsize, "PIPE_personality", pmem);

	/* stats we will be checking often */
	system_calls = getkconstant(symbols, symbolsize, "system_calls", pmem);
	usedmem = getkconstant(symbols, symbolsize, "usedmem", pmem);
	memsize = getkconstant(symbols, symbolsize, "memsize", pmem);
	mempages = getkconstant(symbols, symbolsize, "mempages", pmem);
	corcnt = getkconstant(symbols, symbolsize, "corcnt", pmem);
	lbolt = getkconstant(symbols, symbolsize, "lbolt", pmem);
	stadsk = getkconstant(symbols, symbolsize, "stadsk", pmem);
	stablk = getkconstant(symbols, symbolsize, "stablk", pmem);

	/* get to task table */
	tsktab = getkint32(symbols, symbolsize, "tsktab", pmem);
	tskend = getkint32(symbols, symbolsize, "tskend", pmem);

printf("\033[2J");
totalcpu = 100000000;
while(1)
{
	nowstamp = time(NULL);
#ifdef __clang__
	/* we want time from kernel image */
	nowstamp = bootstamp + (getkint32(symbols, symbolsize, "stimh", pmem) / 100 );
#endif

	/* gather stats over all processes */
	lastuid = -1;
	pcount = 0;
	prunning = 0;
	putime = 0;
	pstime = 0;
	openfd = 0;
	
	printf("\033[5;1H\033[>32l");
	printf("\033[1mPID.PPID.STATUS.OWNER....TTY...PRI..SIZE.TIME.....%%CPU.\033[K%s\033[0m\n", framenum & 1 ? "QUANTUM.PERSON" : "CMDLINE");

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

		priority = NULL;
		/* catch unknown magic values */
		if (atask.tsprir == 251) priority = "pipe";
		if (atask.tsprir == 246) priority = "in";
		if (atask.tsprir == 216) priority = "wait";
		if (atask.tsprir == 176) priority = "slp";
		if (atask.tsprir == 158) priority = "trce";
		if (atask.tsprir == 120) priority = "sys";
		if (atask.tsstat == TTERM) priority = "---";
		if (priority == NULL)
		{
			sprintf(numbers, "%-3d", atask.tsprir);
			priority = numbers;
		}

#if 0		
		/* build mode codes */
		mode[0] = 0;
		if (atask.tsmode & TCORE) strcat(mode, "CORE ");
		if (atask.tsmode & TLOCK) strcat(mode, "LOCK ");
		if (atask.tsmode & TSYSTM) strcat(mode, "SYSTM ");
		if (atask.tsmode & TTRACP) strcat(mode, "TRACE ");
		if (atask.tsmode & TSWAPO) strcat(mode, "SWAP ");
		if (atask.tsmode & TARGX) strcat(mode, "ARGX ");
#endif

		/* worth skipping if unchanged? */
		if (lastuid != ntohs(atask.tsuid))
		{
			lastuid = ntohs(atask.tsuid);
			pentry = getpwuid(lastuid);
		}

		/* controlling tty (struct not fully known) */
		if (atask.tstty)
		{
			rc = lseek(pmem, 0, SEEK_CUR);
			/* struct is TTYSIZ (0x2a) */
			lseek(pmem, ntohl(atask.tstty) + 0x1c, 0);
			read(pmem, buffer, 4);
			lseek(pmem, rc, SEEK_SET);
			i = buffer[1];
			tty[0] = ((i & 0x6) == 0x06) ? 'p' : 't';
			i = buffer[2];
			tty[3] = '0' + (i / 10);
			tty[4] = '0' + (i % 10);
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
		i = offsetof(struct userbl, uquantum);
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, &userbl.uquantum, 10);

		i = offsetof(struct userbl, umem) + 2 * MTSIZE;
		lseek(pmem, ntohl(atask.tsutop) + i, 0);
		read(pmem, userbl.umem + 2 * MTSIZE, 10);

#ifdef __clang__
		lseek(pmem, ntohl(atask.tsutop), 0);
		read(pmem, &userbl, sizeof(userbl));
#endif

		lseek(pmem, rc, SEEK_SET);

		/* pages are 4k */
		swapsize = 4 * (ntohs(userbl.usizet)+ntohs(userbl.usized)+ntohs(userbl.usizes));
#if 0
		printf("swap size = %d (%d)\n", swapsize * 1024, ntohs(atask.tssize));

		/* read swap image */
		printf("swap offset = %d %d %d %d\n",
		atask.tsswap[0],atask.tsswap[1],atask.tsswap[2],atask.tsswap[3]);
#endif

		/* not sure how to identify a non-task */
		if (atask.tstid != 0 && swapsize == 0)
		{

		}


			/* running stats */
			printf("%-3d  %-3d %-6s %-8s %-5s %-4s %3dK ",ntohs(atask.tstid), ntohs(atask.tstidp),
				 status, pentry->pw_name, (atask.tstty) ? tty : "xxx  ", priority, swapsize);

			/* cpu time */
			timestamp = (ntohl(userbl.utimu) + ntohl(userbl.utims)) / 100;
			printf("%2.2d:%2.2d:%2.2d ", (timestamp/3600), (timestamp%3600) / 60, (timestamp%3600) % 60);
		
			i = (atask.tsact);
	/*		printf("%d:%2.2d:%2.2d ", ((i/60/60) % 60), ((i/60) % 60), i % 60); */
	/*		printf("%d [%s] ", i,	mode); */

			/* unknowns... */
			/* printf("cpu%d tsmode%d tsact%d swap%d\n", ntohs(atask.tscpu), atask.tsmode2, (atask.tsact), ntohl(*(int *)atask.tsswap) ); */

			/* if its core.. */
			if (atask.tsmode & TCORE)
			{
				struct mt *arun;
				unsigned int page,page_addr;
				unsigned int argcee;
				unsigned int argvee[4];
				char cmdline[32];
				int remain;
				
				printf("%-4d ", ((ntohl(userbl.utimu) + ntohl(userbl.utims)) *100)/totalcpu );

				/* printf("IO count:%d  limits[time:%d io:%d mem:%d] ", ntohl(userbl.uicnt), ntohl(userbl.utimlmt), ntohl(userbl.uiotlmt), ntohl(userbl.umemlmt)); */
				/* printf("effective uid:%d actual uid:%d dperm:%2.2x ", ntohs(userbl.uuid),ntohs(userbl.uuida),userbl.udperm); */

				/* alternate  between displaying different task info */
				if (framenum & 1)
				{
					/* scheduling */
					personality = ntohl(userbl.upersonality);
					status = "??? ";
					if (personality == cpu_pers)  status = "CPU ";
					if (personality == dsk_pers)  status = "DISK";
					if (personality == tty_pers)  status = "TTY ";
					if (personality == pip_pers)  status = "PIPE";
					printf("%-7d %s  \033[K\n",ntohs(userbl.uquantum), status );
				}
				else
				{
					/* find cmdline */
					if (ntohs(atask.tstid) == 1)
					{
						/* pid1 is launched differently so fake its argvee */
						/* fwiw, it shows as being called tty_wait */
						printf("/etc/init");						
					}
					else
					if (userbl.usizes)
					{
						/* read user stack page entry for oldest page in chunk */
						arun = (struct mt *)(userbl.umem);
		
						i = ntohl(arun[2].paddr) + (ntohs(arun[2].numpages) - 1) * 4;
						lseek(pmem, i, 0);
						len = read(pmem, &page, 4);
#if 0
    printf("\n len=%d seek = %6.6x\n", len, i);
	printf("vaddr:%6.6x  paddr:%6.6x  count:%d\n", ntohl(arun[0].vaddr), ntohl(arun[0].paddr), ntohs(arun[0].numpages));
	printf("vaddr:%6.6x  paddr:%6.6x  count:%d\n", ntohl(arun[1].vaddr), ntohl(arun[1].paddr), ntohs(arun[1].numpages));
	printf("vaddr:%6.6x  paddr:%6.6x  count:%d\n", ntohl(arun[2].vaddr), ntohl(arun[2].paddr), ntohs(arun[2].numpages));

	printf("%8.8x => %8.8x \n", ntohl(page), ntohl(page) << 12 );
#endif
						/* remove permissions bits */
						page_addr = (ntohl(page) & 0xfffff) << 12;
								
						/* read argc, read argv pointer */
						lseek(pmem, page_addr, 0);
						read(pmem, &argcee, 4);
						if (ntohl(argcee) < 1) argcee = htonl(1);
						read(pmem, argvee, 4 * ntohl(argcee));
			
						remain = 20;
						for (i=0; i<ntohl(argcee); i++)
						{
							lseek(pmem, page_addr + (ntohl(argvee[i]) & 0xfff), 0);
							read(pmem, cmdline, remain);
							cmdline[remain] = 0;
							printf("%s ", cmdline);
							remain -= strlen(cmdline);
							if (remain <= 0)
								break;
						}

						lseek(pmem, rc, SEEK_SET);
					}
					/* delete rest of line */
					printf("\033[K\n");
				}
				
#if 0
				/* memory map */
				printf("tchunk:%d dchunk:%d schunk:%d uerror:%d\n", ntohs(userbl.utchunk), ntohs(userbl.udchunk), ntohs(userbl.uschunk), userbl.uerror);
				printf("tpages:%d dpages:%d spages:%d\n", ntohs(userbl.usizet), ntohs(userbl.usized), ntohs(userbl.usizes));
				if (userbl.usizet)
				{
					arun = (struct mt *)(userbl.umem);
					while (arun->numpages)
					{
						printf("vaddr:%6.6x  paddr:%6.6x  count:%d\n", ntohl(arun->vaddr), ntohl(arun->paddr), ntohs(arun->numpages));
						arun++;
					}
				}
				/* extra pages */
				if (userbl.udep_chunks)
				{
					struct mt *arun = (struct mt *)(userbl.umdep_segs);
					while (arun->numpages)
					{
						printf("udep: vaddr:%6.6x  paddr:%6.6x  count:%d\n", ntohl(arun->paddr), ntohl(arun->vaddr), ntohs(arun->numpages));
						arun++;
					}
				}

				tname = ntohl(userbl.ucname);
				if (tname)
				{
					memset(buffer,0,16);
					readkstring(buffer, 14, tname, pmem);
					printf("ucname=%s umem=%d  utask=%d \n", buffer, ntohs(userbl.umxmem), (int)userbl.utask_ord);
				}
#endif
			}

			/* total stats */
			pcount++;
			if (atask.tsstat == TRUN) prunning++;
			if (userbl.uquantum)
			{
				putime += ntohl(userbl.utimu);
				pstime += ntohl(userbl.utims);
			}
			for(i=0;i<UNFILS; i++)
				if (userbl.ufiles[i])
					openfd++;
		}
	}
    printf("\033[2K");	
	printf("\033[1K\033[>31l");
	printf("\033[1;1H");
	printf("proc: %d total, %d running fd:%d utime:%d/%d   ", pcount,prunning, openfd, putime, pstime);

	timestamp = nowstamp - bootstamp;
	printf("uptime: %2.2d:%2.2d:%2.2d \n", (timestamp/3600), (timestamp%3600) / 60, (timestamp%3600) % 60);

	printf("system_calls: %d ", readkint32(system_calls, pmem));
	printf("freemem: %dk ", readkint16(corcnt, pmem) * 4);
	printf("physmem: %dk\n", readkint32(memsize, pmem) / 1024);
	
	printf("mempages: %d ", readkint32(mempages, pmem));
	printf("lbolt %d ", readkint16(lbolt, pmem));
	printf("disk ops:%d ", readkint32(stadsk, pmem));
	printf("block ops:%d\n", readkint32(stablk, pmem));

#ifndef __clang__
{
unsigned char dbuffer[2048];
int dbufferlen,nde_size,ncount;
netdev *ptr;

  dbufferlen = ldiddle("ndevsw"); 			/* buffer len */
  rdiddle("nde_size", &nde_size, sizeof(nde_size));	/* ndev struct size */
  ncount  = dbufferlen / nde_size;
  ptr = rdiddle("ndevsw", dbuffer, dbufferlen);		/* buffer data */

  ptr = dbuffer;
  while (ncount-- > 0)
  {
  	if (ptr->nd_flags & F_N_ONLINE)
  	{
    	printf("network %s: recv: %d xmit: %d\n", 
		  ptr->nd_name, ptr->nd_stat.sb_rcnt, ptr->nd_stat.sb_xcnt);
	}
    ptr++;
  }
}
#endif

	sleep(5);
	
	/* used for %CPU */
	totalcpu = putime + pstime;
	framenum++;
	
}

	close(pmem);

	return 0;
}
