#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include "ph.h"
#include "kernutils.h"

#ifndef __clang__
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)
#else
#define TCORE  0x01   /*  task is in core  */
#define TLOCK  0x02   /*  task is locked in core  */
#define TSYSTM 0x04   /*  task is system scheduler  */
#define TTRACP 0x08   /*  task is being traced  */
#define TSWAPO 0x10   /*  task is being swapped  */
#define TARGX  0x20   /*  task is in argument expansion  */

#include <stdlib.h>
#include <unistd.h>
#endif

/* tweaking OS settings */
int main(argc, argv)
int argc;
char **argv;
{
	int i, rc, boot, pmem;
	PH header;
	char bootfile[1024];
	char *needle;
	unsigned int value,setting;
	char *symbols;
	int symbolsize;
	char *sdata;
	char *send;
	char buffer[64];
	symbolheader *unalignedsym;
	int  tsktab,tskend;
	struct kerntask atask;
	
	
#ifdef __clang__
	strcpy(bootfile, "/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/system.boot");
	

#else
	kernbootfile(buffer);
	sprintf(bootfile, "/%s", buffer);
#endif

	/* usage:  kset key [value] */
	needle = argc > 1 ? argv[1] : NULL;
	value = 0;
	if (argc > 2)
	{
		if (sscanf(argv[2], "0x%x", &value) != 1)
			sscanf(argv[2], "%d", &value);
	}

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
	
	unalignedsym = needle ? findsymbol(symbols, symbolsize, needle) : listsymbol(symbols, symbolsize);
	if (unalignedsym)
	{
		symbolheader *sym = (symbolheader *)buffer;
		
		/* need to copy so we get on a word aligned boundary for m68k */
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, unalignedsym, sizeof(symbolheader));
 		memcpy(buffer+sizeof(symbolheader), unalignedsym+1, ntohs(sym->len));

		printf("%s 0x%8.8x  %*s\n", (ntohs(sym->segment)==SEGABS) ? "ABS" : "DAT", ntohl(sym->offset), ntohs(sym->len),(char *)(sym+1));

		if (ntohs(sym->segment) == SEGDATA)
		{
			if (pmem > 0)
			{
				lseek(pmem, ntohl(sym->offset), 0);
				read(pmem, &setting, 4);
				printf("value = 0x%8.8x\n", setting);
			}
		}
		else
		if (argc > 2)
		{
			printf("***** Setting %s:(0x%8.8x) to 0x%8.8x\n", (char *)(sym+1), ntohl(sym->offset), (value));
			printf("Are you certain? (Y/n) ");
			i = getchar();
			if (i == 'Y')
			{
				sym->offset = htonl(value);
				
				boot = open(bootfile, O_RDWR);
				lseek(boot, sizeof(PH) + ntohl(header.textsize) + ntohl(header.datasize) + ntohl(header.relocsize), 0);
				lseek(boot, (char*)unalignedsym - symbols, 1);
#ifdef DO_REAL_WRITE
				write(boot, sym, sizeof(sym));
#endif
				close(boot);
			}
				
		}
	}

	close(pmem);

	return 0;
}
