#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include "ph.h"
#include "kernutils.h"

#ifndef __clang__
#include <sys/task.h>
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)
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
	int i;
	
	for(i=0; i<len; i++)
	{
		printf("%2.2x ", *buffer++);
		if ((i & 31)==31)
			printf("\n");
	}
	printf("\n");
}

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
	struct kerntask atask;
	struct passwd *pentry;
	int lastuid;
  time_t timestamp;
  struct tm *ts;
    
  int ucpu,usys_ratio,umapreg,uregs,upersonality;
  int utext_blink,utext_flink,ucname,usarg0,utask,umem;
  int cpu_pers,dsk_pers,tty_pers,pip_pers;

  char mode[64];
  char *status;
  char *priority;
  unsigned int userblk;
  int ublkmap_block_count;
  int tname;
  unsigned int personality;
  
  unsigned int regs,regPC,regSR;

  int txttab,ttytab,numtty,fdntab, hdrtab,exectbl;
  
#ifdef __clang__
	strcpy(buffer, "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/system.boot");
#else
	kernbootfile(buffer);
#endif

	sprintf(bootfile, "/%s", buffer);

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
  printf("symbolsize = %d\n", symbolsize);

#ifdef STATIC_LOOKUP
	tsktab = 0x0001EEF0;
	tskend = 0x00020270;
#else
	/* get to task table */
	tsktab = getkint32(symbols, symbolsize, "tsktab", pmem);
	tskend = getkint32(symbols, symbolsize, "tskend", pmem);
#endif

	/* process #0 skipped */
	tsktab += sizeof(atask);

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
printf("stablk %8.8x\n", getkint32(symbols, symbolsize, "stablk", pmem));
printf("stafre %8.8x\n", getkint32(symbols, symbolsize, "stafre", pmem));
printf("stadsk %8.8x\n", getkint32(symbols, symbolsize, "stadsk", pmem));
printf("staswp %8.8x\n", getkint32(symbols, symbolsize, "staswp", pmem));
printf("uiocnt %d\n", getkint16(symbols, symbolsize, "uiocnt", pmem));
printf("corcnt %d\n\n", getkint16(symbols, symbolsize, "corcnt", pmem));

/* scheduler info */
printf("sizeof(atask) = %d  kerntask=%d\n", (int)sizeof(atask), getkconstant(symbols, symbolsize, "TSKSIZ", pmem));
printf("CLOCKS_PER_SECOND %d\n", getkconstant(symbols, symbolsize, "CLOCKS_PER_SECOND", pmem));
printf("swap device %d\n", getkint16(symbols, symbolsize, "swapdv", pmem));
printf("slplst %8.8x\n", getkint32(symbols, symbolsize, "slplst", pmem));
printf("runlst %8.8x\n", getkint32(symbols, symbolsize, "runlst", pmem));
printf("usrtop %8.8x\n\n", getkint32(symbols, symbolsize, "usrtop", pmem));

	rc = lseek(pmem, tsktab, 0);
	if (rc < 0)
		fprintf(stderr, "failed to seek\n");

/* userblock offsets */
ucpu = getkconstant(symbols, symbolsize, "ucpu", pmem);
usys_ratio = getkconstant(symbols, symbolsize, "usys_ratio", pmem);
umapreg = getkconstant(symbols, symbolsize, "umapreg", pmem);
uregs = getkconstant(symbols, symbolsize, "uregs", pmem);
upersonality = getkconstant(symbols, symbolsize, "upersonality", pmem);

utext_blink = getkconstant(symbols, symbolsize, "utext_blink", pmem);
utext_flink = getkconstant(symbols, symbolsize, "utext_flink", pmem);
ucname = getkconstant(symbols, symbolsize, "ucname", pmem);
usarg0 = getkconstant(symbols, symbolsize, "usarg0", pmem);
utask = getkconstant(symbols, symbolsize, "utask", pmem);
umem = getkconstant(symbols, symbolsize, "umem", pmem);

/* these are task scheduling profiles depending on current work */
cpu_pers = getkconstant(symbols, symbolsize, "CPU_personality", pmem);
dsk_pers = getkconstant(symbols, symbolsize, "DISK_personality", pmem);
tty_pers = getkconstant(symbols, symbolsize, "TTY_personality", pmem);
pip_pers = getkconstant(symbols, symbolsize, "PIPE_personality", pmem);


#if 0
/* reading all userblocks */
ublkmap_block_count = getkint16(symbols, symbolsize, "ublkmap_block_count", pmem);
userblk = getkint32(symbols, symbolsize, "ublkmap", pmem);
while (ublkmap_block_count--)
{
    tname = readkint32(userblk + ucname, pmem);
    readkstring(buffer, 14, tname, pmem);
    printf("userblk = 0x%8.8x  ucname=%s umem=%d  utask=%d\n", userblk, buffer, readkint16(userblk + umem, pmem), readkint16(userblk + utask, pmem));
    
		printf("cpu:%3x usys:%4x ", readkint16(userblk + ucpu, pmem), readkint16(userblk + usys_ratio, pmem));

		len = getkconstant(symbols, symbolsize, "PERSONALITY_SIZE", pmem);
		personality = readkint32(userblk + upersonality, pmem);

		/* personality name ??? */
		status = "??? ";
		if (personality == cpu_pers)  status = "CPU ";
		if (personality == dsk_pers)  status = "DISK";
		if (personality == tty_pers)  status = "TTY ";
		if (personality == pip_pers)  status = "PIPE";
		printf("person:%s  ",status );

		printf("%1x ", readkint16(userblk + umapreg, pmem));

		/* process Registers & SR */
    len = getkconstant(symbols, symbolsize, "REGSIZ", pmem);
    if (len)
    {
      regs = readkint32(userblk + uregs, pmem);
      regPC = readkint32(regs + 0x4e, pmem);
      printf("%6.6x/",regPC );
      regSR = readkint16(regs + 0x4c, pmem);
      printf("%04x ",regSR );
    }

    break;
}
#endif

	printf("Id State Mode  Prio CPU  S/U Event  Sleep  User  PC/Link Syscal  PC/SR\n");
	lastuid = -1;
	while(tsktab < tskend)
	{

		rc = read(pmem, &atask, sizeof(atask));
		if (rc != sizeof(atask))
			fprintf(stderr, "failed to read\n");

		/* process#0 is special, so exclude it */
		if (ntohs(atask.tstid) == 0)
			break;

#if 0
		/* walking linked list of textblks */
		if (atask.tstext)
		{
			printf("%6.6x: ",ntohl(atask.tstext) );
			unsigned int textblk = ntohl(atask.tstext);
			unsigned int firstblk = readkint32(textblk + 4, pmem);
			unsigned int someblk = firstblk;
			do
			{
				printf("%6.6x ",someblk );
				readkstring(buffer, 14, someblk + ucname, pmem);

				someblk = readkint32(someblk + utext_flink, pmem);
			} while (someblk != firstblk);
		}
#endif
  
		/* userblk for this task */
		userblk = ntohl(atask.tsutop);
    tname = readkint32(userblk + ucname, pmem);
    readkstring(buffer, 14, tname, pmem);
    printf("task = 0x%8.8x userblk = 0x%8.8x  ucname=%s umem=%d  utask=%d\n", tsktab, userblk, buffer, readkint16(userblk + umem, pmem),readkint16(userblk + utask, pmem));
    

		/* kernel function has these column titles: */
		/*    Task   Id State Mode  Prio CPU  S/U Event  Sleep  User  PC/Link Syscal  PC/SR */
		printf("tskid:%2d parent(%d) tsstat:%d tsmode:%4x tsprir:%5x ",ntohs(atask.tstid), ntohs(atask.tstidp), atask.tsstat, atask.tsmode, atask.tsprir);

		printf("cpu:%3x usys:%4x ", readkint16(userblk + ucpu, pmem), readkint16(userblk + usys_ratio, pmem));

		len = getkconstant(symbols, symbolsize, "PERSONALITY_SIZE", pmem);
		personality = readkint32(userblk + upersonality, pmem);

		/* personality name ??? */
		status = "??? ";
		if (personality == cpu_pers)  status = "CPU ";
		if (personality == dsk_pers)  status = "DISK";
		if (personality == tty_pers)  status = "TTY ";
		if (personality == pip_pers)  status = "PIPE";
		printf("person:%s  ",status );

		/* kernel prints tsevnt.. */
		printf("tsevnt:%6.6x ",ntohl(atask.tsevnt) );
		
		/* kernel prints tsslnk  (userblk?) */
		printf("tsslnk:%6.6x ",ntohl(atask.tsslnk) );

#ifdef finding_command_name
		unsigned int userblk = ntohl(atask.tsslnk);
		unsigned int ucname = readkint32(userblk + 82, pmem);
		readkstring(status, 14, ucname, pmem);
		printf("%s ", status);
#endif

		/* kernel prints tsutop  (labelled "PC/Link") */
		printf("usrblk:%6.6x ",ntohl(atask.tsutop) );

		/* kernel prints tslink */
		printf("%6.6x ",ntohl(atask.tslink) );

		printf("%1x ", readkint16(userblk + umapreg, pmem));
 

		/* process Registers & SR */
    len = getkconstant(symbols, symbolsize, "REGSIZ", pmem);
    if (len)
    {
      regs = readkint32(userblk + uregs, pmem);
      regPC = readkint32(regs + 0x4e, pmem);
      printf("%6.6x/",regPC );
      regSR = readkint16(regs + 0x4c, pmem);
      printf("%04x ",regSR );
    }

#ifdef output_like_native_status
	
		/* status name */
		if (atask.tsstat == TRUN)  status = "run";
		if (atask.tsstat == TSLEEP) status = "sleep";
		if (atask.tsstat == TWAIT) status = "wait";
		if (atask.tsstat == TTERM) status = "term";
		if (atask.tsstat == TTRACE) status = "trace";

		
		sprintf(numbers, "pri%-3d", atask.tsprir);
		priority = numbers;

#if 0
		/* catch unknown magic values */
		if (atask.tsprir == 251) priority = "pipe";
		if (atask.tsprir == 246) priority = "in";
		if (atask.tsprir == 216) priority = "wait";
		if (atask.tsprir == 176) priority = "slp";
		if (atask.tsprir == 158) priority = "trce";
		if (atask.tsprir == 120) priority = "sys";
		if (atask.tsstat == TTERM) priority = "---";
#endif

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

		printf("%5d %6s %8s %5d %5s %5s %3dK ",ntohs(atask.tstid), status, pentry->pw_name, ntohs(atask.tstidp), ntohs(atask.tstty) > 1 ? tty : "xx", priority, ntohs(atask.tssize));

		i = ntohs(atask.tsact);
		printf("%d:%2.2d:%2.2d ", ((i/60/60) % 60), ((i/60) % 60), i % 60);
/*		printf("%d [%s] ", i,	mode); */

#endif

		/* unknowns... */
		printf("cpu%d tsmode%d tsact%3d \n", ntohs(atask.tscpu), atask.tsmode2, (atask.tsact));

		tsktab += sizeof(atask);
	}


	len = getkconstant(symbols, symbolsize, "PRCSIZ", pmem);
	len = getkconstant(symbols, symbolsize, "TXSSIZ", pmem);
	txttab = getkint32(symbols, symbolsize, "txttab", pmem);
	printf("txttab %8.8x\n",txttab);
	rc = lseek(pmem, txttab, 0);
	while(1)
	{
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		break;
	}

	len = getkconstant(symbols, symbolsize, "TTYSIZ", pmem);
	ttytab = getkint32(symbols, symbolsize, "ttytab", pmem);
	numtty = getkint16(symbols, symbolsize, "numtty", pmem);
	printf("ttytab %8.8x numtty=%d\n",ttytab, numtty);
	rc = lseek(pmem, ttytab, 0);
	while(numtty--)
	{
		struct kerntty *ktty = (struct kerntty *)buffer;
		read(pmem, buffer, len);
		printbuffer(buffer, len);
	}
	
	
	len = getkconstant(symbols, symbolsize, "FDNSIZ", pmem);
	fdntab = getkint32(symbols, symbolsize, "fdntab", pmem);
	printf("fdntab %8.8x\n",fdntab);
	rc = lseek(pmem, fdntab, 0);
	while(1)
	{
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		read(pmem, buffer, len);
		printbuffer(buffer, len);

		break;

	}

	len = getkconstant(symbols, symbolsize, "HDRSIZ", pmem);
  hdrtab = getkint32(symbols, symbolsize, "hdrtab", pmem);
	printf("hdrtab %8.8x\n",hdrtab);
	rc = lseek(pmem, hdrtab, 0);
	while(1)
	{
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		break;
	}



	len = getkconstant(symbols, symbolsize, "EXECSIZE", pmem);
	exectbl = getkint32(symbols, symbolsize, "exectbl", pmem);
	printf("exectbl %8.8x\n",exectbl);
	rc = lseek(pmem, exectbl, 0);
	while(1)
	{
		read(pmem, buffer, len);
		printbuffer(buffer, len);
		read(pmem, buffer, len);
		printbuffer(buffer, len);

		break;
	}

	close(pmem);

	return 0;
}
