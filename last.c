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
    
   fd = open("/act/history", O_RDONLY);	
  if (fd > 0)
  {
    while((n = read(fd, &record, sizeof(record))) > 0)
    {
      printf("%02s %s\n", record.code, ctime(&record.timestamp));
    }
    close(fd);
  }
  
  exit(0);
}


