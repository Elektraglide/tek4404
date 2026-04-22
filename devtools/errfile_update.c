#include <stdio.h>
#include <fcntl.h>
#include <sys/modes.h>

#define SEEK_SET 0

/* parse Uniflex error messages and insert/update a message */

void writemesg(fdout, i, newoffset, length, buffer)
int fdout;
int length;
char *buffer;
{
  short soff = newoffset;
  short slen = length;

  lseek(fdout, i * 2, SEEK_SET);
  write(fdout, &soff, 2);
  lseek(fdout, newoffset, SEEK_SET);
  write(fdout, &slen, 2);
  write(fdout, buffer, length);
}

int main(argc, argv)
int argc;
char **argv;
{
int fd,fdout,curr;
short oldnumentries,i,offset,length;
char buffer[128];

short newnumentries;
int newerr;
char *newmsg;
int newoffset;

  if (argc > 1)
  {
    fd = open(argv[1], O_RDONLY);	
    newerr = atol(argv[2]);
    newmsg = argv[3];

    if (fd > 0 && newerr < 512 && newmsg)
    {
      /* offset of first message allows us to calc how many entries */          
      read(fd, &oldnumentries, sizeof(oldnumentries));
      oldnumentries >>= 1;

	  newnumentries = newerr < oldnumentries ? oldnumentries : newerr + 1;
	  newoffset = newnumentries * 2;

      fdout = creat("/tmp/foo", S_IREAD | S_IWRITE);
	  if (fdout < 0)
	  {
	    fprintf(stderr, "unable to create output file\n");
	    exit(1);	
	  }
	  
      for(i=0; i<oldnumentries; i++)
      {
      	/* read existing one */
        lseek(fd, i * 2, SEEK_SET);
        read(fd, &offset, sizeof(offset));
	    lseek(fd, offset, SEEK_SET);
	    read(fd, &length, 2);
	    read(fd, buffer, length);

		if (i == newerr)
		{
          fprintf(stderr, "replacing %d: %s => %s\n", i, buffer, newmsg);
          length = 2 + strlen(newmsg) + 1;	
          strcpy(buffer, newmsg);
		}

		/* write new one */
		writemesg(fdout, i, newoffset, length, buffer);
		newoffset += length;				
      }
      close(fd);

      /* empty messages if needed to pad to our newerr */
	  for( ;i<newerr; i++)
	  {
        writemesg(fdout, i, newoffset, 1, "");
        newoffset += 2 + 1;
	  }
	  if (i == newerr)
	  {
        fprintf(stderr, "appending %d: %s\n", i, newmsg);
	    writemesg(fdout, i, newoffset, strlen(newmsg)+1, newmsg);
	  }
      close(fdout);	  

    }
    else
    {
      perror("cannot open");	
    }
  }	
  else
  {
    fprintf(stderr,"adderr errorfile errnum errmesg\n");	
  }

}
