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

char buffer[4096];
int verbose = 0;

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


int hashcount = 0;
int hashmax;
unsigned int *hashes = NULL;
void symbolclear()
{
	hashcount = 0;
	hashmax = 1024;
	hashes = (unsigned int *)malloc(sizeof(unsigned int) * hashmax);
}

#define HASH_INIT (0x811c9dc5u)
#define HASH_PRIME (0x01000193u)

int symbolexists(char *name, int len)
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
		for(int i=0; i<hashcount; i++)
		{
			if (hashes[i] == hval)
				return 1;
		}

		// add it and resize if needed
		hashes[hashcount++] = hval;
		if (hashcount >= hashmax)
		{
			hashmax *= 2;
			hashes = realloc(hashes, sizeof(unsigned int) * hashmax);
		}
	}
	
	return 0;
}



int main(int argc, char *argv[])
{
int inputcount;
char *inputs[128];
char outputname[256];
int fd, out_fd;
int len,n,i = 0;
PH ph = {};
off_t crp;

	inputcount = 0;
	out_fd = -1;
	for(i=1; i<argc; i++)
	{
		if (!strcmp(argv[i],"-v"))
			verbose = 1;
		else
		if (!strcmp(argv[i],"-o"))
		{
			i++;
			out_fd = open(argv[i], O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (out_fd < 0)
			{
				fprintf(stderr, "%s: output file failed\n", argv[i]);
				return 0;
			}
			strcpy(outputname, basename(argv[i]));
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
				// read whole file
				crp = lseek(fd, 0, SEEK_END);
				inputs[inputcount] = (char *)malloc(crp);
				lseek(fd, 0, SEEK_SET);
				read(fd, inputs[inputcount], crp);
				close(fd);
				inputcount++;
			}
		}
	}

	if (verbose) fprintf(stderr, "concatenating %d files\n", inputcount);

	// setup header type
	ph.magic[0] = 0x05;
	ph.source = htons(2);
	ph.configuration = htons(4);
	ph.namesize = htons(strlen(outputname));
	
	write(out_fd, &ph, sizeof(ph));

	// concat texts
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];
		lseek(out_fd, ntohl(header->textsize), SEEK_CUR);
		
		ph.textsize = htonl(ntohl(ph.textsize) + ntohl(header->textsize));
	}
	// concat datas
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];
		lseek(out_fd, ntohl(header->datasize), SEEK_CUR);

		ph.datasize = htonl(ntohl(ph.datasize) + ntohl(header->datasize));
		ph.bsssize = htonl(ntohl(ph.bsssize) + ntohl(header->bsssize));
	}

	// reloc records
	int textoffset = 0;
	int dataoffset = 0;
	int bssoffset = 0;
	n = 0;
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];

		// update every reloc record
		char *rd = inputs[i] + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize);
		char *endrd = rd + ntohl(header->relocsize);
		while(rd < endrd)
		{
			relocheader *rr = (relocheader *)rd;

			n++;
			int kind = ntohs(rr->kind);
			int addendoffset = 0;
			switch(kind & 0xf0)
			{
				case 0x00:
					addendoffset = 0;
					break;
				case 0x40:
					addendoffset = textoffset;
					break;
				case 0x80:
					addendoffset = dataoffset;
					break;
				case 0xc0:
					addendoffset = bssoffset;
					break;
			}

			int *addend = NULL;
			switch(kind & 0xf)
			{
				case 1:
					addend = (int *)(inputs[i] + sizeof(PH) + ntohl(rr->offset));
					rr->offset = htonl(ntohl(rr->offset) + textoffset);
					break;
				case 2:
					addend = (int *)(inputs[i] + sizeof(PH) + ntohl(header->textsize) + ntohl(rr->offset));
					rr->offset = htonl(ntohl(rr->offset) + dataoffset);
					break;
			}

			if (addend)
				*addend = htonl(ntohl(*addend) + addendoffset);

			rd = (char *)(rr+1) + ntohs(rr->len);
		}
		
		write(out_fd, inputs[i] + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize), ntohl(header->relocsize));

		textoffset += ntohl(header->textsize);
		dataoffset += ntohl(header->datasize);
		bssoffset += ntohl(header->bsssize);

		ph.relocsize = htonl(ntohl(ph.relocsize) + ntohl(header->relocsize));
	}
	if (verbose) fprintf(stderr, "%d relocations\n", n);

	// unique symbols
	textoffset = 0;
	dataoffset = 0;
	bssoffset = 0;
	n = 0;
	len = 0;
	symbolclear();
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];

		char *sdata = inputs[i] + sizeof(PH) + ntohl(header->textsize) + ntohl(header->datasize) + ntohl(header->relocsize);
		char *endsd = sdata + ntohl(header->symbolsize);
		while(sdata < endsd)
		{
			symbolheader *sym = (symbolheader *)sdata;
			
			// only if we have notr seen it before
			if (!symbolexists((char *)(sym+1),ntohs(sym->len)))
			{
				n++;
				switch(ntohs(sym->segment))
				{
					case SEGTEXT:
						sym->offset = htonl(ntohl(sym->offset) + textoffset);
						break;
					case SEGDATA:
						sym->offset = htonl(ntohl(sym->offset) + dataoffset);
						break;
					case SEGBSS:
						sym->offset = htonl(ntohl(sym->offset) + bssoffset);
						break;
				}

				write(out_fd, sym, sizeof(symbolheader));
				if (sym->len)
					write(out_fd, sym+1, ntohs(sym->len));
				len += sizeof(symbolheader);
				len += ntohs(sym->len);
			}
			else
			{
				if (verbose) fprintf(stderr, "%*s: duplicate symbol in %d\n", ntohs(sym->len), (char *)(sym+1), i);
			}
			
			sdata = (char *)(sym+1) + ntohs(sym->len);
		}

		textoffset += ntohl(header->textsize);
		dataoffset += ntohl(header->datasize);
		bssoffset += ntohl(header->bsssize);
	}
	ph.symbolsize = htonl(len);
	if (verbose) fprintf(stderr, "%d symbols\n", n);
	
	// name
	write(out_fd, outputname, strlen(outputname));
	
	// update header with final values
	lseek(out_fd, 0, SEEK_SET);
	write(out_fd, &ph, sizeof(ph));

	// write edited text
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];
		write(out_fd, inputs[i] + sizeof(PH), ntohl(header->textsize));
	}
	// concat datas
	for(i=0; i<inputcount; i++)
	{
		PH *header = (PH *)inputs[i];
		write(out_fd, inputs[i] + sizeof(PH) + ntohl(header->textsize), ntohl(header->datasize));
	}

	close(out_fd);

	if (verbose)
	{
		fprintf(stderr, "-----------------\n");
		fprintf(stderr, "textstart = %08x\n", ntohl(ph.textstart));
		fprintf(stderr, "textsize = %08x\n", ntohl(ph.textsize));
		fprintf(stderr, "datastart = %08x\n", ntohl(ph.datastart));
		fprintf(stderr, "datasize = %08x\n", ntohl(ph.datasize));
		fprintf(stderr, "relocsize = %08x\n", ntohl(ph.relocsize));
		fprintf(stderr, "symbolsize = %08x\n", ntohl(ph.symbolsize));
		fprintf(stderr, "namesize = %08x\n", ntohs(ph.namesize));
	}

  return 0;
}

