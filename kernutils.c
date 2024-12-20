/* various reverse engineered functionality accessing the kernel */

#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <pwd.h>

#include "kernutils.h"

#ifndef __clang__
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)
#else
#include <stdlib.h>
#include <unistd.h>

#endif

/*******************************************************/

char *getsymbols(exepath, symbolsize)
char *exepath;
int *symbolsize;
{
	int rc, boot;
	char *symbols;
	PH header;

	boot = open(exepath, O_RDWR);
	if (boot < 0)
	{
		fprintf(stderr, "failed to open %s\n", exepath);
		return 0;
	}
	
	read(boot, &header, sizeof(header));

	symbols = (char *)malloc(ntohl(header.symbolsize));
	lseek(boot, sizeof(PH) + ntohl(header.textsize) + ntohl(header.datasize) + ntohl(header.relocsize), 0);
	rc = read(boot, symbols, ntohl(header.symbolsize));
	close(boot);

	if (rc != ntohl(header.symbolsize))
	{
		return 0;
	}

	*symbolsize = ntohl(header.symbolsize);
	return symbols;
}

symbolheader * listsymbol(symbols, symbolsize)
char *symbols;
int symbolsize;
{
	char *sdata;
	char *send;
	char *sname;
	
	sdata = symbols;
	send = sdata + symbolsize;
	while(sdata < send)
	{
		symbolheader sym;

		/* need to copy so we get on a word aligned boundary for m68k */
		memcpy(&sym, sdata, sizeof(symbolheader));

		/* SEGABS are kernel settings */
		/* SEGDATA are kernel runtimes */
		if(ntohs(sym.segment) == SEGABS || ntohs(sym.segment) == SEGDATA)
		{
			sname = sdata + sizeof(symbolheader);

			printf("%s 0x%8.8x  %.*s\n", (ntohs(sym.segment)==SEGABS) ? "ABS" : "DAT", ntohl(sym.offset), ntohs(sym.len),sname);
		}

		sdata += sizeof(symbolheader) + ntohs(sym.len);
		/* broken len fields... */
		if (sym.len > 9)
		{
			while (*sdata)
				sdata++;
		}
	}
	
	return NULL;
}

symbolheader * findsymbol(symbols, symbolsize, needle)
char *symbols;
int symbolsize;
char *needle;
{
	char *sdata;
	char *send;
	char *sname;
	
	sdata = symbols;
	send = sdata + symbolsize;
	while(sdata < send)
	{
		symbolheader sym;

		/* need to copy so we get on a word aligned boundary for m68k */
		memcpy(&sym, sdata, sizeof(symbolheader));

		/* SEGABS are kernel settings */
		/* SEGDATA are kernel runtimes */
		if(ntohs(sym.segment) == SEGABS || ntohs(sym.segment) == SEGDATA)
		{
			sname = sdata + sizeof(symbolheader);

			if ((*sname == *needle) && !strncmp(sname, needle, ntohs(sym.len)))
			{
				return (symbolheader *)sdata;
			}
		}

		sdata += sizeof(symbolheader) + ntohs(sym.len);
		/* broken len fields... */
		if (sym.len > 9)
		{
			while (*sdata)
				sdata++;
		}
	}
	
	return NULL;
}

int getkconstant(symbols, symbolsize, needle, pmem)
char *symbols;
int symbolsize;
char *needle;
int pmem;
{
	symbolheader *unalignedsym;
	symbolheader sym;
	int setting = 0;
	
	unalignedsym = findsymbol(symbols, symbolsize, needle);
	if (unalignedsym)
	{
		memcpy(&sym, unalignedsym, sizeof(sym));
	
		setting = sym.offset;
	}

	return ntohl(setting);
}

int getkvalue(symbols, symbolsize, needle, pmem)
char *symbols;
int symbolsize;
char *needle;
int pmem;
{
	int offset,setting;

	offset = getkconstant(symbols, symbolsize, needle, pmem);
	lseek(pmem, offset, 0);
	read(pmem, &setting, 4);		/* always 4 bytes? */

	return ntohl(setting);

}

int readkint32(addr, pmem)
int addr;
int pmem;
{
	int setting = 0;
	
	if (addr)
	{
		lseek(pmem, addr, 0);
		read(pmem, &setting, 4);		/* always 4 bytes? */
	}

	return ntohl(setting);
}


