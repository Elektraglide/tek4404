#include <stdio.h>
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

Elf32_Sym  symbols[65536];

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


	int textindex = findnamedsect(".text", sect, ntohs(eh.e_shnum));
	ph.textstart = htonl(adddata(ntohl(sect[textindex].sh_size)));
	ph.textsize = sect[textindex].sh_size;
	
	// rodata gets appended to data
	int dataindex = findnamedsect(".data", sect, ntohs(eh.e_shnum));
	int rodataindex = findnamedsect(".rodata", sect, ntohs(eh.e_shnum));
	ph.datastart = htonl(adddata(ntohl(sect[dataindex].sh_size)+ntohl(sect[rodataindex].sh_size)));
	ph.datasize = sect[dataindex].sh_size + sect[rodataindex].sh_size;

	int commentindex = findnamedsect(".comment", sect, ntohs(eh.e_shnum));
	ph.commentsize = htons(ntohl(sect[commentindex].sh_size));

	int bssindex = findnamedsect(".bss", sect, ntohs(eh.e_shnum));
	ph.bsssize = sect[bssindex].sh_size;
	
	int symindex = findnamedsect(".symtab", sect, ntohs(eh.e_shnum));
	ph.symbolsize = htonl(ntohl(sect[symindex].sh_size) / ntohl(sect[symindex].sh_entsize));		// NB numentries for now
	
	
	int relaindex = findnamedsect(".rela.text", sect, ntohs(eh.e_shnum));
	ph.relocsize = htonl(ntohl(sect[relaindex].sh_size) / ntohl(sect[relaindex].sh_entsize));		// NB numentries for now;


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
	lseek(fd, ntohl(sect[textindex].sh_offset), SEEK_SET);
	i = ntohl(sect[textindex].sh_size);
	while(i > 0)
	{
		n = read(fd, buffer, min(i, sizeof(buffer)) );
		write(ph_fd, buffer, n);
		i -= n;
	}
	
	// copy data & rodata section
	lseek(fd, ntohl(sect[dataindex].sh_offset), SEEK_SET);
	i = ntohl(sect[dataindex].sh_size);
	while(i > 0)
	{
		n = read(fd, buffer, min(i, sizeof(buffer)) );
		write(ph_fd, buffer, n);
		i -= n;
	}
	lseek(fd, ntohl(sect[rodataindex].sh_offset), SEEK_SET);
	i = ntohl(sect[rodataindex].sh_size);
	while(i > 0)
	{
		n = read(fd, buffer, min(i, sizeof(buffer)) );
		write(ph_fd, buffer, n);
		i -= n;
	}
	
	// we need symbols to index into
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	i = htonl(ph.symbolsize);
	n = read(fd, symbols, i * sizeof(Elf32_Sym));
	
	// reloc
	lseek(fd, ntohl(sect[relaindex].sh_offset), SEEK_SET);
	i = htonl(ph.relocsize);
	len = 0;
	while(i-- > 0)
	{
		Elf32_Rel elfreloc;
		relocheader rel;
		
		read(fd, &elfreloc, sizeof(elfreloc));
		int s = ELF32_R_SYM(ntohl(elfreloc.r_info));
		int t = ELF32_R_TYPE(ntohl(elfreloc.r_info));
		if (t == STT_FUNC || t == STT_OBJECT || t == STT_SECTION)
		{
			rel.offset = htonl(ntohl(elfreloc.r_offset));
			rel.kind = htons(0x8001);

			// lookup symbol and embed it
			char *symbolname = strtab + ntohl(symbols[s].st_name);
			n = strlen(symbolname);
			rel.len = ntohs(n);

			fprintf(stderr, "reloc %08x %s\n", ntohl(elfreloc.r_offset), symbolname);
			
			write(ph_fd, &rel, sizeof(rel));
			write(ph_fd, symbolname, n);
			len += sizeof(rel);
			len += n;
		}
	}
	ph.relocsize = htonl(len);
				
	// symbols
	lseek(fd, ntohl(sect[symindex].sh_offset), SEEK_SET);
	i = htonl(ph.symbolsize);
	len = 0;
	while(i-- > 0)
	{
		Elf32_Sym elfsymbol;
		symbolheader sym;
		
		read(fd, &elfsymbol, sizeof(elfsymbol));
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
	
		
	// lastly, any comments
	lseek(fd, ntohl(sect[commentindex].sh_offset), SEEK_SET);
	i = ntohl(sect[commentindex].sh_size);
	while(i > 0)
	{
		n = read(fd, buffer, min(i, sizeof(buffer)) );
		write(ph_fd, buffer, n);
		i -= n;
	}
	
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

