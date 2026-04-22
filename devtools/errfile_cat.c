#include <stdio.h>
#include <fcntl.h>

#define SEEK_SET 0

/* parse Uniflex error messages and cat them to stdout */

int main(argc, argv)
int argc;
char **argv;
{
int fd,curr;
short firstentry,i,offset,length;
char buffer[128];

  if (argc > 1)
  {
    fd = open(argv[1], O_RDONLY);	
    if (fd > 0)
    {
      /* offset of first message allows us to calc how many entries */
      read(fd, &firstentry, sizeof(firstentry));
      firstentry >>= 1;
      for(i=0; i<firstentry; i++)
      {
        lseek(fd, i * 2, SEEK_SET);
        read(fd, &offset, sizeof(offset));
	    lseek(fd, offset, SEEK_SET);
	    read(fd, &length, 2);
	    read(fd, buffer, length);
	    if (length > 1)
          printf("%3d: 0x%4.4x %s\n", i, offset, buffer);
      }    
      close(fd);
    }
    else
    {
      perror("cannot open");	
    }
  }	
  else
  {
    fprintf(stderr,"%s errorfile\n",argv[0]);
  }

}
