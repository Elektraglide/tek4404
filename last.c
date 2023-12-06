#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>

typedef struct
{
  char code[8];
  int unused;  
  long timestamp;
} HistoryRecord;

int main(argc, argv)
int argc;
char **argv;
{
  int fd,n;
  HistoryRecord record;
  char buffer[32];
      
   fd = open("/act/history", O_RDONLY);	
  if (fd > 0)
  {
    while((n = read(fd, &record, sizeof(record))) > 0)
    {
      strcpy(buffer, ctime(&record.timestamp));
      /* no CR */
      buffer[24] = '\0';
      printf("%s\t%02s\n", buffer, record.code);
    }
    close(fd);
  }
  
  exit(0);
}


