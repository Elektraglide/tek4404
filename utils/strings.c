#include <stdio.h>
#include <ctype.h>

#include <sys/fcntl.h>
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


char astring[256];

int main(argc, argv)
int argc;
char *argv[];
{
FILE *fp;
int i = 0;
char c = 1;
int total = 0;
int startpos = 0x40;
PH ph;
int fd;

/* should we seek to .data */
fd = open(argv[1], O_RDONLY);
if (fd > 0)
{
  read(fd, &ph, sizeof(ph));
  close(fd);

  if (ph.magic[0] == 0x04)
  {
	startpos = 0x40 + ph.textsize;
  }
}

fp = fopen(argv[1], "r");
if (fp == NULL)
{
    printf("failed to open %s\n", argv[1]);
  exit(1);
}

/* get top start of search area */
fseek(fp, startpos, SEEK_SET);

while (!feof(fp))
{
total++;
c = fgetc(fp);
if (c >= '0' && c <= '9')
  astring[i++] = c;

if (c >= 'a' && c <= 'z')
  astring[i++] = c;

if (c >= 'A' && c <= 'Z')
  astring[i++] = c;

if (ispunct(c))
  astring[i++] = c;

if (c == 0  || i > 253)
{
  astring[i] = 0;
  if (i > 3)
    printf("%s\n", astring);
  i = 0;
}
}
printf("*** cleaning up read %d bytes\n", total);
fclose(fp);

exit(0);
}

