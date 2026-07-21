#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/acct.h>

int main(argc, argv)
int argc;
char **argv;
{
  int fd,n;
  struct hist record;
  char buffer[32];
      
   fd = open("/act/attempts", O_RDONLY);	
  if (fd > 0)
  {
    while((n = read(fd, &record, sizeof(record))) > 0)
    {
      /* some records are init record */
      if (record.user_name[0] == '\0')
      	continue;
      
      strcpy(buffer, ctime(&record.time_field));
      /* no CR */
      buffer[24] = '\0';

      printf("%s tty%c%c\t%s\n", record.user_name,record.tty_num[0],record.tty_num[1], buffer);
    }
    close(fd);
  }
  
  exit(0);
}


