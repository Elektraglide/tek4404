#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#include "elf.h"
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define O_APPEND        0x00000008      /* set append mode */
#define O_CREAT         0x00000200      /* create if nonexistant */
#define O_TRUNC         0x00000400      /* truncate to zero length */

#define min(A,B)  (A < B ? A : B)

extern int open(char *filename, int mode);
extern int read(int fd, void *buff, int len);
extern int write(int fd, void *buff, int len);
extern int lseek(int fd, int offset, int whence);
extern void close(int fd);

#pragma pack(1)

typedef struct {

  short kind;
  int offset;
  short segment;

  short len;
  
} symbolheader;


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

/* ongoing symbols */
Elf32_Sym symbols[4096];
int numsymbols = 0;

/* ongoing string lump offsets */
char globalstrings[4096] = "\0";
int stringoffset = 1;
int addstring(char *label)
{
  int startpos = stringoffset;
  memcpy(globalstrings+stringoffset, label, strlen(label)+1);
  stringoffset += strlen(label)+1;
  return startpos;
}

/* ongoing data lump offsets */
int dataoffset = 0;
int adddata(int len)
{
  int startpos = dataoffset;
  dataoffset += len;
  return startpos;
}

int main(int argc, char *argv[])
{
int fd, elf_fd;
int n,i = 0;
int symbolsize;
PH ph;
symbolheader sym;
char buffer[1024];
char *typename;

  /* should we seek to .data */
  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
  {
      fprintf(stderr, "%s: not found\n", argv[1]);
      return(1);
  }

  // think this should be 0x40 bytes long as offsets are from after the header
  read(fd, &ph, sizeof(PH));
  if (ntohs(ph.magic) != 0x400)
  {
    fprintf(stderr, "%s: not an executable  (%04x)\n", argv[1], ph.magic);
    return(1);
  }

  fprintf(stderr, "executable:  %s \n", strrchr(argv[1],'/'));

  fprintf(stderr, "textstart = %08x\n", ntohl(ph.textstart));
  fprintf(stderr, "textsize = %08x\n", ntohl(ph.textsize));
  fprintf(stderr, "datastart = %08x\n", ntohl(ph.datastart));
  fprintf(stderr, "datasize = %08x\n", ntohl(ph.datasize));
  fprintf(stderr, "relocsize = %08x\n", ntohs(ph.relocsize));
  fprintf(stderr, "minpage = %d maxpage = %d stack = %d\n", ntohs(ph.minpage),ntohs(ph.maxpage),ntohs(ph.stacksize));

  /* datastart offset from header */
  i =  ntohl(ph.datastart) + 0x40;
  fprintf(stderr, "datastart = %08x\n", i);

  /* writing ELF */
  sprintf(buffer, "%s.elf", argv[1]);
  elf_fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC);
  if (elf_fd < 0)
  {
    fprintf(stderr, "%s: failed to create\n", buffer);
    return(1);
  }
  
  /* we have ELF header first followed by progsheader and sectionheaders */
  adddata(sizeof(Elf32_Ehdr));

  Elf32_Ehdr elfheader;
  elfheader.e_ident[EI_MAG0] = ELFMAG0;
  elfheader.e_ident[EI_MAG1] = ELFMAG1;
  elfheader.e_ident[EI_MAG2] = ELFMAG2;
  elfheader.e_ident[EI_MAG3] = ELFMAG3;
  elfheader.e_ident[EI_CLASS] = ELFCLASS32;
  elfheader.e_ident[EI_DATA] = ELFDATA2MSB;
  elfheader.e_ident[EI_VERSION] = EV_CURRENT;
  elfheader.e_ident[EI_OSABI] = ELFOSABI_NONE;
  elfheader.e_ident[EI_ABIVERSION] = 0;
  elfheader.e_ident[EI_PAD] = 0;
  
  elfheader.e_type = htons(ET_EXEC);
  elfheader.e_machine = htons(EM_68K);
  elfheader.e_version = htonl(EV_CURRENT);
  elfheader.e_entry = ph.xferaddress;
  elfheader.e_phoff = htonl(adddata(sizeof(Elf32_Phdr) * 2));  // [0].text, [1].data
  elfheader.e_shoff = htonl(adddata(sizeof(Elf32_Shdr) * 4));  // [0].text, [1].data, [2]strings, [3]symbols
  elfheader.e_flags = 0;
  elfheader.e_ehsize = htons(sizeof(elfheader));

  elfheader.e_phentsize = htons(sizeof(Elf32_Phdr));
  elfheader.e_phnum = htons(2);
  elfheader.e_shentsize = htons(sizeof(Elf32_Shdr));
  elfheader.e_shnum = htons(4);
  elfheader.e_shstrndx =  htons(2);
  fprintf(stderr,"wrote %d bytes (ElfHeader)\n", (int)sizeof(elfheader));
  write(elf_fd, &elfheader, sizeof(elfheader));

  // what is this for?
  Elf32_Phdr prog[2];
  prog[0].p_type = htonl(PT_LOAD);
  prog[0].p_offset = htonl(adddata(ntohl(ph.textsize)));
  prog[0].p_vaddr = ph.textstart;
  prog[0].p_paddr = ph.textstart;
  prog[0].p_filesz = ph.textsize;
  prog[0].p_memsz = ph.textsize;
  prog[0].p_flags = htonl(PF_X | PF_R);
  prog[0].p_align = htonl(4);
  
  prog[1].p_type = htonl(PT_LOAD);
  prog[1].p_offset = htonl(adddata(ntohl(ph.datasize)));
  prog[1].p_vaddr = ph.datastart;
  prog[1].p_paddr = ph.datastart;
  prog[1].p_filesz = ph.datasize;
  prog[1].p_memsz = ph.datasize;
  prog[1].p_flags = htonl(PF_W | PF_R);
  prog[1].p_align = htonl(4);
  
  fprintf(stderr,"wrote %d bytes (ProgHeader: %d %d)\n", (int)sizeof(prog), prog[0].p_offset, prog[0].p_memsz);
  write(elf_fd, prog, sizeof(prog));

  Elf32_Shdr sect[4];
  sect[0].sh_name = htonl(addstring(".text"));
  sect[1].sh_name = htonl(addstring(".data"));
  sect[2].sh_name = htonl(addstring(".strtab"));
  sect[3].sh_name = htonl(addstring(".symtab"));

  // section 0: .text
  sect[0].sh_type = htonl(SHT_PROGBITS);
  sect[0].sh_flags = htonl(SHF_ALLOC | SHF_EXECINSTR);
  sect[0].sh_addr = ph.textstart;
  sect[0].sh_offset = prog[0].p_offset;
  sect[0].sh_size = ph.textsize;
  sect[0].sh_link = 0;
  sect[0].sh_info = 0;
  sect[0].sh_addralign = 0;
  sect[0].sh_entsize = 0;

  // section 1: .data
  sect[1].sh_type = htonl(SHT_PROGBITS);
  sect[1].sh_flags = htonl(SHF_ALLOC | SHF_WRITE);
  sect[1].sh_addr = ph.datastart;
  sect[1].sh_offset = prog[1].p_offset;
  sect[1].sh_size = ph.datasize;
  sect[1].sh_link = 0;
  sect[1].sh_info = 0;
  sect[1].sh_addralign = 0;
  sect[1].sh_entsize = 0;

#if 0
  // symbol#0 is reserved
  symbols[numsymbols].st_name = 0;
  symbols[numsymbols].st_value = 0;
  symbols[numsymbols].st_size = 0;
  symbols[numsymbols].st_info = 0;
  symbols[numsymbols].st_other = 0;
  symbols[numsymbols].st_shndx = htons(SHN_UNDEF);
  numsymbols++;
#endif

  /* seek to symbols (offset from end) */
  n = lseek(fd, 0, SEEK_END);
  //fprintf(stderr, "total length = %08x\n", n);
  lseek(fd, -ntohs(ph.commentsize), SEEK_CUR);
  n = lseek(fd, -ntohl(ph.symbolsize), SEEK_CUR);
  fprintf(stderr, "symbolstart = %08X\n", n);
  lseek(fd, n, SEEK_SET);
  
  /* each symbol */
  symbolsize = ntohl(ph.symbolsize);
  fprintf(stderr, "symbolsize = %08x\n\n", symbolsize);
  for (i=0; i<symbolsize; i += sizeof(symbolheader))
  {
    sym.kind = readshort(fd);
    sym.offset = readlong(fd);
    sym.segment = readshort(fd);
    sym.len = readshort(fd);
    read(fd, buffer, sym.len);

    /* peek to see if is there more because sometimes the len field is just wrong */
    read(fd, &buffer[sym.len], 1);
    sym.len++;

    /* if we're past the symbolsize, dont so our hack */
    if (i + sym.len + sizeof(symbolheader) < symbolsize)
    {
      if (buffer[sym.len-1] != 0)
      {
        while (buffer[sym.len-1] != 0)
        {
          sym.len++;
          read(fd, &buffer[sym.len-1], 1);
        }
      }
      else
      {
        sym.len--;
      }

      /* overshot, go back one */
      n = lseek(fd, -1, SEEK_CUR);
      lseek(fd, n, SEEK_SET);
    }
    else
    {
      sym.len--;
    }

    buffer[sym.len] = '\0';
    switch(sym.segment)
    {
      case 8:
        typename = "ABS";
        break;
      case 9:
        typename = "TEXT";
        break;
      case 10:
        typename = "DATA";
        break;
      case 11:
        typename = "BSS";
        break;
    }
    printf("%32s:  %s  %08x\n", buffer, typename, sym.offset);
    
    symbols[numsymbols].st_name = htonl(addstring(buffer));
    symbols[numsymbols].st_value = htonl(sym.offset);
    symbols[numsymbols].st_size = htonl(strlen(buffer));
    symbols[numsymbols].st_info = ELF32_ST_INFO(STB_GLOBAL, sym.segment == 9 ? STT_FUNC : STT_OBJECT);
    symbols[numsymbols].st_other = STV_DEFAULT;
    symbols[numsymbols].st_shndx = sym.segment == 9 ? htons(0) : htons(1) ;
    numsymbols++;
    
    i += sym.len;
  }

  // section 2: strings
  sect[2].sh_type = htonl(SHT_STRTAB);
  sect[2].sh_flags = htonl(SHF_STRINGS);
  sect[2].sh_addr = htonl(0x8000);
  sect[2].sh_offset = htonl(adddata(stringoffset));
  sect[2].sh_size = htonl(stringoffset);
  sect[2].sh_link = 0;
  sect[2].sh_info = 0;
  sect[2].sh_addralign = 0;
  sect[2].sh_entsize = htonl(1);

  // section 3: symbols
  sect[3].sh_type = htonl(SHT_SYMTAB);
  sect[3].sh_flags = 0;
  sect[3].sh_addr = htonl(0x9000);
  sect[3].sh_offset = htonl(adddata(sizeof(Elf32_Sym) * numsymbols));
  sect[3].sh_size = htonl(sizeof(Elf32_Sym) * numsymbols);
  sect[3].sh_link = htonl(2);
  sect[3].sh_info = 0;
  sect[3].sh_addralign = 0;
  sect[3].sh_entsize = htonl(sizeof(Elf32_Sym));

  // completed section headers
  for (i=0; i<4; i++)
  {
    fprintf(stderr,"wrote %d bytes (Section%d %d %d)\n", (int)sizeof(sect[0]), i, sect[i].sh_offset, sect[i].sh_size);
    write(elf_fd, &sect[i], sizeof(sect[0]));
  }
  
  // write text
  n = lseek(elf_fd, 0, SEEK_CUR);
  lseek(fd, ntohl(ph.textstart) + 0x40, SEEK_SET);
  i = ntohl(ph.textsize);
  fprintf(stderr,"wrote %d bytes at %d (.text)\n", i, n);
  while ((i > 0) && (n = read(fd, buffer,  min(i,sizeof(buffer)) )) > 0)
  {
    write(elf_fd, buffer, n);
    i -= n;
  }

  // write data
  n = lseek(elf_fd, 0, SEEK_CUR);
  lseek(fd, ntohl(ph.datastart) + 0x40, SEEK_SET);
  i = ntohl(ph.datasize);
  fprintf(stderr,"wrote %d bytes at %d (.data)\n", i, n);
  while ((i > 0) && (n = read(fd, buffer, min(i,sizeof(buffer)) )) > 0)
  {
    write(elf_fd, buffer, n);
    i -= n;
  }
  
  // write strings
  n = lseek(elf_fd, 0, SEEK_CUR);
  fprintf(stderr,"wrote %d bytes at %d (Strings)\n", stringoffset, n);
  write(elf_fd, globalstrings, stringoffset);

  // write symbols
  n = lseek(elf_fd, 0, SEEK_CUR);
  fprintf(stderr,"wrote %d bytes at %d (Symbols)\n", (int)sizeof(symbols[0]) * numsymbols, n);
  write(elf_fd, symbols, sizeof(symbols[0]) * numsymbols);
  
  n = lseek(elf_fd, 0, SEEK_CUR);
  close(elf_fd);
  

  return 0;
}

