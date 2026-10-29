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
      if (record.user_name[0] < '0')
      	continue;
      
      /* ensure its terminated */
      strncpy(buffer, record.user_name, 10);
      buffer[10] = '\0';

      printf("%-10s\ttty%c%c", buffer, record.tty_num[0], record.tty_num[1]);

      strcpy(buffer, ctime(&record.time_field));
      /* no CR */
      buffer[24] = '\0';
      printf("\t%s\n", buffer);

    }
    close(fd);
  }
  
  exit(0);
}


