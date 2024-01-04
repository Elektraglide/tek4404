#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>
#include <utmp.h>

int main(argc, argv)
int argc;
char **argv;
{
  int fd,n;
  struct utmp *record;
  char buffer[32];

  fd = open("/act/utmp", O_RDONLY);
  if (fd > 0)
  {
  	setutent();
    while((record = getutent()) > 0)
    {
      strcpy(buffer, ctime(&record->ut_time));
      /* no CR */
      buffer[24] = '\0';
      
      printf("%s\t%02s\n", record->ut_user, record->ut_line, buffer );
    }
    endutent();
    
    close(fd);
  }
  
  exit(0);
}


