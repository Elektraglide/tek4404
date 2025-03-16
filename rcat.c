#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <memory.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
  
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define O_APPEND        0x00000008      /* set append mode */
#define O_CREAT         0x00000200      /* create if nonexistant */
#define O_TRUNC         0x00000400      /* truncate to zero length */

#define min(A,B)  (A < B ? A : B)

#ifndef __clang__
#define ntohl(A) (A)
#define ntohs(A) (A)
#define htonl(A) (A)
#define htons(A) (A)
#define off_t int
extern char *basename();
#else

#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#endif

/* MMU page size */
#define TEXTBASE 0x0000
#define PAGESZ 4096

/* Xfer entrypoint */
#define ENTRYPOINT "Start"
#define ENTRYPOINTLEN 5

/* cmd line options */
int verbose = 0;
int stripsymbols = 0;
int justcat = 0;

/* should be the date the concat was created... */
char rcs[64];

int rcslog()
{
  time_t timestamp;
  struct tm *ts;

    timestamp = time(NULL);
    ts = localtime(&timestamp);
    sprintf(rcs, "rcat %2.2d-%2.2d-%4.4d %2.2d:%2.2d",
        ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900,
        ts->tm_hour, ts->tm_min);

  return strlen(rcs);
}


void datacpy(dst, src, len)
int dst;
int src;
int len;
{
	char buffer[4096];
	int n;
	
	while(len > 0)
	{
		n = read(src, buffer, min(len, sizeof(buffer)) );
		write(dst, buffer, n);
		len -= n;
	}
}

typedef struct
{
	char srcfile[64];
	char *content;
	char *relocs;
	char *symbols;
	int textoff,dataoff,bssoff;

	int used;

} compileunit;

#define MAXINPUTS 512
compileunit inputs[MAXINPUTS];


typedef struct
{
	int srcunit;
	unsigned int namehash;
	int segment;
	int finaloffset;
} externalsymbol;

typedef struct
{
	int symbolcount;
	int symbolmax;
	externalsymbol *symbols;
} symbolbucket;

/* 8 buckets */
symbolbucket buckets[8];

void symbolclear()
{
	int i;

	if (buckets[0].symbols)
	{
		for(i=0; i<8; i++)
			buckets[i].symbolcount = 0;
	}
	else
	for(i=0; i<8; i++)
	{
		buckets[i].symbolcount = 0;
		buckets[i].symbolmax = 128;
		buckets[i].symbols = (externalsymbol *)malloc(sizeof(externalsymbol) * buckets[i].symbolmax);
	}
}

int symbolcount()
{
	int total = 0;
	int i;

	for(i=0; i<8; i++)
	{
		total += buckets[i].symbolcount;
	}
	
	return total;
}

#define HASH_INIT (0x811c9dc5)
#define HASH_PRIME (0x01000193)

int symboladd(unit, name, len, segment, finaloffset)
int unit;
char *name;
int len;
int segment;
int finaloffset;
{
	unsigned int hval;
	symbolbucket *abucket;
	register int i;
	register externalsymbol *sptr;
	
	if (len > 0)
	{
		if (verbose==2) fprintf(stderr, "%.*s: symbol from %s\n", len, name, inputs[unit].srcfile);
	
		/* generate a hash for this string */
		/* Larson hash */
    hval = HASH_INIT;
    while(len--)
    {
			/* times 101 */
			hval = (hval<<6)+(hval<<5)+(hval<<2) + hval;
      hval ^= (unsigned int)*name++;
    }

		/* which bucket are we using? */
		abucket = &buckets[hval & 7];
		
		/* do we have it? */
		i = abucket->symbolcount;
		sptr = abucket->symbols;
		while(i--)
		{
			if (sptr->namehash == hval) return 1;
			sptr++;
		}

		/* add it and resize if needed */
		abucket->symbols[abucket->symbolcount].srcunit = unit;
		abucket->symbols[abucket->symbolcount].namehash = hval;
		abucket->symbols[abucket->symbolcount].segment = segment;
		abucket->symbols[abucket->symbolcount].finaloffset = finaloffset;
		
		abucket->symbolcount++;
		if (abucket->symbolcount >= abucket->symbolmax)
		{
			abucket->symbolmax *= 2;
			abucket->symbols = (externalsymbol *)realloc(abucket->symbols, sizeof(externalsymbol) * abucket->symbolmax);
		}
	}
	
	return 0;
}

externalsymbol *symbolfind(name, len)
char *name;
int len;
{
	unsigned int hval;
	symbolbucket *abucket;
	register int i;
	register externalsymbol *sptr;

	if (len > 0)
	{
		/* generate a hash for this string */
		/* Larson hash */
    hval = HASH_INIT;
    while(len--)
    {
			/* times 101 */
			hval = (hval<<6)+(hval<<5)+(hval<<2) + hval;
			hval ^= (unsigned int)*name++;
		}

		/* which bucket are we using? */
		abucket = &buckets[hval & 7];

		i = abucket->symbolcount;
		sptr = abucket->symbols;
		while(i--)
		{
			if (sptr->namehash == hval) return sptr;
			sptr++;
		}
	}
	return 0;
}

/* simple set of integers representing compileunit index */
typedef struct
{
	int members[MAXINPUTS];
	int lastentry;
} set;


int addmember(results,unit)
set *results;
int unit;
{
	int i;
	for(i=0; i<results->lastentry; i++)
	{
		if (results->members[i] == unit)
			return 0;
	}
	results->members[results->lastentry] = unit;
	results->lastentry++;
	return 1;
}

/* all missing symbols */
int addmissing(missing, name, len)
set *missing;
char *name;
int len;
{
	unsigned int hval;
	int i;

		/* generate a hash for this string */
		hval = HASH_INIT;
		while (len--)
		{
			hval *= HASH_PRIME;
			hval ^= (unsigned int)*name++;
		}

		/* do we have it? */
		for(i=0; i<missing->lastentry; i++)
		{
			if (missing->members[i] == hval)
				return 0;
		}
		missing->members[missing->lastentry] = hval;
		missing->lastentry++;
		return 1;
}

void gatherdependence(results, unitid, depth)
set *results;
int unitid;
int depth;
{
PH *header;
char *rd,*endrd;
int kind;

	/* append unitid */
	if (!addmember(results, unitid))
		return;
	
	/* find all other dependencies of this unit */
	header = (PH *)inputs[unitid].content;

	if (verbose==2) fprintf(stderr, "%.*s searching %s\n", depth, "  ", inputs[unitid].srcfile);

	rd = inputs[unitid].content + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize);
	endrd = rd + ntohl(header->relocsize);
	while(rd < endrd)
	{
		relocheader rr;

		/* aligned for m68k */		
		memcpy(&rr, rd, sizeof(rr));

		kind = ntohs(rr.kind);

		/* follow external symbols */
		if (kind & 0x8000)
		{
			/* lookup symbol */
			externalsymbol *es = symbolfind((char *)(rd+sizeof(rr)),ntohs(rr.len));
			if (es)
			{
				if (es->srcunit != unitid)
					gatherdependence(results, es->srcunit, depth+1);
			}
		}

		rd = (char *)(rd+sizeof(rr)) + ntohs(rr.len);
		/* handle broken len fields */
		if (ntohs(rr.len) > 9)
		{
			while (*rd)
				rd++;
		}
	}
}

int symbolemit(out_fd,name, len, segment, finaloffset)
int out_fd;
char *name;
int len;
int segment;
int finaloffset;
{
  int bytes = 0;
	symbolheader sym;
  
  sym.kind = 0;
  sym.offset = htonl(finaloffset);
  sym.segment = htons(segment);
  sym.len = htons(len);
  
	write(out_fd, &sym, sizeof(symbolheader));
	if (sym.len)
		write(out_fd, name, ntohs(sym.len));
	bytes += sizeof(symbolheader);
	bytes += ntohs(sym.len);

	return bytes;
}

int main(argc, argv)
int argc;
char **argv;
{
int inputcount;
char outputname[256];
int fd, out_fd;
int len,n,j,i = 0;
int bssstart;
PH ph;
off_t crp;
externalsymbol *rootsymbol;
set missing;

	/* cmd line args */
	inputcount = 0;
	out_fd = -1;
	for(i=1; i<argc; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == '+')
		{
			if (argv[i][1] == '?')
			{
				printf("Concanetates relocatable object files (.r) into a single file\n");
				printf("Symbols and relocation records are rebased.\n");
				printf("In addition, for a full link, rcat also performs:\n");
				printf("Dependencies are followed from symbol 'Start' to determine\nwhat objects are needed.\n");
				printf("External symbols are resolved to addresses\n");
				exit(0);
			}
			else
			if (argv[i][1] == 'c')
				justcat = 1;
			else
			if (argv[i][1] == 's')
				stripsymbols = 1;
			else
			if (argv[i][1] == 'v')
			{
				verbose = 1;
				if (argv[i][2] == 'v')
					verbose = 2;
			}
			else
			if (argv[i][1] == 'o')
			{
				i++;
				out_fd = creat(argv[i], S_IREAD | S_IWRITE | S_IEXEC);
				if (out_fd < 0)
				{
					fprintf(stderr, "%s: output file failed\n", argv[i]);
					return 0;
				}
				strncpy(outputname, basename(argv[i]), sizeof(outputname));
			}
		}
		else
		{
			 fd = open(argv[i], O_RDONLY);
			if (fd < 0)
			{
				fprintf(stderr, "%s: not found\n", argv[i]);
			}
			else
			{
				PH header;
				char *unitname;
				
				/* read all compile units in file */
				while(1)
				{
					n = read(fd, &header, sizeof(header));
					/* no more units.. */
					if (n != sizeof(header))
						break;

					if (header.magic[0] != 0x05)
					{
						fprintf(stderr, "0x%02x: unknown header type\n",header.magic[0]);
						/* appears to be some kind of catalog of symbols... */
							crp = lseek(fd, 0, SEEK_CUR);

						break;
					}
					n = sizeof(PH) + ntohl(header.textsize) + ntohl(header.datasize) + ntohl(header.relocsize) + ntohl(header.symbolsize);
					n += ntohs(header.commentsize) + ntohs(header.namesize) + ntohs(header.rcssize);
					inputs[inputcount].content = (char *)malloc(n);
					memcpy(inputs[inputcount].content, &header, sizeof(header));
					read(fd, inputs[inputcount].content + sizeof(PH), n - sizeof(PH));

					/* worth only loading symbolcs and relocs? */
					inputs[inputcount].relocs = inputs[inputcount].content + sizeof(PH) + ntohl(header.textsize) + ntohl(header.datasize);
					inputs[inputcount].symbols = inputs[inputcount].relocs + ntohl(header.relocsize);

					/* track usage stats */
					inputs[inputcount].used = 0;
					
					/* do we have a name for this unit? */
					unitname = basename(argv[i]);
					if (ntohs(header.namesize))
						unitname = inputs[inputcount].content + n - ntohs(header.namesize);
					if (*unitname == '\0')
						unitname++;
						
					memset(inputs[inputcount].srcfile, 0, sizeof(inputs[inputcount].srcfile));
					strncpy(inputs[inputcount].srcfile, unitname, sizeof(inputs[inputcount].srcfile));
					inputcount++;
					if (inputcount >= MAXINPUTS)
					{
					  fprintf(stderr, "too many inputs\n");
					  exit(2);	
					}
					if (verbose==2) fprintf(stderr, "added %8.8x %8.8lx %s\n", ntohl(header.textsize), ntohl(header.datasize), unitname);
				}
				close(fd);
			}
		}
	}
	if (verbose) fprintf(stderr, "processing %d compile units\n", inputcount);

	/* one off initialization */
	missing.lastentry = 0;
	buckets[0].symbols = NULL;

#ifdef LINKING
	/* gather all symbols so we can see what is actually referenced */
	symbolclear();
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		char *sdata = inputs[i].symbols;
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader sym;

			/* aligned for m68k */
			memcpy(&sym, sdata, sizeof(sym));

			/* we dont care about finaloffsets, just gathering all symbols */
			symboladd(i, (char *)(sdata+sizeof(sym)), ntohs(sym.len), ntohs(sym.segment), 0);

			/* tek-cc gets this wrong... */
			sdata = (sdata+sizeof(sym)) + ntohs(sym.len);

			/* NB need to handle broken len? */
		}
	}
	/* adding predefined loader symbols */
	symboladd(0, "$$syscall", 9, SEGABS, 0);
	symboladd(0, "ETEXT", 5, SEGTEXT, 0);
	symboladd(0, "EDATA", 5, SEGDATA, 0);
	symboladd(0, "END", 3, SEGBSS, 0);
	if (verbose) fprintf(stderr, "found %d unique symbols\n", externalsymbolcount);
	
	/* if we are performing full link, then walk dependencies to determine working set */
	if (justcat == 0)
	{
		rootsymbol = symbolfind(ENTRYPOINT,ENTRYPOINTLEN);
		if (rootsymbol)
		{
			set depends;
			
			depends.lastentry = 0;
			gatherdependence(&depends, rootsymbol->srcunit, 0);
			for(i=0; i<depends.lastentry; i++)
			{
				inputs[depends.members[i]].used++;
			}
		}
		else
		{
			fprintf(stderr, "can't find '%s' entrypoint\n",ENTRYPOINT);
			return -1;
		}
		
		/* now we can cull unreferenced compileunits */
		for(i=0; i<inputcount; i++)
		{
			/* is it never referenced? */
			if (inputs[i].used == 0)
			{
				if (verbose==2) fprintf(stderr, "%s: unreferenced\n",inputs[i].srcfile);
				inputcount--;
				for(j=i; j<inputcount; j++)
				{
					inputs[j] = inputs[j+1];
				}
				i--;
			}
		}
	}
	if (verbose) fprintf(stderr, "linking %d compile units\n", inputcount);
#endif

	/* setup header type based on if this a simple concat or a full link */
	memset(&ph, 0, sizeof(ph));
	ph.magic[0] = justcat ? 0x05 : 0x04;
	ph.source = htons(2);
	ph.configuration = htons(4);
	ph.rcssize = htons(rcslog());
	ph.namesize = htons(strlen(outputname));
	if (justcat == 0)
	{
		ph.flags = htons(0x8040);
		ph.xferaddress = htonl(0x00000000);
	}
	write(out_fd, &ph, sizeof(ph));

	/* concat text sections and assign final offsets */

	ph.textstart = htonl(0);
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		lseek(out_fd, ntohl(header->textsize), SEEK_CUR);
		inputs[i].textoff = ntohl(ph.textsize);
	
		ph.textsize = htonl(ntohl(ph.textsize) + ntohl(header->textsize));
	}
	
	/* concat data sections (and bss) */
	ph.datastart = htonl(ntohl(ph.textsize));
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		lseek(out_fd, ntohl(header->datasize), SEEK_CUR);
		inputs[i].dataoff = ntohl(ph.datasize);
	
		ph.datasize = htonl(ntohl(ph.datasize) + ntohl(header->datasize));
	}

	/* concat bss sections */
	bssstart = ntohl(ph.textsize) + ntohl(ph.datasize);
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		inputs[i].bssoff = ntohl(ph.bsssize);
	
		ph.bsssize = htonl(ntohl(ph.bsssize) + ntohl(header->bsssize));
	}
	
#ifdef LINKING
	/* if this is a full link, we want addresses, not relative offsets */
	if (justcat == 0)
	{
		if (verbose) fprintf(stderr, "BROKEN Generating addresses\n");
		textstart = 0;
		datastart = 0;
		bssstart = 0;
	}

	/* gather all symbols with final offsets */
	symbolclear();
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		char *sdata = inputs[i].symbols;
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader sym;
			int finaloffset;
			
			/* aligned for m68k */
			memcpy(&sym, sdata, sizeof(sym));

			/* NB finaloffset relative to section starts */
			int finaloffset = 0;
			switch(ntohs(sym.segment))
			{
				case SEGTEXT:
					finaloffset = (ntohl(sym.offset) + inputs[i].textoff);
					break;
				case SEGDATA:
					finaloffset = (ntohl(sym.offset) + inputs[i].dataoff);
					break;
				case SEGBSS:
					finaloffset = (ntohl(sym.offset) + inputs[i].bssoff);
					break;
			}
			
			symboladd(i, (char *)(sdata+sizeof(sym)), ntohs(sym.len), ntohs(sym.segment), finaloffset);
			sdata = (char *)(sdata+sizeof(sym)) + ntohs(sym.len);
			
			/* NB need to handle broken len? */
		}
	}
	/* adding predefined loader symbols */
	symboladd(0, "$$syscall", 9, SEGABS, 0x00004e4f);		// this is an ABS symbol in system.boot,  available elsewhere (systat)?
	symboladd(0, "ETEXT", 5, SEGTEXT, ntohl(ph.textsize));
	symboladd(0, "EDATA", 5, SEGDATA, ntohl(ph.textsize) + ntohl(ph.datasize));
	symboladd(0, "END", 3, SEGBSS, ntohl(ph.textsize) + ntohl(ph.datasize) + ntohl(ph.bsssize));
	if (verbose) fprintf(stderr, "using %d unique symbols\n", externalsymbolcount);

	/* if this is a full link, we want a xfer address */
	if (justcat == 0)
	{
		ph.flags = htons(0x8040);
		ph.xferaddress = htonl(TEXTBASE);
		rootsymbol = symbolfind(ENTRYPOINT,ENTRYPOINTLEN);
		if (rootsymbol)
		{
			ph.xferaddress = htonl(rootsymbol->finaloffset);
		}
	}
#endif
	

	/* rebase reloc records and resolve external symbols */
	n = 0;
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		/* update every reloc record */
		char *rd = inputs[i].relocs;
		char *endrd = rd + ntohl(header->relocsize);
		while(rd < endrd)
		{
			relocheader *rr = (relocheader *)rd;

			int kind = ntohs(rr->kind);
			int *addend;

			int addendoffset = 0;
			switch(kind & 0xf0)
			{
				case 0x00:
					addendoffset = 0;
					break;
				case 0x40:
					addendoffset = inputs[i].textoff;
					break;
				case 0x80:
					addendoffset = inputs[i].dataoff;
					break;
				case 0xc0:
					addendoffset = inputs[i].bssoff;
					break;
			}

#ifdef LINKING
			// resolve external to internal
			if (kind & 0x8000)
			{
				// lookup final offset
				externalsymbol *es = symbolfind((char *)(rr+1),ntohs(rr->len));
				if (es)
				{
					fprintf(stderr, "%20s: resolved symbol for %s with symbol in %s\n", (char *)(rr+1), inputs[i].srcfile, inputs[es->srcunit].srcfile);
					addendoffset = es->finaloffset;
				}
				else
				if (addmissing(&missing, (rd+sizeof(rr)),(int)ntohs(rr.len)))
				{
					fprintf(stderr, "%32s: missing symbol for %s\n", (char *)(rr+1), inputs[i].srcfile);
				}
			}
#endif

			/* relocation for which segment; NB addend is relative to in-file sections not in-memory address */
			addend = NULL;
			switch(kind & 0xf)
			{
				/* TEXT */
				case 1:
					addend = (int *)(inputs[i].content + sizeof(PH) + ntohl(rr->offset));
					rr->offset = htonl(ntohl(rr->offset) + inputs[i].textoff);
					break;
					
				/* DATA */
				case 2:
					addend = (int *)(inputs[i].content + sizeof(PH) + ntohl(header->textsize) + ntohl(rr->offset));
					rr->offset = htonl(ntohl(rr->offset) + inputs[i].dataoff);
					break;

				default:
					fprintf(stderr, "unknown kind %d\n", kind & 0xf);
					break;
			}

			if (addend)
				*addend = htonl(ntohl(*addend) + addendoffset);

			rd = (char *)(rr+1) + ntohs(rr->len);
			/* handle broken len fields */
			if (ntohs(rr->len) > 9)
			{
				while (*rd)
					rd++;
			}

			/* counting records */
			n++;
		}

		if (justcat)
		{
			write(out_fd, inputs[i].content + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize), ntohl(header->relocsize));
			ph.relocsize = htonl(ntohl(ph.relocsize) + ntohl(header->relocsize));
		}
	}
	if (verbose) fprintf(stderr, "%d relocation records\n", n);

	/* export symbols if requested */
	n = 0;
	len = 0;
	symbolclear();
	if (stripsymbols == 0)
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		char *sdata = inputs[i].symbols;
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader sym;
			
			/* aligned for m68k */
			memcpy(&sym, sdata, sizeof(sym));
			
			/* only if we have not seen it before */
			if (!symboladd(i, (char *)(sdata+sizeof(sym)),ntohs(sym.len), ntohs(sym.segment), 0))
			{
				n++;
				switch(ntohs(sym.segment))
				{
					case SEGTEXT:
						sym.offset = htonl(ntohl(sym.offset) + inputs[i].textoff);
						break;
					case SEGDATA:
						sym.offset = htonl(ntohl(sym.offset) + inputs[i].dataoff);
						break;
					case SEGBSS:
						sym.offset = htonl(ntohl(sym.offset) + inputs[i].bssoff);
						break;
				}

				write(out_fd, &sym, sizeof(symbolheader));
				if (sym.len)
					write(out_fd, sdata+sizeof(sym), ntohs(sym.len));
				len += ntohs(sym.len);
			}
			
			sdata = (sdata+sizeof(sym)) + ntohs(sym.len);
		}
	}
	ph.symbolsize = htonl(len);
	fprintf(stderr, "%d symbols ", n);
	if (missing.lastentry)
		fprintf(stderr, "*** %d missing symbols (unresolved)", missing.lastentry);
	fprintf(stderr,"\n");
    
	/* RCS section */
	write(out_fd, rcs, strlen(rcs));
	
	/* name section */
	write(out_fd, outputname, strlen(outputname));
	
	/* update header with final values */
	lseek(out_fd, 0, SEEK_SET);
	write(out_fd, &ph, sizeof(ph));

	/* write edited text (accounting for rounding) */
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		write(out_fd, inputs[i].content + sizeof(PH), ntohl(header->textsize));
	}
	/* write edited datas (accounting for rounding) */
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		write(out_fd, inputs[i].content + sizeof(PH) + ntohl(header->textsize), ntohl(header->datasize));
	}

	close(out_fd);

	if (verbose)
	{
		fprintf(stderr, "-----------------\n");
		fprintf(stderr, "textstart = %8.8x\n", ntohl(ph.textstart));
		fprintf(stderr, "textsize  = %8.8x\n", ntohl(ph.textsize));
		fprintf(stderr, "datastart = %8.8x\n", ntohl(ph.datastart));
		fprintf(stderr, "datasize  = %8.8x\n", ntohl(ph.datasize));
		fprintf(stderr, "bsssize   = %8.8x\n", ntohl(ph.bsssize));
		fprintf(stderr, "xferaddr  = %8.8x\n", ntohl(ph.xferaddress));
		fprintf(stderr, "relocsize = %8.8x\n", ntohl(ph.relocsize));
		fprintf(stderr, "symbolsize= %8.8x\n", ntohl(ph.symbolsize));
		fprintf(stderr, "rcssize   = %8.8x\n", ntohs(ph.rcssize));
		fprintf(stderr, "namesize  = %8.8x\n", ntohs(ph.namesize));
	}

  return 0;
}
