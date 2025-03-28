#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/acct.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int main(argc, argv)
int argc;
char **argv;
{
  int fd,n;
  struct hist record;
  char buffer[32];
      
   fd = open("/act/history", O_RDONLY);	
  if (fd > 0)
  {
    /* show last 32 enrties */
  	n = lseek(fd, 0, SEEK_END);
  	if (n > sizeof(record) * 32)
 	  n = sizeof(record) * 32;
  	lseek(fd, -n, SEEK_CUR);
  	
    while((n = read(fd, &record, sizeof(record))) > 0)
    {
      /* some records are init record */
      if (record.user_name[0] == '\0')
      	continue;
      
      strcpy(buffer, ctime(&record.time_field));
      /* no CR */
      buffer[24] = '\0';

      printf("%10s tty%c%c\t%s\n", record.user_name,record.tty_num[0],record.tty_num[1], buffer);
    }
    close(fd);
  }
  
  exit(0);
}


