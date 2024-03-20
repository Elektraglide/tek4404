#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/errno.h>
  
#include "elf.h"
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

int readshort(int fd)
{
  short val;
  
  read(fd, &val, sizeof(val));
  return ntohs(val);
}
int readlong(int fd)
{
  int val;
  
  read(fd, &val, sizeof(val));
  return ntohl(val);
}

char strtab[65536] = "\0";

/* ongoing string lump offsets */
char shstrtab[65536] = "\0";

/* ongoing data lump offsets */
int dataoffset = 0;
int adddata(int len)
{
  int startpos = dataoffset;
  dataoffset += len;
  return startpos;
}

int findnamedsect(char *needle, Elf32_Shdr *sect, int numsects)
{
	while (numsects-- > 0)
	{
		if (!strcmp(needle, shstrtab + ntohl(sect[numsects].sh_name)))
		{
			break;
		}
	}
	return numsects;
}

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

int relacompare(const void *a, const void *b)
{
Elf32_Rela *r1 = (Elf32_Rela *)a;
Elf32_Rela *r2 = (Elf32_Rela *)b;

int i = ntohl(r1->r_offset);
int j = ntohl(r2->r_offset);

	return (i - j);
}

// fixup rodata section offset to account for data
void fixup_rodata_offset(Elf32_Rela *rarray, int n, Elf32_Sym *symbols, int rodataindex, int dataoffset)
{
	int i;
	int targetsection;
	
	for(i=0; i<n; i++)
	{
		int symindex = ELF32_R_SYM(ntohl(rarray[i].r_info));
		int rtype = ELF32_R_TYPE(ntohl(rarray[i].r_info));

		// lookup symbol
		int symtype = ELF32_ST_TYPE(symbols[symindex].st_info);
		
		if (rtype == R_68K_32)
		{
			if (symtype == STT_SECTION)
			{
				targetsection = ntohs(symbols[symindex].st_shndx);

				// rodata offsets need to be adjusted because they are relative to data (we dont have rodata in Uniflex)
				if (targetsection == rodataindex)
				{
					//rarray[i].r_offset = htonl(ntohl(rarray[i].r_offset) + dataoffset);
					rarray[i].r_addend = htonl(ntohl(rarray[i].r_addend) + dataoffset);
					if (verbose) fprintf(stderr, "%08x: fixing rodata addend\n", ntohl(rarray[i].r_offset));
				}
			}
		}
	}
}

int emitreloc(int ph_fd, Elf32_Rela *rarray, int numrecords, Elf32_Sym *symbols, int textindex, int dataindex, int rodataindex, int bssindex, PH *ph, int targetsection)
{
	int i;
	int len = 0;

	// calc offset to targetsection knowing we always have: text,data,(bss)
	int sectionstart = 0;
	if (targetsection == dataindex)	sectionstart = ntohl(ph->textsize);
	if (targetsection == rodataindex)	sectionstart = ntohl(ph->textsize) + ntohl(ph->datasize);
	
	int instsegment = 1;
	if (targetsection == dataindex)	instsegment = 2;
	if (targetsection == rodataindex)	instsegment = 2;
	if (targetsection == bssindex)	instsegment = 3;
	
	for(i=0; i<numrecords; i++)
	{
		Elf32_Rela elfreloc;
		relocheader rel;
		
		elfreloc = rarray[i];
		int rsymindex = ELF32_R_SYM(ntohl(elfreloc.r_info));
		int rtype = ELF32_R_TYPE(ntohl(elfreloc.r_info));

		rel.offset = htonl(ntohl(elfreloc.r_offset));

		// lookup symbol
		int symtype = ELF32_ST_TYPE(symbols[rsymindex].st_info);
		char *symbolname = strtab + ntohl(symbols[rsymindex].st_name);
		int namelen = strlen(symbolname);
		rel.len = ntohs(namelen);

		if (rtype == R_68K_NONE)
		{
			rel.kind = 0;
		}
		else
		if (rtype == R_68K_32)
		{
			rel.kind = htons(0x8000 + instsegment);

			if (symtype == STT_SECTION)
			{
				int targetsection = ntohs(symbols[rsymindex].st_shndx);
				
				if (targetsection == bssindex)
				{
					symbolname = "BSS";
					rel.kind = htons(0x00c0 + instsegment);
				}
				else
				if (targetsection == rodataindex)
				{
					symbolname = "RODATA";
					rel.kind = htons(0x0080 + instsegment);
				}
				else
				if (targetsection == dataindex)
				{
					symbolname = "DATA";
					rel.kind = htons(0x0080 + instsegment);
				}
				else
				if (targetsection == textindex)
				{
					symbolname = "TEXT";
					rel.kind = htons(0x0040 + instsegment);
				}
				
			}
			
			// this is horribly inefficient...
			// Uniflex does not have addend reloc records, so we need to embed its value like a Elf32_Rel
			if (elfreloc.r_addend != 0)
			{
				off_t crp = lseek(ph_fd, 0, SEEK_CUR);
					lseek(ph_fd, sizeof(PH) + sectionstart + ntohl(elfreloc.r_offset), SEEK_SET);
					int v = htonl(ntohl(elfreloc.r_addend));
			//		v = htonl(0xDEADDEAD);
					write(ph_fd, &v, sizeof(v));
				lseek(ph_fd, crp, SEEK_SET);
			}
		}
		else
		if (rtype == R_68K_PC32)
		{
			// FIXME:
			rel.kind = htons(0x0081);
		}
		else
		{
			rel.kind = 0;
		}
		
		if (verbose) fprintf(stderr, "%32s %04x %08x addend(%08x)\n", symbolname, ntohs(rel.kind), ntohl(rel.offset), ntohl(elfreloc.r_addend) );
		
		write(ph_fd, &rel, sizeof(rel));
		if (namelen > 0)
			write(ph_fd, symbolname, namelen);
		len += sizeof(rel);
		len += namelen;
	}

	return len;
}

int main(int argc, char *argv[])
{
int inputindex;
int fd, ph_fd;
int len,n,i = 0;
PH ph = {};
Elf32_Ehdr eh;
char buffer[1024];
char *typename;
off_t crp;
int exportlocals = 0;

	inputindex = 0;
	for(i=1; i<argc; i++)
	{
		if (!strcmp(argv[i],"-v"))
			verbose = 1;
		else
		if (!strcmp(argv[i],"-l"))
			exportlocals = 1;
		else
			inputindex = i;
	}

  fd = open(argv[inputindex], O_RDONLY);
  if (fd < 0)
  {
      fprintf(stderr, "%s: not found\n", argv[inputindex]);
      return(1);
  }

	// explore ELF
	read(fd, &eh, sizeof(Elf32_Ehdr));

  if (verbose) fprintf(stderr, "relocatable:  %s \n", strrchr(argv[inputindex],'/'));
	if (ntohs(eh.e_machine) != EM_68K)
	{
      fprintf(stderr, "%s: not m68k\n", argv[inputindex]);
      return(1);
	}
	if (ntohs(eh.e_type) != ET_REL)
	{
      fprintf(stderr, "%s: not relocatable file\n", argv[inputindex]);
      return(1);
	}

	// PH offsets start after header
	adddata(sizeof(ph));
	
	// get sections
  Elf32_Shdr sect[16];
	lseek(fd, ntohl(eh.e_shoff), SEEK_SET);
	read(fd, sect, ntohs(eh.e_shnum) * sizeof(Elf32_Shdr));

	// get strings
	int strindex = ntohs(eh.e_shstrndx);
	lseek(fd, ntohl(sect[strindex].sh_offset), SEEK_SET);
	read(fd, shstrtab, ntohl(sect[strindex].sh_size) );

	for(i=1; i<ntohs(eh.e_shnum); i++)
	{
		if (verbose) fprintf(stderr, "%d: %s\n", i, shstrtab + ntohl(sect[i].sh_name));
	}

	int stindex = findnamedsect(".strtab", sect, ntohs(eh.e_shnum));
	lseek(fd, ntohl(sect[stindex].sh_offset), SEEK_SET);
	read(fd, strtab, ntohl(sect[stindex].sh_size) );

	n = 0;
	int textindex = findnamedsect(".text", sect, ntohs(eh.e_shnum));
	if (textindex > 0)
		n += ntohl(sect[textindex].sh_size);
	//ph.textstart = htonl(adddata(n));
	ph.textsize = htonl(n);
	
	// rodata gets appended to data
	n = 0;
	int dataindex = findnamedsect(".data", sect, ntohs(eh.e_shnum));
	if (dataindex > 0)
		n += ntohl(sect[dataindex].sh_size);
		
	int rodataindex = findnamedsect(".rodata", sect, ntohs(eh.e_shnum));
	if (rodataindex < 0)
		rodataindex = findnamedsect(".rodata.str1.1", sect, ntohs(eh.e_shnum));
	if (rodataindex > 0)
		n += ntohl(sect[rodataindex].sh_size);
	// ensure even length
	if (n & 1)
	{
		n++;
		if (rodataindex > 0)
			sect[rodataindex].sh_size = htonl(ntohl(sect[rodataindex].sh_size) + 1);
		else
			sect[dataindex].sh_size = htonl(ntohl(sect[dataindex].sh_size) + 1);
	}
		
		
	//ph.datastart = htonl(adddata(n));
	ph.datasize = htonl(n);

	int commentindex = findnamedsect(".comment", sect, ntohs(eh.e_shnum));
//	ph.commentsize = htons(ntohl(sect[commentindex].sh_size));
	ph.namesize = htons(ntohl(sect[commentindex].sh_size));

	n = 0;
	int bssindex = findnamedsect(".bss", sect, ntohs(eh.e_shnum));
	if (bssindex > 0)
		n += ntohl(sect[bssindex].sh_size);
	ph.bsssize = htonl(n);
	
	int symindex = findnamedsect(".symtab", sect, ntohs(eh.e_shnum));
	ph.symbolsize = htonl(ntohl(sect[symindex].sh_size) / ntohl(sect[symindex].sh_entsize));		// NB numentries for now
	
	
	int relatextindex = findnamedsect(".rela.text", sect, ntohs(eh.e_shnum));
	ph.relocsize = htonl(ntohl(sect[relatextindex].sh_size) / ntohl(sect[relatextindex].sh_entsize));		// NB numentries for now;
	int relatextsect = relatextindex > 0 ? ntohl(sect[relatextindex].sh_info) : -1;

	int reladataindex = findnamedsect(".rela.data", sect, ntohs(eh.e_shnum));
	int reladatasect = reladataindex > 0 ? ntohl(sect[reladataindex].sh_info) : -1;

	int relarodataindex = findnamedsect(".rela.rodata", sect, ntohs(eh.e_shnum));
	int relarodatasect = relarodataindex > 0 ? ntohl(sect[relarodataindex].sh_info) : -1;

  /* writing PH */
  sprintf(buffer, "%s.r", argv[inputindex]);
  ph_fd = open(buffer, O_RDWR | O_TRUNC | O_CREAT, 0666);
  if (ph_fd < 0)
  {
    fprintf(stderr, "%s: failed to create: %s\n", buffer, strerror(errno));
    return(1);
  }

	// setup header type
	ph.magic[0] = 0x05;
	ph.source = htons(2);
	ph.configuration = htons(4);
	write(ph_fd, &ph, sizeof(ph));

	// copy text section
	if (textindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "text section at %08llx\n", crp);
		lseek(fd, ntohl(sect[textindex].sh_offset), SEEK_SET);
		i = ntohl(sect[textindex].sh_size);
		datacpy(ph_fd, fd, i);
	}
	
	// copy data & rodata section
	if (dataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "data section at %08llx\n", crp);
		lseek(fd, ntohl(sect[dataindex].sh_offset), SEEK_SET);
		i = ntohl(sect[dataindex].sh_size);
		datacpy(ph_fd, fd, i);
	}
	if (rodataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rodata section at %08llx\n", crp);
		lseek(fd, ntohl(sect[rodataindex].sh_offset), SEEK_SET);
		i = ntohl(sect[rodataindex].sh_size);
		datacpy(ph_fd, fd, i);
	}
	
	// we need all symbols to index into
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	Elf32_Sym *symbols = (Elf32_Sym *)malloc(ntohl(sect[symindex].sh_size));
	read(fd, symbols, ntohl(sect[symindex].sh_size));
	
	// reloc
	len = 0;
	if (relatextindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rela.text section at %08llx\n", crp);

		// assert(ntohl(sect[relatextindex].sh_link) == symindex);
		
		lseek(fd, ntohl(sect[relatextindex].sh_offset), SEEK_SET);
		Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[relatextindex].sh_size));
		read(fd, rarray, ntohl(sect[relatextindex].sh_size));
		n = ntohl(sect[relatextindex].sh_size) / ntohl(sect[relatextindex].sh_entsize);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, ntohl(sect[dataindex].sh_size));
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);

		len += emitreloc(ph_fd, rarray, n, symbols, textindex, dataindex, rodataindex, bssindex, &ph, relatextsect);
		free(rarray);
	}
	
	if (reladataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rela.data section at %08llx\n", crp);
		lseek(fd, ntohl(sect[reladataindex].sh_offset), SEEK_SET);

		//	assert(ntohl(sect[reladataindex].sh_link) == symindex);

		Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[reladataindex].sh_size));
		read(fd, rarray, ntohl(sect[reladataindex].sh_size));
		n = ntohl(sect[reladataindex].sh_size) / ntohl(sect[reladataindex].sh_entsize);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, ntohl(sect[dataindex].sh_size));
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);

		len += emitreloc(ph_fd, rarray, n, symbols, textindex, dataindex, rodataindex, bssindex, &ph, reladatasect);
		free(rarray);
	}

	if (relarodataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rela.rodata section at %08llx\n", crp);
		lseek(fd, ntohl(sect[relarodataindex].sh_offset), SEEK_SET);

		//	assert(ntohl(sect[relarodataindex].sh_link) == symindex);

		Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[relarodataindex].sh_size));
		read(fd, rarray, ntohl(sect[relarodataindex].sh_size));
		n = ntohl(sect[relarodataindex].sh_size) / ntohl(sect[relarodataindex].sh_entsize);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, ntohl(sect[dataindex].sh_size));
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);

		len += emitreloc(ph_fd, rarray, n, symbols, textindex, dataindex, rodataindex, bssindex, &ph, relarodatasect);
		free(rarray);
		
	}
	
	ph.relocsize = htonl(len);
	
	// symbols
	crp = lseek(ph_fd, 0, SEEK_CUR);
	if (verbose) fprintf(stderr, "Symbols section at %08llx\n", crp);
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	len = 0;
	for(i=0; i<htonl(ph.symbolsize); i++)
	{
		Elf32_Sym elfsymbol;
		symbolheader sym;
		
		elfsymbol = symbols[i];
		
			char *symbolname = strtab + ntohl(elfsymbol.st_name);
			sym.len = ntohs(strlen(symbolname));
		
		// only interested in GLOBAL symbols
		int t = ELF32_ST_TYPE(elfsymbol.st_info);
		if (t == STT_FILE)
		{
			// TODO: we could use this as name section contents rather than .comment
		}
		
		int b = ELF32_ST_BIND(elfsymbol.st_info);

		if (t == STT_FUNC || t == STT_OBJECT || t == STT_COMMON)
		{
			if (!exportlocals && b != STB_GLOBAL )
			{
				if (verbose && symbolname[0]) fprintf(stderr,"%32s *** SKIPPED\n", symbolname);
				continue;
			}

			sym.kind = 0;
			sym.segment = SEGABS;
			
			sym.offset = htonl(ntohl(elfsymbol.st_value));
			if (ntohs(elfsymbol.st_shndx) == textindex)
			{
				sym.segment = htons(SEGTEXT);
				typename = "TEXT";
			}
			else
			if (ntohs(elfsymbol.st_shndx) == dataindex || ntohs(elfsymbol.st_shndx) == rodataindex)
			{
				sym.segment = htons(SEGDATA);
				typename = "DATA";
			}
			else
			if (ntohs(elfsymbol.st_shndx) == bssindex)
			{
				sym.segment = htons(SEGBSS);
				typename = "BSS";
			}
			else
			if (ntohs(elfsymbol.st_shndx) == commentindex)
			{
				typename = "COMMENT";
			}
			else
			{
				typename = "UNDEF";
			}

			if (verbose) fprintf(stderr,"%32s %6s %08x\n", symbolname, typename, ntohl(sym.offset));
			
			// write sym + string
			write(ph_fd, &sym, sizeof(sym));
			write(ph_fd, symbolname, ntohs(sym.len));
			len += sizeof(sym) + ntohs(sym.len);
		}
		
	}
	ph.symbolsize = htonl(len);
	free(symbols);
	
	// lastly, any comments
	crp = lseek(ph_fd, 0, SEEK_CUR);
	if (verbose) fprintf(stderr, "comment section at %08llx\n", crp);
	lseek(fd, ntohl(sect[commentindex].sh_offset), SEEK_SET);
	i = ntohl(sect[commentindex].sh_size);
	datacpy(ph_fd, fd, i);
	
	// update header with final values
	lseek(ph_fd, 0, SEEK_SET);
	write(ph_fd, &ph, sizeof(ph));
	close(ph_fd);

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

