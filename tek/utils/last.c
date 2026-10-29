#include <stdio.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/acct.h>
#include <utmp.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* very slow but lookup in utmp */
struct utmp *findentry(timestamp)
int timestamp;
{
int fd;
static struct utmp* record;

    fd = open("/act/utmp", O_RDONLY);
    if (fd > 0)
    {
        setutent();
        while ((record = getutent()) != NULL)
        {
            if (record->ut_time == timestamp)
            {
                close(fd);
                return record;
            }
        }
        endutent();
        close(fd);
    }
    return NULL;
}

int main(argc, argv)
int argc;
char **argv;
{
  int fd,n;
  struct utmp* active_login;
  struct hist record;
  char buffer[32];
  int show_historical_login;
  char *base;

  /* 'who' just shows current logins */
  /* 'last' shows historical logins */
  base = strrchr(argv[0], '/');
  if (base)
      base++;
  else
      base = argv[0];
  show_historical_login = (strcmp(base, "last") == 0);

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
      /* is this a still current login? */
      active_login = findentry(record.time_field);

      if (active_login)
      {
          printf("%-10s\t%s *", record.user_name, active_login->ut_line + 5);
      }
      else
      if (show_historical_login)
      {
          if (record.user_name[0])
          {
            printf("%-10s\ttty%c%c", record.user_name, record.tty_num[0], record.tty_num[1]);
          }
          else /* assorted entries from .init.control */
          {
            if (record.tty_num[0] ==  'b' && record.tty_num[1] == 't')
            printf("boot time\t");
            else
            if (record.tty_num[0] == 's' && record.tty_num[1] == 't')
                printf("shutdown time\t");
            else

            /* what are these? */
            if (record.tty_num[0] == 'b' && record.tty_num[1] == 'd')
                printf("boot done?\t");
            else
            if (record.tty_num[0] == 'a' && record.tty_num[1] == 'd')
                continue;
            else
            if (record.tty_num[0] == '0' && record.tty_num[1] == '0')
                continue;
          }
      }

      if (active_login || show_historical_login)
      {
        strcpy(buffer, ctime(&record.time_field));
        /* no CR */
        buffer[24] = '\0';

        printf("\t%s\n", buffer);
      }
    }
    close(fd);
  }
  
  exit(0);
}


