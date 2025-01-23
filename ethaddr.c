#include <stdio.h>

#include <net/config.h>
#include <net/std.h>
#include <net/socket.h>
#include <net/netioc.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// undocumented pioctl() params
#define ENIOCONLINE	(ENIOCBASE+4)
#define ENIOCOFFLINE	(ENIOCBASE+5)
#define ENIOCGETADDR	(ENIOCBASE+6)
#define	ENIOCSETADDR	(ENIOCBASE+7)
#define	ENIOCPROBE	(ENIOCBASE+8)
#define	ENIOCWRITEADDR	(ENIOCBASE+9)


unsigned char macaddr[64];

int main(argc, argv)
int argc;
char *argv[];
{
int fd;
int i,n,v;

  // em
  fd = socket(AF_ETHER, SOCK_RAW, 0);
  if (fd > 0)
  {
    bind(fd, "", 0);
    pioctl(fd, ENIOCGETADDR, macaddr, 6,6);  /* why second 6 needed? */
    for(i=0; i<6; i++)
    {
      printf("%2.2x ", macaddr[i] );
    }
    printf("\n");
    close(fd);
  }

  exit(0);
}

