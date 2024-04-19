#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <memory.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/errno.h>
  
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define O_APPEND        0x00000008      /* set append mode */
#define O_CREAT         0x00000200      /* create if nonexistant */
#define O_TRUNC         0x00000400      /* truncate to zero length */

#define min(A,B)  (A < B ? A : B)

// cmd line options
int verbose = 0;
int stripsymbols = 0;
int justcat = 0;


void datacpy(int dst, int src, int len)
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
	int srcunit;
	unsigned int namehash;
	int segment;
	int finaloffset;
} externalsymbol;

int externalsymbolcount = 0;
int externalsymbolmax;
externalsymbol *symbols = NULL;
void symbolclear()
{
	externalsymbolcount = 0;
	externalsymbolmax = 1024;
	symbols = (externalsymbol *)malloc(sizeof(externalsymbol) * externalsymbolmax);
}

#define HASH_INIT (0x811c9dc5u)
#define HASH_PRIME (0x01000193u)

int symboladd(int unit, char *name, int len, int segment, int finaloffset)
{
	if (len > 0)
	{
		// generate a hash for this string
		unsigned int hval = HASH_INIT;
		while (len--)
		{
			hval *= HASH_PRIME;
			hval ^= (unsigned int)*name++;
		}

		// do we have it?
		for(int i=0; i<externalsymbolcount; i++)
		{
			if (symbols[i].namehash == hval)
			{
				return 1;
			}
		}

		// add it and resize if needed
		symbols[externalsymbolcount].srcunit = unit;
		symbols[externalsymbolcount].namehash = hval;
		symbols[externalsymbolcount].segment = segment;
		symbols[externalsymbolcount].finaloffset = finaloffset;
		externalsymbolcount++;
		if (externalsymbolcount >= externalsymbolmax)
		{
			externalsymbolmax *= 2;
			symbols = realloc(symbols, sizeof(externalsymbol) * externalsymbolmax);
		}
	}
	
	return 0;
}

externalsymbol *symbolfind(char *name, int len)
{
	if (len > 0)
	{
		// generate a hash for this string
		unsigned int hval = HASH_INIT;
		while (len--)
		{
			hval *= HASH_PRIME;
			hval ^= (unsigned int)*name++;
		}

		// do we have it?
		for(int i=0; i<externalsymbolcount; i++)
		{
			if (symbols[i].namehash == hval)
			{
				return symbols + i;
			}
		}
	}
	
	return 0;
}

// simple set of integers
typedef struct
{
	int members[4096];
	int last;
} set;

int addmember(set *results, int unit)
{
	int i;
	for(i=0; i<results->last; i++)
	{
		if (results->members[i] == unit)
			return 0;
	}
	results->members[results->last] = unit;
	results->last++;
	return 1;
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

compileunit inputs[4096];

void gatherdependence(set *results, int i)
{
	// append unitid
	if (!addmember(results, i))
		return;
	
	// find all other dependencies of this unit
	PH *header = (PH *)inputs[i].content;

	char *rd = inputs[i].content + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize);
	char *endrd = rd + ntohl(header->relocsize);
	while(rd < endrd)
	{
		relocheader *rr = (relocheader *)rd;

		int kind = ntohs(rr->kind);

		// follow external symbols
		if (kind & 0x8000)
		{
			// lookup symbol
			externalsymbol *es = symbolfind((char *)(rr+1),ntohs(rr->len));
			if (es)
			{
				if (es->srcunit != i)
					gatherdependence(results, es->srcunit);
			}
			else
			{
				if (verbose) fprintf(stderr, "%32s: missing symbol for %s\n", (char *)(rr+1), inputs[i].srcfile);
			}
		}

		rd = (char *)(rr+1) + ntohs(rr->len);
		/* handle broken len fields */
		if (ntohs(rr->len) > 9)
		{
			while (*rd)
				rd++;
		}
	}
}

int main(int argc, char *argv[])
{
int inputcount;
char outputname[256];
int fd, out_fd;
int len,n,j,i = 0;
PH ph = {};
off_t crp;

	/* cmd line args */
	inputcount = 0;
	out_fd = -1;
	for(i=1; i<argc; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == '+')
		{
			if (argv[i][1] == 'c')
				justcat = 1;
			else
			if (argv[i][1] == 's')
				stripsymbols = 1;
			else
			if (argv[i][1] == 'v')
				verbose = 1;
			else
			if (argv[i][1] == 'o')
			{
				i++;
				out_fd = open(argv[i], O_RDWR | O_CREAT | O_TRUNC, 0666);
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
				
				// read all compile units in file
				while(1)
				{
					n = read(fd, &header, sizeof(header));
					// no more units..
					if (n != sizeof(header))
						break;

					if (header.magic[0] != 0x05)
					{
						fprintf(stderr, "0x%02x: unknown header type\n",header.magic[0]);
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

					// track usage stats
					inputs[inputcount].used = 0;
					
					// do we have a name for this unit?
					unitname = basename(argv[i]);
					if (ntohs(header.namesize))
						unitname = inputs[inputcount].content + n - ntohs(header.namesize);
					if (*unitname == '\0')
						unitname++;
						
					memset(inputs[inputcount].srcfile, 0, sizeof(inputs[inputcount].srcfile));
					strncpy(inputs[inputcount].srcfile, unitname, sizeof(inputs[inputcount].srcfile));
					inputcount++;
					if (verbose) fprintf(stderr, "added %08x %08lx %s\n", ntohl(header.textsize), ntohl(header.datasize), unitname);
				}
				close(fd);
			}
		}
	}
	if (verbose) fprintf(stderr, "processing %d compile units\n", inputcount);

	// gather all symbols so we can see what is actually referenced
	symbolclear();
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		char *sdata = inputs[i].symbols;
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader *sym = (symbolheader *)sdata;

			/* we dont care about finaloffsets, just gathering all symbols */
			symboladd(i, (char *)(sym+1),ntohs(sym->len), ntohs(sym->segment), 0);
			
			sdata = (char *)(sym+1) + ntohs(sym->len);
			// NB need to handle broken len?
		}
	}
	// adding predefined loader symbols
	symboladd(0, "$$syscall", 9, SEGABS, 0);
	symboladd(0, "ETEXT", 5, SEGTEXT, 0);
	symboladd(0, "EDATA", 5, SEGDATA, 0);
	symboladd(0, "END", 3, SEGBSS, 0);
	if (verbose) fprintf(stderr, "found %d unique symbols\n", externalsymbolcount);
	

	// recursively visit nodes from Start
	externalsymbol *rootsymbol = symbolfind("Start",5);
	if (rootsymbol)
	{
		set depends;
		
		depends.last = 0;
		gatherdependence(&depends, rootsymbol->srcunit);
		for(i=0; i<depends.last; i++)
		{
			inputs[depends.members[i]].used++;
		}
	}
	else
	{
		fprintf(stderr, "can't find Start entrypoint\n");
		return -1;
	}
	
	/* now we can cull unreferenced compileunits */
	for(i=0; i<inputcount; i++)
	{
		/* is it never referenced? */
		if (inputs[i].used == 0)
		{
			//if (verbose) fprintf(stderr, "%s: unreferenced\n",inputs[i].srcfile);
			inputcount--;
			for(j=i; j<inputcount; j++)
			{
				inputs[j] = inputs[j+1];
			}
			i--;
		}
	}
	if (verbose) fprintf(stderr, "linking %d compile units\n", inputcount);


	// setup header type based on if this a simple concat or a full link
	ph.magic[0] = justcat ? 0x05 : 0x04;
	ph.source = htons(2);
	ph.configuration = htons(4);
	ph.namesize = htons(strlen(outputname));
	write(out_fd, &ph, sizeof(ph));

	// concat text sections and assign final offsets
	int textstart = 0;
	ph.textstart = htonl(textstart);
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		lseek(out_fd, ntohl(header->textsize), SEEK_CUR);
		inputs[i].textoff = ntohl(ph.textsize);
	
		ph.textsize = htonl(ntohl(ph.textsize) + ntohl(header->textsize));
	}
	
	// concat data sections
	ph.datastart = htonl(ntohl(ph.textsize));
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		lseek(out_fd, ntohl(header->datasize), SEEK_CUR);
		inputs[i].dataoff = ntohl(ph.datastart) + ntohl(ph.datasize);
	
		ph.datasize = htonl(ntohl(ph.datasize) + ntohl(header->datasize));
	}

	// concat bss sections
	int bssstart = ntohl(ph.textsize) + ntohl(ph.datasize);
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		inputs[i].bssoff = bssstart + ntohl(ph.bsssize);
	
		ph.bsssize = htonl(ntohl(ph.bsssize) + ntohl(header->bsssize));
	}

	// gather all symbols with final offsets
	symbolclear();
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		char *sdata = inputs[i].symbols;
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader *sym = (symbolheader *)sdata;

			int finaloffset = 0;
			switch(ntohs(sym->segment))
			{
				case SEGTEXT:
					finaloffset = (ntohl(sym->offset) + inputs[i].textoff);
					break;
				case SEGDATA:
					finaloffset = (ntohl(sym->offset) + inputs[i].dataoff);
					break;
				case SEGBSS:
					finaloffset = (ntohl(sym->offset) + inputs[i].bssoff);
					break;
			}
			
			symboladd(i, (char *)(sym+1),ntohs(sym->len), ntohs(sym->segment), finaloffset);
			
			sdata = (char *)(sym+1) + ntohs(sym->len);
			// NB need to handle broken len?
		}
	}
	// adding predefined loader symbols
	symboladd(0, "$$syscall", 9, SEGABS, 0x00004e4f);		// this is an ABS symbol in system.boot,  available elsewhere (systat)?
	symboladd(0, "ETEXT", 5, SEGTEXT, ntohl(ph.textsize));
	symboladd(0, "EDATA", 5, SEGDATA, ntohl(ph.textsize) + ntohl(ph.datasize));
	symboladd(0, "END", 3, SEGBSS, ntohl(ph.textsize) + ntohl(ph.datasize) + ntohl(ph.bsssize));
	if (verbose) fprintf(stderr, "using %d unique symbols\n", externalsymbolcount);
	
	

	// rebase reloc records and resolve external symbols
	n = 0;
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;

		// update every reloc record
		char *rd = inputs[i].relocs;
		char *endrd = rd + ntohl(header->relocsize);
		while(rd < endrd)
		{
			relocheader *rr = (relocheader *)rd;

			// target segment to which this relocation refers
			int kind = ntohs(rr->kind);
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

			// resolve external to internal
			if (kind & 0x8000)
			{
				// lookup final offset
				externalsymbol *es = symbolfind((char *)(rr+1),ntohs(rr->len));
				if (es)
				{
					//fprintf(stderr, "%32s: resolved symbol for %s with symbol in %s\n", (char *)(rr+1), inputs[i].srcfile, inputs[es->srcunit].srcfile);
					addendoffset = es->finaloffset;
				}
				else
				{
					fprintf(stderr, "%32s: missing symbol for %s\n", (char *)(rr+1), inputs[i].srcfile);
				}
			}

			// relocation for which segment
			int *addend = NULL;
			switch(kind & 0xf)
			{
				case 1:
					addend = (int *)(inputs[i].content + sizeof(PH) + ntohl(rr->offset));
					rr->offset = htonl(ntohl(rr->offset) + inputs[i].textoff);
					break;
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
		}

		if (justcat)
		{
			write(out_fd, inputs[i].content + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize), ntohl(header->relocsize));
			ph.relocsize = htonl(ntohl(ph.relocsize) + ntohl(header->relocsize));
			n++;
		}
	}
	if (verbose) fprintf(stderr, "%d relocations\n", n);

	// export symbols if requested
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
			symbolheader *sym = (symbolheader *)sdata;
			
			// only if we have not seen it before
			if (!symboladd(i, (char *)(sym+1),ntohs(sym->len), ntohs(sym->segment), 0))
			{
				n++;
				switch(ntohs(sym->segment))
				{
					case SEGTEXT:
						sym->offset = htonl(ntohl(sym->offset) + inputs[i].textoff);
						break;
					case SEGDATA:
						sym->offset = htonl(ntohl(sym->offset) + inputs[i].dataoff);
						break;
					case SEGBSS:
						sym->offset = htonl(ntohl(sym->offset) + inputs[i].bssoff);
						break;
				}

				write(out_fd, sym, sizeof(symbolheader));
				if (sym->len)
					write(out_fd, sym+1, ntohs(sym->len));
				len += sizeof(symbolheader);
				len += ntohs(sym->len);
			}
			
			sdata = (char *)(sym+1) + ntohs(sym->len);
		}
	}
	ph.symbolsize = htonl(len);
	if (verbose) fprintf(stderr, "%d symbols\n", n);

	// name section
	write(out_fd, outputname, strlen(outputname));
	
	// update header with final values
	lseek(out_fd, 0, SEEK_SET);
	write(out_fd, &ph, sizeof(ph));

	// write edited text
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		write(out_fd, inputs[i].content + sizeof(PH), ntohl(header->textsize));
	}
	// write edited datas
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i].content;
		write(out_fd, inputs[i].content + sizeof(PH) + ntohl(header->textsize), ntohl(header->datasize));
	}

	close(out_fd);

	if (verbose)
	{
		fprintf(stderr, "-----------------\n");
		fprintf(stderr, "textstart = %08x\n", ntohl(ph.textstart));
		fprintf(stderr, "textsize = %08x\n", ntohl(ph.textsize));
		fprintf(stderr, "datastart = %08x\n", ntohl(ph.datastart));
		fprintf(stderr, "datasize = %08x\n", ntohl(ph.datasize));
		fprintf(stderr, "bsssize = %08x\n", ntohl(ph.bsssize));
		fprintf(stderr, "relocsize = %08x\n", ntohl(ph.relocsize));
		fprintf(stderr, "symbolsize = %08x\n", ntohl(ph.symbolsize));
		fprintf(stderr, "namesize = %08x\n", ntohs(ph.namesize));
	}

  return 0;
}

