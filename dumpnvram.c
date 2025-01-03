#include <stdio.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

unsigned char nvram[64];

int main(argc, argv)
int argc;
char *argv[];
{
int fp;
int i,n,v;
unsigned int nvmem = 0x721000;

fp = open("/dev/pmem", 0);
if (fp < 0)
{
  fprintf(stderr, "failed to open\n");
  exit(1);
}

/* custom address to map? */
if (argc > 1)
{
  if (sscanf(argv[1], "0x%x", &n) == 1)
  {
    nvmem = n;
  }
}

/* nvram */
lseek(fp, nvmem, SEEK_SET);
n = read(fp, nvram, 24);
close(fp);

fprintf(stderr,"read %d bytes from 0x%8.8x\n",n,nvmem);
for(i=0; i<n; i += 4)
{
  /* stored as 4-bit nybble in high bits */
  v = (nvram[i] & 0xf0) | (nvram[i+2] >> 4);

  printf("%2.2x ", v);
}
printf("\n");

exit(0);
}

