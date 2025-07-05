#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include "ph.h"
#include "kernutils.h"

/* recovered headers */
#include "task.h"
#include "userbl.h"

#ifndef __clang__
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


unsigned char userstack[4096];

/* tweaking OS settings */
int main(argc, argv)
int argc;
char **argv;
{
	int i, len, rc, pmem;
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
  
#ifdef __clang__
	strcpy(buffer, "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/system.boot");
#else
	kernbootfile(buffer);
#endif

	sprintf(bootfile, "/%s", buffer);
	buffer[0] = 0;


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
    
	printf("\033[5;1H\033[>32l");
	printf("\033[1mPID.PPID.STATUS.OWNER....TTY...PRI..SIZE.TIME.....%%CPU.IO...QUANTUM.PERSON\033[0m\n");

	rc = lseek(pmem, tsktab, SEEK_SET);
	if (rc < 0)
		fprintf(stderr, "failed to seek\n");

	tskcount = (tskend - tsktab + 1) / sizeof(atask);
	while(tskcount--)
	{
		rc = read(pmem, &atask, sizeof(atask));
		if (rc != sizeof(atask))
			fprintf(stderr, "failed to read\n");

		/* not sure how to identify a non-task */
		if (atask.tsprir == 0)
			break;

		/* status name */
		if (atask.tsstat == TRUN)  status = "run";
		if (atask.tsstat == TSLEEP) status = "sleep";
		if (atask.tsstat == TWAIT) status = "wait";
		if (atask.tsstat == TTERM) status = "term";
		if (atask.tsstat == TTRACE) status = "trace";

		
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

		/* controlling tty */
		i = (ntohs(atask.tstty) - 2);
		tty[3] = '0' + (i / 10);
		tty[4] = '0' + (i % 10);

		/* read userblock */
		rc = lseek(pmem, 0, SEEK_CUR);
		lseek(pmem, ntohl(atask.tsutop), 0);
		read(pmem, &userbl, sizeof(userbl));
		lseek(pmem, rc, SEEK_SET);

		/* pages are 4k */
		swapsize = 4 * (ntohs(userbl.usizet)+ntohs(userbl.usized)+ntohs(userbl.usizes));
#if 0
		printf("swap size = %d (%d)\n", swapsize * 1024, ntohs(atask.tssize));

		/* read swap image */
		printf("swap offset = %d %d %d %d\n",
		atask.tsswap[0],atask.tsswap[1],atask.tsswap[2],atask.tsswap[3]);
#endif

		/* is it alive? */
		if ((atask.tsstat && swapsize) || (atask.tstid == 0))
		{
			/* running stats */
			printf("%-3d  %-3d %-6s %-8s %-5s %-4s %3dK ",ntohs(atask.tstid), ntohs(atask.tstidp),
				 status, pentry->pw_name, ntohs(atask.tstty) > 1 ? tty : "xxx  ", priority, swapsize);

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
				
				printf("%-4d %-5d ", ((ntohl(userbl.utimu) + ntohl(userbl.utims)) *100)/totalcpu,ntohl(userbl.uicnt) );

				/* printf("IO count:%d  limits[time:%d io:%d mem:%d] ", ntohl(userbl.uicnt), ntohl(userbl.utimlmt), ntohl(userbl.uiotlmt), ntohl(userbl.umemlmt)); */
				/* printf("effective uid:%d actual uid:%d dperm:%2.2x ", ntohs(userbl.uuid),ntohs(userbl.uuida),userbl.udperm); */

				/* scheduling */
				personality = ntohl(userbl.upersonality);
				status = "??? ";
				if (personality == cpu_pers)  status = "CPU ";
				if (personality == dsk_pers)  status = "DISK";
				if (personality == tty_pers)  status = "TTY ";
				if (personality == pip_pers)  status = "PIPE";
				printf("%-7d %s  \n",ntohs(userbl.uquantum), status );
				
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
				
				/* read last page of userstack to find command line */
				rc = lseek(pmem, 0, SEEK_CUR);
				lseek(pmem, ntohl(userbl.ustklim)-4096, SEEK_SET);
				read(pmem, userstack, sizeof(userstack));
				lseek(pmem, rc, SEEK_SET);
				printbuffer(userstack+4096-128, 128);
				
				printf("size=%d slot=%d fdn=%s ", ntohs(userbl.udname_size),ntohs(userbl.udname_slot),userbl.ufdn+2);

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
		}
	}
	
	printf("\033[1K\033[>31l");
	printf("\033[1;1H");
	printf("processes: %d total, %d running utime:%d/%d   ", pcount,prunning, putime, pstime);

	timestamp = nowstamp - bootstamp;
	printf("uptime: %2.2d:%2.2d:%2.2d \n", (timestamp/3600), (timestamp%3600) / 60, (timestamp%3600) % 60);

	printf("system_calls: %d ", readkint32(system_calls, pmem));
	printf("freemem: %dk ", readkint16(corcnt, pmem) * 4);
	printf("physmem: %dk\n", readkint32(memsize, pmem) / 1024);

	printf("mempages: %d ", readkint32(mempages, pmem));
	printf("lbolt %d ", readkint16(lbolt, pmem));
	printf("disk ops:%d ", readkint32(stadsk, pmem));
	printf("block ops:%d\n", readkint32(stablk, pmem));

	sleep(2);
	
	/* used for %CPU */
	totalcpu = putime + pstime;
	
}

	close(pmem);

	return 0;
}
