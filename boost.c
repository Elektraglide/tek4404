#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/modes.h>
#include <sys/sgtty.h>
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

#define offsetof(st, m) ((int)&(((st *)0)->m))

/* set task priority bias to boost */
int boostext(symbols, symbolsize, pmem, tid)
char* symbols;
int symbolsize;
int pmem;
int tid;
{
	int i, rc;
	int  tsktab,tskend,tskcount;
	struct task atask;
  
 	int result;
  
	/* get to task table */
	tsktab = getkint32(symbols, symbolsize, "tsktab", pmem);
	tskend = getkint32(symbols, symbolsize, "tskend", pmem);

	/* walk tasks to find ours */
	rc = lseek(pmem, tsktab, SEEK_SET);
	if (rc < 0)
	{
		fprintf(stderr, "failed to seek\n");
		return -2;
	}

	/* skip pid0 as it is not a Uniflex task */
	rc = read(pmem, &atask, sizeof(atask));

	result = -1;
	tskcount = (tskend - tsktab + 1) / sizeof(atask);
	while (tskcount--)
	{
		rc = lseek(pmem, 0, SEEK_CUR);
		read(pmem, &atask, sizeof(atask));
		if (atask.tstid == tid)
		{
			char bias = -16;

			i = offsetof(struct task, tsprb);
			lseek(pmem, rc + i, SEEK_SET);
			write(pmem, &bias, 1);

			result = 0;
			break;
		}
	}

	return result;
}

/* handle seeking getting kernel symbols too */
int boost(tid)
int tid;
{
char* symbols;
int symbolsize;
int pmem;
char bootfile[256];
unsigned char buffer[256];
int result;

	kernbootfile(buffer);
	sprintf(bootfile, "/%s", buffer);
	buffer[0] = 0;

	pmem = open("/dev/pmem", O_RDWR);
	if (pmem < 0)
	{
		fprintf(stderr, "boost2: failed to open /dev/pmem\n");
		return -2;
	}

	symbols = getsymbols(bootfile, &symbolsize);
	result = boostext(symbols, symbolsize, pmem, tid);
	free(symbols);
	close(pmem);

	return result;
}
