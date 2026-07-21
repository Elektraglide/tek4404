/* various reverse engineered functionality accessing the kernel */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
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

int getkint32(symbols, symbolsize, needle, pmem)
char *symbols;
int symbolsize;
char *needle;
int pmem;
{
	int orig,offset,setting;

	orig = lseek(pmem, 0, 1);
	offset = getkconstant(symbols, symbolsize, needle, pmem);
	lseek(pmem, offset, 0);
	read(pmem, &setting, 4);

	lseek(pmem, orig, 0);

	return ntohl(setting);
}

int getkint16(symbols, symbolsize, needle, pmem)
char *symbols;
int symbolsize;
char *needle;
int pmem;
{
	int orig,offset;
	short setting;

	orig = lseek(pmem, 0, 1);
	offset = getkconstant(symbols, symbolsize, needle, pmem);
	lseek(pmem, offset, 0);
	read(pmem, &setting, 2);

	lseek(pmem, orig, 0);

	return ntohs(setting);
}

int readkint32(addr, pmem)
int addr;
int pmem;
{
	int orig,setting = 0;
	
	if (addr)
	{
		orig = lseek(pmem, 0, 1);
		lseek(pmem, addr, 0);
		read(pmem, &setting, 4);

		lseek(pmem, orig, 0);
	}

	return ntohl(setting);
}

int readkint16(addr, pmem)
int addr;
int pmem;
{
	int orig;
	short setting = 0;
	
	if (addr)
	{
		orig = lseek(pmem, 0, 1);
		lseek(pmem, addr, 0);
		read(pmem, &setting, 2);

		lseek(pmem, orig, 0);
	}

	return ntohs(setting);
}

char * readkstring(buffer, len, addr, pmem)
char *buffer;
int len;
int addr;
int pmem;
{
	int orig;
	int rc;
	
	if (addr)
	{
		orig = lseek(pmem, 0, 1);
		lseek(pmem, addr, 0);
    buffer[0] = '\0';
		rc = read(pmem, buffer, len);

		lseek(pmem, orig, 0);
	}

	return buffer;
}

char *kernboottime(symbols, symbolsize, pmem)
char *symbols;
int symbolsize;
int pmem;
{
  static char buffer[64];
  time_t timestamp;
  struct tm *ts;
  
  timestamp = getkint32(symbols, symbolsize, "sbttim", pmem);
  ts = localtime(&timestamp);
  sprintf(buffer, "%2.2d-%2.2d-%4.4d %2.2d:%2.2d",
      ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900,
      ts->tm_hour, ts->tm_min);

  return buffer;
}

