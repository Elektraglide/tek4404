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

#define STATIC_LOOKUPxx

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
	int i, len, rc, boot, pmem;
	PH header;
	char bootfile[256];
	char *needle;
	char *symbols;
	int symbolsize;
	unsigned char buffer[256];
	int  tsktab,tskend;
	struct task atask;
	struct userbl userbl;
	struct passwd *pentry;
	int lastuid;
	time_t bootstamp,timestamp,beginstamp,nowstamp;
	struct tm *ts;
	int cpu_pers,dsk_pers,tty_pers,pip_pers;

  char mode[64];
  char *status;
  char *priority;
  int tname,swapsize;
  unsigned int personality;
  
  
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
  printf("symbolsize = %d from bootfile:%s\n", symbolsize, bootfile);

/* VM info */
printf("PAGSIZ %d\n", getkconstant(symbols, symbolsize, "PAGSIZ", pmem));
printf("pages_in %d\n", getkint32(symbols, symbolsize, "pages_in", pmem));
printf("pages_out %d\n", getkint32(symbols, symbolsize, "pages_out", pmem));
printf("page_faults %d\n", getkint32(symbols, symbolsize, "page_faults", pmem));
printf("pages_stolen %d\n", getkint32(symbols, symbolsize, "pages_stolen", pmem));
printf("pages_copied %d\n", getkint32(symbols, symbolsize, "pages_copied", pmem));

printf("buffer_lookups %d\n", getkint32(symbols, symbolsize, "buffer_lookups", pmem));
printf("buffer_compares %d\n", getkint32(symbols, symbolsize, "buffer_compares", pmem));
printf("fdn_lookups %d\n", getkint32(symbols, symbolsize, "fdn_lookups", pmem));
printf("fdn_compares %d\n", getkint32(symbols, symbolsize, "fdn_compares", pmem));
printf("fdn_compares %d\n", getkint32(symbols, symbolsize, "fdn_compares", pmem));
printf("segments_copied %d\n", getkint32(symbols, symbolsize, "segments_copied", pmem));
printf("physical_translations %d\n\n", getkint32(symbols, symbolsize, "physical_translations", pmem));


/* OS stats */
printf("system boot time:  %s\n", kernboottime(symbols, symbolsize, pmem));
printf("system_calls %d\n", getkint32(symbols, symbolsize, "system_calls", pmem));
printf("usedmem %dk\n", getkint32(symbols, symbolsize, "usedmem", pmem) / 1024);
printf("memsize %dk\n", getkint32(symbols, symbolsize, "memsize", pmem) / 1024);
printf("disk ops %d\n", getkint32(symbols, symbolsize, "stadsk", pmem));
printf("block ops%d\n", getkint32(symbols, symbolsize, "stablk", pmem));
printf("block freed %d\n", getkint32(symbols, symbolsize, "stafre", pmem));

printf("staswp %8.8x\n", getkint32(symbols, symbolsize, "staswp", pmem));
printf("uiocnt %d\n", getkint16(symbols, symbolsize, "uiocnt", pmem));
printf("corcnt %d\n", getkint16(symbols, symbolsize, "corcnt", pmem));
printf("lbolt %d\n\n", getkint16(symbols, symbolsize, "lbolt", pmem) / 256);
bootstamp = getkint32(symbols, symbolsize, "sbttim", pmem);


/* scheduler info */
printf("sizeof(atask) = %d  TSKSIZ=%d\n", (int)sizeof(atask), getkconstant(symbols, symbolsize, "TSKSIZ", pmem));
printf("CLOCKS_PER_SECOND %d\n", getkconstant(symbols, symbolsize, "CLOCKS_PER_SECOND", pmem));
printf("swap device %d\n", getkint16(symbols, symbolsize, "swapdv", pmem));
printf("slplst %8.8x\n", getkint32(symbols, symbolsize, "slplst", pmem));
printf("runlst %8.8x\n", getkint32(symbols, symbolsize, "runlst", pmem));
printf("usrtop %8.8x\n\n", getkint32(symbols, symbolsize, "usrtop", pmem));

/* these are task scheduling profiles depending on current work */
len = getkconstant(symbols, symbolsize, "PERSONALITY_SIZE", pmem);
cpu_pers = getkconstant(symbols, symbolsize, "CPU_personality", pmem);
dsk_pers = getkconstant(symbols, symbolsize, "DISK_personality", pmem);
tty_pers = getkconstant(symbols, symbolsize, "TTY_personality", pmem);
pip_pers = getkconstant(symbols, symbolsize, "PIPE_personality", pmem);

  nowstamp = time(NULL);

#ifdef STATIC_LOOKUP
	tsktab = 0x0001EEF0;
	tskend = 0x00020270;
#else
	/* get to task table */
	tsktab = getkint32(symbols, symbolsize, "tsktab", pmem);
	tskend = getkint32(symbols, symbolsize, "tskend", pmem);
	printf("tsktab 0x%8.8x 0x%8.8x\n", tsktab, tskend);
#endif

	rc = lseek(pmem, tsktab, SEEK_SET);
	if (rc < 0)
		fprintf(stderr, "failed to seek\n");
 
	lastuid = -1;
	while(tsktab < tskend)
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

		
		sprintf(numbers, "pri%-3d", atask.tsprir);
		priority = numbers;
		/* catch unknown magic values */
		if (atask.tsprir == 251) priority = "pipe";
		if (atask.tsprir == 246) priority = "in";
		if (atask.tsprir == 216) priority = "wait";
		if (atask.tsprir == 176) priority = "slp";
		if (atask.tsprir == 158) priority = "trce";
		if (atask.tsprir == 120) priority = "sys";
		if (atask.tsstat == TTERM) priority = "---";

		/* build mode codes */
		mode[0] = 0;
		if (atask.tsmode & TCORE) strcat(mode, "CORE ");
		if (atask.tsmode & TLOCK) strcat(mode, "LOCK ");
		if (atask.tsmode & TSYSTM) strcat(mode, "SYSTM ");
		if (atask.tsmode & TTRACP) strcat(mode, "TRACE ");
		if (atask.tsmode & TSWAPO) strcat(mode, "SWAP ");
		if (atask.tsmode & TARGX) strcat(mode, "ARGX ");

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
        printf("swap size = %d (%d)\n", swapsize * 1024, ntohs(atask.tssize));

        /* read swap image */
        printf("swap offset = %d %d %d %d\n",
        atask.tsswap[0],atask.tsswap[1],atask.tsswap[2],atask.tsswap[3]);

		/* running stats */
		beginstamp = ntohl(userbl.ustart);
		if (beginstamp == 0)
  		  beginstamp = bootstamp;

		timestamp = nowstamp - beginstamp;		
		printf("%d => task running for: %2.2d:%2.2d:%2.2d\n", timestamp,
  		(timestamp/3600), (timestamp%3600) / 60, (timestamp%3600) % 60); 
/*
		ts = localtime(&beginstamp);
		printf("task start: %2.2d-%2.2d-%4.4d %2.2d:%2.2d\n", ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900,ts->tm_hour, ts->tm_min);
*/
		printf("pid(%d) ppid(%d) %6s %8s %3s %5s %3dK ",ntohs(atask.tstid), ntohs(atask.tstidp), 
		   status, pentry->pw_name, ntohs(atask.tstty) > 1 ? tty : "xxx", priority, swapsize);

		i = (atask.tsact);
/*		printf("%d:%2.2d:%2.2d ", ((i/60/60) % 60), ((i/60) % 60), i % 60); */
/*		printf("%d [%s] ", i,	mode); */

		/* unknowns... */
		printf("cpu%d tsmode%d tsact%d swap%d\n", ntohs(atask.tscpu), atask.tsmode2, (atask.tsact), ntohl(atask.tsswap) );

		/* if its core.. */
		if (atask.tsmode & TCORE)
		{
			struct mt *arun;
			
			printf("task time user: %d  task time system: %d ", ntohl(userbl.utimu) ,ntohl(userbl.utims));
			printf("total user: %d  total system: %d\n", ntohl(userbl.utimuc) ,ntohl(userbl.utimsc));
			
			printf("effective uid:%d actual uid:%d dperm:%2.2x ", ntohs(userbl.uuid),ntohs(userbl.uuida),userbl.udperm);
			printf("IO count:%d  limits[time:%d io:%d mem:%d] ", ntohl(userbl.uicnt), ntohl(userbl.utimlmt), ntohl(userbl.uiotlmt), ntohl(userbl.umemlmt));

			/* scheduling */
			printf("cpu:%d usys:%d uquantum:%d ", ntohs(userbl.ucpu), ntohs(userbl.usys_ratio), ntohs(userbl.uquantum));
			personality = ntohl(userbl.upersonality);
			status = "??? ";
			if (personality == cpu_pers)  status = "CPU ";
			if (personality == dsk_pers)  status = "DISK";
			if (personality == tty_pers)  status = "TTY ";
			if (personality == pip_pers)  status = "PIPE";
			printf("person:%s\n",status );
			
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

            printf("\n");
		}


		tsktab += sizeof(atask);
	}

	close(pmem);

	return 0;
}
