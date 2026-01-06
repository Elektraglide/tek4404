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
  
#include "elf.h"
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define O_APPEND        0x00000008      /* set append mode */
#define O_CREAT         0x00000200      /* create if nonexistant */
#define O_TRUNC         0x00000400      /* truncate to zero length */

#define min(A,B)  (A < B ? A : B)

char outputname[1024];

int verbose = 0;

char strtab[65536] = "\0";

/* ongoing string lump offsets */
char shstrtab[65536] = "\0";

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

char *relocrecords;
int relocoffsetcompare(const void *a, const void *b)
{
relocheader *r1 = (relocheader *)(relocrecords + *(int *)a);
relocheader *r2 = (relocheader *)(relocrecords + *(int *)b);

int i = ntohl(r1->offset);
int j = ntohl(r2->offset);

	return (i - j);
}

int istext(Elf32_Shdr *sect)
{
	if (sect->sh_type != htonl(SHT_PROGBITS))
		return 0;
		
	return (sect->sh_flags & htonl(SHF_EXECINSTR));
}
int isdata(Elf32_Shdr *sect)
{
	if (sect->sh_type != htonl(SHT_PROGBITS))
		return 0;

	return !(sect->sh_flags & htonl(SHF_EXECINSTR)) && (sect->sh_flags & htonl(SHF_ALLOC));
}
int isrodata(Elf32_Shdr *sect)
{
	return isdata(sect) && !(sect->sh_flags & htonl(SHF_WRITE));
}
int isbss(Elf32_Shdr *sect)
{

	return (sect->sh_type == htonl(SHT_NOBITS));
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
		char *symbolname = strtab + ntohl(symbols[symindex].st_name);
		
		if (rtype == R_68K_32)
		{
			targetsection = ntohs(symbols[symindex].st_shndx);
			if (symtype == STT_SECTION)
			{

				// rodata offsets need to be adjusted because they are relative to data (we dont have rodata in Uniflex)
				if (targetsection == rodataindex)
				{
					rarray[i].r_addend = htonl(ntohl(rarray[i].r_addend) + dataoffset);
					if (verbose) fprintf(stderr, "%08x: fixing rodata(%d) addend\n", ntohl(rarray[i].r_offset),rodataindex);
				}
			}
			else
			if (symtype != STT_NOTYPE)
			{
				fprintf(stderr, "symtype: %d, %s\n", symtype, symbolname);
			}
		}
		else
		{
			fprintf(stderr, "error %d\n", rtype);
		}
	}
}

int emitreloc(int ph_fd, Elf32_Rela *rarray, int numrecords, Elf32_Sym *symbols, Elf32_Shdr *sect, int relocsection, int sectionbase)
{
	int i;
	int len = 0;

	if (relocsection < 0)
	{
		return 0;
	}

	// map to Uniflex section code
	int instsegment = 0;
	if (istext(&sect[relocsection])) instsegment = 1;
	if (isdata(&sect[relocsection])) instsegment = 2;
	if (instsegment == 0)
	{
		fprintf(stderr,"error: unknown segment\n");
	}
	
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
		int namelen = (int)strlen(symbolname);
		rel.len = ntohs(namelen);

		if (rtype == R_68K_NONE)
		{
			rel.kind = 0;
		}
		else
		if (rtype == R_68K_32)
		{
			// assume external
			rel.kind = htons(0x8000 + instsegment);

			if (symtype == STT_SECTION)
			{
				int targetsection = ntohs(symbols[rsymindex].st_shndx);
				
				if (isbss(&sect[targetsection]))
				{
					symbolname = "BSS";
					rel.kind = htons(0x00c0 + instsegment);
				}
				else
				if (isdata(&sect[targetsection]))
				{
					symbolname = "DATA";
					rel.kind = htons(0x0080 + instsegment);
				}
				else
				if (istext(&sect[targetsection]))
				{
					symbolname = "TEXT";
					rel.kind = htons(0x0040 + instsegment);
				}
				else
				{
					fprintf(stderr,"failed to classify section %d\n", targetsection);
				}
				
			}
			else
			if (symtype != STT_NOTYPE)
			{
				fprintf(stderr, "error: %d\n", symtype);
			}
			
			// this is horribly inefficient...
			// Uniflex does not have addend reloc records, so we need to embed its value like a Elf32_Rel
			if (elfreloc.r_addend != 0)
			{
				off_t crp = lseek(ph_fd, 0, SEEK_CUR);
					lseek(ph_fd, sizeof(PH) + sectionbase + ntohl(elfreloc.r_offset), SEEK_SET);
					int S;
					read(ph_fd, &S, sizeof(S));
					lseek(ph_fd, sizeof(PH) + sectionbase + ntohl(elfreloc.r_offset), SEEK_SET);

					if (S != 0)
					{
						// means the offset is wrong as we expect 0x0000000 ready to be filled in..
						fprintf(stderr, "%s: applying addend with non-zero S: %08x\n", basename(outputname), ntohl(S));
					}

					int A = htonl(ntohl(elfreloc.r_addend) + S);
			//		A = htonl(0xDEADDEAD);
					write(ph_fd, &A, sizeof(A));
				lseek(ph_fd, crp, SEEK_SET);
			}
		}
		else
		{
			// FIXME: R_68K_PC32 etc
			fprintf(stderr, "error: %d\n", rtype);
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

	// output filename
  sprintf(outputname, "%s.r", argv[inputindex]);

	// explore ELF
	read(fd, &eh, sizeof(Elf32_Ehdr));

  if (verbose) fprintf(stderr, "\nconverting:  %s \n", basename(argv[inputindex]));
	if (ntohs(eh.e_machine) != EM_68K)
	{
      fprintf(stderr, "not m68k: %s\n", argv[inputindex]);
      return(1);
	}
	if (ntohs(eh.e_type) != ET_REL)
	{
      fprintf(stderr, "not relocatable file: %s\n", argv[inputindex]);
      return(1);
	}

	// get sections
  Elf32_Shdr sect[16];
	lseek(fd, ntohl(eh.e_shoff), SEEK_SET);
	read(fd, sect, ntohs(eh.e_shnum) * sizeof(Elf32_Shdr));

	// get strings
	int strindex = ntohs(eh.e_shstrndx);
	lseek(fd, ntohl(sect[strindex].sh_offset), SEEK_SET);
	read(fd, shstrtab, ntohl(sect[strindex].sh_size) );

	// summary of sections
	for(i=1; i<ntohs(eh.e_shnum); i++)
	{
		char *typename = "";
		if (istext(&sect[i]))
			typename = "TEXT";
		else
		if (isrodata(&sect[i]))
			typename = "RODATA";
		else
		if (isdata(&sect[i]))
			typename = "DATA";
		else
		if (isbss(&sect[i]))
			typename = "BSS";
		else
			typename = "";
			
		if (verbose) fprintf(stderr, "%2d: %14s %8.8x %6s\n", i, shstrtab + ntohl(sect[i].sh_name), ntohl(sect[i].sh_size), typename);
	}

	int stindex = findnamedsect(".strtab", sect, ntohs(eh.e_shnum));
	lseek(fd, ntohl(sect[stindex].sh_offset), SEEK_SET);
	read(fd, strtab, ntohl(sect[stindex].sh_size) );

	n = 0;
	int textindex = findnamedsect(".text", sect, ntohs(eh.e_shnum));
	if (textindex > 0)
		n += ntohl(sect[textindex].sh_size);
	ph.textsize = htonl(n);


	// rodata gets appended to data  TODO: search for any section with PROGBITS and name like 'data'
	n = 0;
	int dataindex = findnamedsect(".data", sect, ntohs(eh.e_shnum));
	if (dataindex > 0)
	{
		n += ntohl(sect[dataindex].sh_size);
		// ensure even length
		if (n & 1)
		{
			n++;
			sect[dataindex].sh_size = htonl(ntohl(sect[dataindex].sh_size) + 1);
		}
	}
	int rodataindex = findnamedsect(".rodata", sect, ntohs(eh.e_shnum));
	if (rodataindex > 0)
	{
		n += ntohl(sect[rodataindex].sh_size);
		// ensure even length
		if (n & 1)
		{
			n++;
			sect[rodataindex].sh_size = htonl(ntohl(sect[rodataindex].sh_size) + 1);
		}
	}

	// NB what about .rodata.str.[2,3,4]
	int	rodatastringindex = findnamedsect(".rodata.str1.1", sect, ntohs(eh.e_shnum));
	if (rodatastringindex > 0)
	{
		n += ntohl(sect[rodatastringindex].sh_size);
		// ensure even length
		if (n & 1)
		{
			n++;
			sect[rodatastringindex].sh_size = htonl(ntohl(sect[rodatastringindex].sh_size) + 1);
		}
	}

	ph.datasize = htonl(n);

	int commentindex = findnamedsect(".comment", sect, ntohs(eh.e_shnum));
	ph.commentsize = htons(ntohl(sect[commentindex].sh_size));
	ph.namesize = htons(strlen(basename(outputname)));

	n = 0;
	int bssindex = findnamedsect(".bss", sect, ntohs(eh.e_shnum));
	if (bssindex > 0)
		n += ntohl(sect[bssindex].sh_size);
	ph.bsssize = htonl(n);
	
	int symindex = findnamedsect(".symtab", sect, ntohs(eh.e_shnum));
	ph.symbolsize = htonl(ntohl(sect[symindex].sh_size) / ntohl(sect[symindex].sh_entsize));		// NB numentries for now
	
	n = 0;
	int relatextindex = findnamedsect(".rela.text", sect, ntohs(eh.e_shnum));
	int relatextsect = relatextindex > 0 ? ntohl(sect[relatextindex].sh_info) : -1;
	if (relatextindex > 0)
		n += ntohl(sect[relatextindex].sh_size) / ntohl(sect[relatextindex].sh_entsize);
		
	int reladataindex = findnamedsect(".rela.data", sect, ntohs(eh.e_shnum));
	int reladatasect = reladataindex > 0 ? ntohl(sect[reladataindex].sh_info) : -1;
	if (reladataindex > 0)
		n += ntohl(sect[reladataindex].sh_size) / ntohl(sect[reladataindex].sh_entsize);

	int relarodataindex = findnamedsect(".rela.rodata", sect, ntohs(eh.e_shnum));
	int relarodatasect = relarodataindex > 0 ? ntohl(sect[relarodataindex].sh_info) : -1;
	if (relarodataindex > 0)
		n += ntohl(sect[relarodataindex].sh_size) / ntohl(sect[relarodataindex].sh_entsize);

	ph.relocsize = htonl(n);		// NB numentries for now;

  /* writing PH */
  ph_fd = open(outputname, O_RDWR | O_TRUNC | O_CREAT, 0666);
  if (ph_fd < 0)
  {
    fprintf(stderr, "%s: failed to create: %s\n", outputname, strerror(errno));
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
	
	// copy data & rodata & rodatastring sections
	if (dataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "data section at %08llx\n", crp);
		lseek(fd, ntohl(sect[dataindex].sh_offset), SEEK_SET);
		i = ntohl(sect[dataindex].sh_size);
		datacpy(ph_fd, fd, i);
	}
	if (rodatastringindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rodata str section at %08llx\n", crp);
		lseek(fd, ntohl(sect[rodatastringindex].sh_offset), SEEK_SET);
		i = ntohl(sect[rodatastringindex].sh_size);
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
	int relocstart = lseek(ph_fd, 0, SEEK_CUR);
	int reloccount = 0;
	
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
		
		int dataoffset = 0;
		if (dataindex > 0) dataoffset += ntohl(sect[dataindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodatastringindex, dataoffset);
		if (rodatastringindex > 0) dataoffset += ntohl(sect[rodatastringindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, dataoffset);
		
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);
		reloccount += n;
		
		len += emitreloc(ph_fd, rarray, n, symbols, sect, textindex, 0);
		free(rarray);
	}

	if (reladataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rela.data section at %08llx\n", crp);

		// assert(ntohl(sect[reladataindex].sh_link) == symindex);
		
		lseek(fd, ntohl(sect[reladataindex].sh_offset), SEEK_SET);
		Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[reladataindex].sh_size));
		read(fd, rarray, ntohl(sect[reladataindex].sh_size));
		n = ntohl(sect[reladataindex].sh_size) / ntohl(sect[reladataindex].sh_entsize);
		
		int dataoffset = 0;
		if (dataindex > 0) dataoffset += ntohl(sect[dataindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodatastringindex, dataoffset);
		if (rodatastringindex > 0) dataoffset += ntohl(sect[rodatastringindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, dataoffset);
		
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);
		reloccount += n;

		dataoffset = 0;
		if (textindex > 0) dataoffset += ntohl(sect[textindex].sh_size);
		len += emitreloc(ph_fd, rarray, n, symbols, sect, dataindex, dataoffset);
		free(rarray);
	}

	if (relarodataindex > 0)
	{
		crp = lseek(ph_fd, 0, SEEK_CUR);
		if (verbose) fprintf(stderr, "rela.rodata section at %08llx\n", crp);

		// assert(ntohl(sect[relarodataindex].sh_link) == symindex);
		
		lseek(fd, ntohl(sect[relarodataindex].sh_offset), SEEK_SET);
		Elf32_Rela *rarray = (Elf32_Rela *)malloc(ntohl(sect[relarodataindex].sh_size));
		read(fd, rarray, ntohl(sect[relarodataindex].sh_size));
		n = ntohl(sect[relarodataindex].sh_size) / ntohl(sect[relarodataindex].sh_entsize);
		
		int dataoffset = 0;
		if (dataindex > 0) dataoffset += ntohl(sect[dataindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodatastringindex, dataoffset);
		if (rodatastringindex > 0) dataoffset += ntohl(sect[rodatastringindex].sh_size);
		fixup_rodata_offset(rarray, n, symbols, rodataindex, dataoffset);
		
		qsort(rarray, n, sizeof(Elf32_Rela), relacompare);
		reloccount += n;

		dataoffset = 0;
		if (textindex > 0) dataoffset += ntohl(sect[textindex].sh_size);
		if (dataindex > 0) dataoffset += ntohl(sect[dataindex].sh_size);
		len += emitreloc(ph_fd, rarray, n, symbols, sect, rodataindex, dataoffset);

		free(rarray);
	}

	ph.relocsize = htonl(len);

	// reloc records need to be in sorted order but they are variable length records
	crp = lseek(ph_fd, 0, SEEK_CUR);
	relocrecords = (char *)malloc(len);
	lseek(ph_fd, relocstart, SEEK_SET);
	read(ph_fd, relocrecords, len);

	// build an index into variable length records array
	int *relocrecordindex = (int *)malloc(sizeof(int) * reloccount);
	char *ptr = relocrecords;
	for (i=0; i<reloccount; i++)
	{
		relocheader *rel = (relocheader *)ptr;
	
		relocrecordindex[i] = (int)(ptr - relocrecords);
		
		ptr = ptr + sizeof(relocheader) + ntohs(rel->len);
	}
	qsort(relocrecordindex, reloccount, sizeof(int), relocoffsetcompare);
	
	// rewrite reloc records in ascending offset order
	lseek(ph_fd, relocstart, SEEK_SET);
	for (i=0; i<reloccount; i++)
	{
		relocheader *rel = (relocheader *)(relocrecords + relocrecordindex[i]);
		//if (verbose) fprintf(stderr, "relocation %08x\n", ntohl(rel->offset));
		
		write(ph_fd, rel, sizeof(relocheader));
		if (rel->len)
			write(ph_fd, (rel+1), ntohs(rel->len));
	}
	
	free(relocrecordindex);
	free(relocrecords);

	
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

		int targetsection = ntohs(elfsymbol.st_shndx);

		if (targetsection)
		if (t == STT_NOTYPE || t == STT_FUNC || t == STT_OBJECT || t == STT_COMMON)
		{
			sym.kind = 0;
			sym.segment = htons(SEGABS);
			
			sym.offset = htonl(ntohl(elfsymbol.st_value));

			if (!exportlocals && (b != STB_GLOBAL && t != STT_NOTYPE))
			{
				if (verbose && symbolname[0]) fprintf(stderr,"%32s %08x *** SKIPPED\n", symbolname, ntohl(sym.offset));
				continue;
			}

			if (istext(&sect[targetsection]))
			{
				sym.segment = htons(SEGTEXT);
				typename = "TEXT";
			}
			else
			if (isdata(&sect[targetsection]))
			{
				sym.segment = htons(SEGDATA);
				typename = "DATA";
				if (isrodata(&sect[targetsection]))
				{
					// rodata is after data; how do we know rodata or rodata.str1?
					sym.offset = htonl(ntohl(sym.offset) + ntohl(sect[dataindex].sh_size));
				}
			}
			else
			if (isbss(&sect[targetsection]))
			{
				sym.segment = htons(SEGBSS);
				typename = "BSS";
			}
			else
			if (targetsection == commentindex)
			{
				typename = "COMMENT";
			}
			else
			{
				typename = "UNDEF";
				continue;
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

	// any comments
	crp = lseek(ph_fd, 0, SEEK_CUR);
	if (verbose) fprintf(stderr, "comment section at %08llx\n", crp);
	lseek(fd, ntohl(sect[commentindex].sh_offset), SEEK_SET);
	i = ntohl(sect[commentindex].sh_size);
	datacpy(ph_fd, fd, i);
	
	// name  (max 11 chars..)
	write(ph_fd, basename(outputname), strlen(basename(outputname)));

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
		fprintf(stderr, "commentsize = %08x\n", ntohs(ph.commentsize));
		fprintf(stderr, "namesize = %08x\n", ntohs(ph.namesize));
	}

  return 0;
}

