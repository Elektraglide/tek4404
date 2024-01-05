#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/sir.h>

int getsize(ptr)
unsigned char *ptr;
{
  int n;

  n = ptr[0];
  n *= 256;
  n += ptr[1];
  n *= 256;
  n += ptr[2];

  return n;
}

int main(argc, argv)
int argc;
char **argv;
{
  int fd;
  unsigned int n;
  struct sir record;
  char buffer[32];

  fd = open("/dev/swap", O_RDONLY);
  if (fd > 0)
  {
  	lseek(fd, 512, 0);

	read(fd, &record, sizeof(record));

	printf("System name: %s\n", record.sfname);
	printf("Volume name: %s\n", record.spname);
	printf("Volume number: %d\n", record.sfnumb);
	printf("Creation time: %s", ctime(&record.scrtim));
	printf("Last update time: %s", ctime(&record.sutime));

	printf("Total size: %d\n", getsize(record.ssizfr) * 512);

	printf("Total free: %d\n", getsize(record.sfreec) * 512);

	printf("Total FDN blocks: %d\n", record.sszfdn);
	printf("Free FDN: %d\n", record.sfdnc);

        /* NB its only 2 bytes, so discard lower 8 bits */
 	printf("Swap size: %d\n", getsize(record.sswpsz) / 256 * 512);
 	printf("Flawed blocks: %d\n", record.sflawc);

	close(fd);
  }
  
  exit(0);
}


