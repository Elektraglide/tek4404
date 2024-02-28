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

// fixup data section offset to account for rodata
void fixupoffset(Elf32_Rela *rarray, int n, Elf32_Sym *symbols, int dataindex, int rodataoffset)
{
	int i;
	
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
				if (symindex == dataindex)
				{
					rarray[i].r_offset = htonl(ntohl(rarray[i].r_offset) + rodataoffset);
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
int fd, ph_fd;
int len,n,i = 0;
PH ph = {};
Elf32_Ehdr eh;
char buffer[1024];
char *typename;

  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
  {
      fprintf(stderr, "%s: not found\n", argv[1]);
      return(1);
  }

	// explore ELF
	read(fd, &eh, sizeof(Elf32_Ehdr));

  fprintf(stderr, "executable:  %s \n", strrchr(argv[1],'/'));
	if (ntohs(eh.e_machine) != EM_68K)
	{
      fprintf(stderr, "%s: not m68k\n", argv[1]);
      return(1);
	}
	if (ntohs(eh.e_type) != ET_REL)
	{
      fprintf(stderr, "%s: not relocatable file\n", argv[1]);
      return(1);
	}
#if 0	// FIXME: not understanding this I flag..
	if (ntohl(eh.e_flags) != EF_CPU32)
	{
      fprintf(stderr, "%s: not Direct 32 bit \n", argv[1]);
      return(1);
	}
#endif

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
	// ensure even length
	if (n & 1)
	{
		n++;
		sect[dataindex].sh_size = htonl(ntohl(sect[dataindex].sh_size) + 1);
	}
		
	int rodataindex = findnamedsect(".rodata", sect, ntohs(eh.e_shnum));
	if (rodataindex > 0)
		n += ntohl(sect[rodataindex].sh_size);
	// ensure even length
	if (n & 1)
	{
		n++;
		sect[rodataindex].sh_size = htonl(ntohl(sect[rodataindex].sh_size) + 1);
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
	
	
	int relaindex = findnamedsect(".rela.text", sect, ntohs(eh.e_shnum));
	ph.relocsize = htonl(ntohl(sect[relaindex].sh_size) / ntohl(sect[relaindex].sh_entsize));		// NB numentries for now;
	int relasecttion = ntohs(sect[relaindex].sh_info);


  /* writing PH */
  sprintf(buffer, "%s.r", argv[1]);
  ph_fd = open(buffer, O_RDWR | O_TRUNC| O_CREAT, 0666);
  if (ph_fd < 0)
  {
    fprintf(stderr, "%s: failed to create: %s\n", buffer, strerror(errno));
    return(1);
  }

	// setup header type
	ph.magic[0] = 0x05;
	write(ph_fd, &ph, sizeof(ph));

	// copy text section
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "text section at %08lx\n", n);
	lseek(fd, ntohl(sect[textindex].sh_offset), SEEK_SET);
	i = ntohl(sect[textindex].sh_size);
	datacpy(ph_fd, fd, i);
	
	// copy data & rodata section
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "rodata section at %08lx\n", n);
	lseek(fd, ntohl(sect[rodataindex].sh_offset), SEEK_SET);
	i = ntohl(sect[rodataindex].sh_size);
	datacpy(ph_fd, fd, i);
	
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "data section at %08lx\n", n);
	lseek(fd, ntohl(sect[dataindex].sh_offset), SEEK_SET);
	i = ntohl(sect[dataindex].sh_size);
	datacpy(ph_fd, fd, i);

	// we need all symbols to index into
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	Elf32_Sym *symbols = (Elf32_Sym *)malloc(ntohl(sect[symindex].sh_size));
	read(fd, symbols, ntohl(sect[symindex].sh_size));
	
	// reloc
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "Reloc section at %08lx\n", n);
	lseek(fd, ntohl(sect[relaindex].sh_offset), SEEK_SET);
	// Uniflex expects relocation records in sorted order
	Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[relaindex].sh_size));
	read(fd, rarray, ntohl(sect[relaindex].sh_size));
	fixupoffset(rarray, htonl(ph.relocsize), symbols, dataindex, ntohl(sect[rodataindex].sh_size));
	qsort(rarray, htonl(ph.relocsize), sizeof(Elf32_Rela), relacompare);

	len = 0;
	for(i=0; i<htonl(ph.relocsize); i++)
	{
		Elf32_Rela elfreloc;
		relocheader rel;
		
		elfreloc = rarray[i];
		int symindex = ELF32_R_SYM(ntohl(elfreloc.r_info));
		int rtype = ELF32_R_TYPE(ntohl(elfreloc.r_info));

		rel.offset = htonl(ntohl(elfreloc.r_offset));

		// lookup symbol
		int symtype = ELF32_ST_TYPE(symbols[symindex].st_info);
		
		char *symbolname = strtab + ntohl(symbols[symindex].st_name);
		n = strlen(symbolname);
		rel.len = ntohs(n);

		if (rtype == R_68K_NONE)
		{
			rel.kind = 0;
		}
		else
		if (rtype == R_68K_32)
		{
			rel.kind = htons(0x8001);

			if (symtype == STT_SECTION)
			{
				if (symindex == bssindex)
				{
					symbolname = "BSS";
					rel.kind = htons(0x00c1);
				}
				if (symindex == rodataindex)
				{
					symbolname = "RODATA";
					rel.kind = htons(0x0081);
				}
				if (symindex == dataindex)
				{
					symbolname = "DATA";
					rel.kind = htons(0x0081);
				}

				// this is horribly inefficient...
				// Uniflex does not have addend reloc records, so we need to embed its value like a Elf32_Rel
				// seek to offset in text section and apply r_addend (why text section?)
				if (elfreloc.r_addend != 0)
				{
					int crp = lseek(ph_fd, 0, SEEK_CUR);
						lseek(ph_fd, sizeof(ph) + ntohl(elfreloc.r_offset), SEEK_SET);
						int v = htonl(ntohl(elfreloc.r_addend));
						write(ph_fd, &v, sizeof(v));
					lseek(ph_fd, crp, SEEK_SET);
				}
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
		
		fprintf(stderr, "%32s %3d %08x addend(%08lx)\n", symbolname, rtype, ntohl(rel.offset), ntohl(elfreloc.r_addend) );
		
		write(ph_fd, &rel, sizeof(rel));
		if (n > 0)
			write(ph_fd, symbolname, n);
		len += sizeof(rel);
		len += n;

	}
	ph.relocsize = htonl(len);
	free(rarray);
	
	// symbols
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "Symbols section at %08lx\n", n);
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	len = 0;
	for(i=0; i<htonl(ph.symbolsize); i++)
	{
		Elf32_Sym elfsymbol;
		symbolheader sym;
		
		elfsymbol = symbols[i];
		int b = ELF32_ST_BIND(elfsymbol.st_info);
		int t = ELF32_ST_TYPE(elfsymbol.st_info);
		if (t == STT_FUNC || t == STT_OBJECT || t == STT_COMMON)
		{
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

			char *symbolname = strtab + ntohl(elfsymbol.st_name);
			sym.len = ntohs(strlen(symbolname));
		
			fprintf(stderr,"%32s %6s %08x\n", symbolname, typename, ntohl(sym.offset));
			
			// write sym + string
			write(ph_fd, &sym, sizeof(sym));
			write(ph_fd, symbolname, ntohs(sym.len));
			len += sizeof(sym) + ntohs(sym.len);
		}
		
	}
	ph.symbolsize = htonl(len);
	free(symbols);
	
	// lastly, any comments
	n = lseek(ph_fd, 0, SEEK_CUR);
	fprintf(stderr, "comment section at %08lx\n", n);
	lseek(fd, ntohl(sect[commentindex].sh_offset), SEEK_SET);
	i = ntohl(sect[commentindex].sh_size);
	datacpy(ph_fd, fd, i);
	
	// update header with final values
	lseek(ph_fd, 0, SEEK_SET);
	write(ph_fd, &ph, sizeof(ph));
	close(ph_fd);

	fprintf(stderr, "-----------------\n");
  fprintf(stderr, "textstart = %08x\n", ntohl(ph.textstart));
  fprintf(stderr, "textsize = %08x\n", ntohl(ph.textsize));
  fprintf(stderr, "datastart = %08x\n", ntohl(ph.datastart));
  fprintf(stderr, "datasize = %08x\n", ntohl(ph.datasize));
  fprintf(stderr, "relocsize = %08x\n", ntohl(ph.relocsize));
  fprintf(stderr, "symbolsize = %08x\n", ntohl(ph.symbolsize));
  fprintf(stderr, "commentsize = %08x\n", ntohs(ph.commentsize));


  return 0;
}

