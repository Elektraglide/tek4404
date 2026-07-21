#include <stdio.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

char nvram[65536];

int main(argc, argv)
int argc;
char *argv[];
{
int fp,fp2;
int i,n,total;
char filename[32];
int offset;

fp = open(argv[1], 0);
if (fp < 0)
{
    printf("failed to open %s\n", argv[1]);
  exit(1);
}

offset = 0x00000;
if (argc > 1)
	sscanf(argv[2], "%x", &offset);

printf("dumping %s starting at 0x%8.8x\n", argv[1], offset);

lseek(fp, offset, 0);

for(i=0; i<0x100000; i+= 65536)
{
   n = read(fp, nvram, 65536);
   printf("read %d bytes offset %d\n", n, i);
 
  sprintf(filename, "mem%7.7x.bin", offset + i);
  fp2 = creat(filename, 0x01);
  if (fp2 > 0)
  {

    n = write(fp2, nvram, 65536);
    printf("dumping %s (%d bytes)\n", filename, n);
    close(fp2); 
  }
}

printf("\ncleaning up\n");
close(fp);

exit(0);
}

