/*
Ctrl-C is received  but writing to fdmaster does not send SIGINT
Is that shell ignoring it, or broken pty implementation?
Appears to be broken pty implementation AND shell ignoring SIGINT

*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/pty.h>
#include <sys/sgtty.h>
#include <sys/stat.h>
#include <signal.h>

#include <net/telnet.h>
#include <net/select.h>
#include <net/inet.h>
#include <net/nerrno.h>

#ifndef __clang__
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

typedef int socklen_t;

void setsid() {}

socketopt opt;
char *telnetprocess[] = {"/etc/login", NULL};

#else

extern int open();
extern int wait();
extern int kill();

extern int system_control();  /* arg=1 does something,  arg=2 makes pty controlling terminal.. */

#define in_sockaddr sockaddr_in
#define  IPO_TELNET    23
char *telnetprocess[] = {"sh", NULL};

#include "uniflexshim.h"

#endif

/*

native: cc +v telnetd.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o telnetd telnetd.c 

*/

#define DEBUGxx
#define DEBUGCONSOLExx
/***********************************/


/* States */

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_OPT    2
#define STATE_SB     3
#define STATE_OPTDAT 4
#define STATE_SE     5


/* Telnet options NOT defined by Tek4404 headers */

#define TELOPT_TERMINAL_TYPE       24  /* Terminal Type (RFC1091) */
#define TELOPT_NAWS                31  /* Negotiate About Window Size (RFC1073) */
#define TELOPT_TERMINAL_SPEED      32  /* Terminal Speed (RFC1079) */
#define TELOPT_TOGGLE_FLOW_CONTROL 33  /* Remote Flow Control (RFC1372) */
#define TELOPT_LINEMODE            34  /* Linemode (RFC1184) */
#define TELOPT_AUTHENTICATION      37  /* Authentication (RFC1416) */

/* Globals */

#define STOPPED 0
#define RUNNING 1

char hexascii[] = "0123456789ABCDEF";

/* Uniflex maps \n to \r..  so we have to insert at runtime */
char copyright[] = "Telnet Server Version 1.1 Copyright (C) 2024 By Adam Billyard\n";
char welcomemotd[] = "Welcome to Tektronix 4404 Uniflex\n\n";
char linefeed[] = "\012";
int sock = -1;
int state = STOPPED;
int sessionsock;
int sessionpty;

int off = 0;
int on = 1;

FILE  *console;

char sessionname[64];
char *sessionargv[2];

struct buffer {
  unsigned char data[4096];
  unsigned char *start;
  unsigned char *end;
};

/* is this a standard thing? */
#define TERM_VT100 1

typedef struct {
  int type;
  int cols,lines;
  int ttyin, ttyout;
} termparams;

struct termstate {
  int sock;
  int state;
  int code;
  unsigned char optdata[256];
  int optlen;
  termparams term;
  struct buffer bi;
  struct buffer bo;
};

void sendopt(ts, code, option)
struct termstate *ts;
int code;
int option;
{
  unsigned char buf[3];

  buf[0] = IAC;
  buf[1] = (unsigned char) code;
  buf[2] = (unsigned char) option;
  write(ts->sock, buf, 3);
  
  /* fprintf(stderr, "sendopt: OPTION %d code %d\n", option, code); */

}

void parseopt(ts, code, option)
struct termstate *ts;
int code;
int option;
{

  switch (option) {
    case T_ECHO:
    case T_SGA:
    case TELOPT_NAWS:
      break;

    case TELOPT_TERMINAL_TYPE:
    case TELOPT_TERMINAL_SPEED:
      sendopt(ts, DO, option);
      break;

    default:
      if (code == WILL || code == WONT) {
        sendopt(ts, DONT, option);
      } else {
        sendopt(ts, WONT, option);
      }
  }
}

void parseoptdat(ts, option, data, len)
struct termstate *ts;
int option;
unsigned char *data;
int len;
{
  int cols,lines;

  /* fprintf(stderr, "parseoptdat: OPTION %d data (%d bytes)\n", option, len); */

  switch (option) {
    case TELOPT_NAWS:
      if (len == 4) {
        cols = ntohs(*(unsigned short *) data);
        lines = ntohs(*(unsigned short *) (data + 2));
        if (cols != 0) ts->term.cols = cols;
        if (lines != 0) ts->term.lines = lines;
        fprintf(console, "parseoptdat: term(%d,%d)\012\n",ts->term.cols,ts->term.lines);

        /* TODO: send child new window size; we dont have SIGWINCH */

      }
      break;

    case TELOPT_TERMINAL_SPEED:
      break;

    case TELOPT_TERMINAL_TYPE:    
      break;
  }
}

void parse(ts) 
struct termstate *ts;
{
  unsigned char *p = ts->bi.start;
  unsigned char *q = p;
  int c;

  while (p < ts->bi.end) {
    c = *p++;

    switch (ts->state) {
      case STATE_NORMAL:
        if (c == IAC) {
          ts->state = STATE_IAC;
        } else {
          *q++ = c;
        }
        break;

      case STATE_IAC:
        switch (c) {
          case IAC:
            *q++ = c;
            ts->state = STATE_NORMAL;
            break;

          case WILL:
          case WONT:
          case DO:
          case DONT:
            ts->code = c;
            ts->state = STATE_OPT;
            break;

          case SB:
            ts->state = STATE_SB;
            break;

          default:
            ts->state = STATE_NORMAL;
        }
        break;

      case STATE_OPT:
        parseopt(ts, ts->code, c);
        ts->state = STATE_NORMAL;
        break;

      case STATE_SB:
        ts->code = c;
        ts->optlen = 0;
        ts->state = STATE_OPTDAT;
        break;

      case STATE_OPTDAT:
        if (c == IAC) {
          ts->state = STATE_SE;
        } else if (ts->optlen < sizeof(ts->optdata)) {
          ts->optdata[ts->optlen++] = c;
        }
        break;

      case STATE_SE:
        if (c == SE) parseoptdat(ts, ts->code, ts->optdata, ts->optlen);
        ts->state = STATE_NORMAL;
        break;
    } 
  }

  ts->bi.end = q;
}

/************************************************/
char hex[3];
char *int2hex(val)
int val;
{
  hex[0] = hexascii[val >> 4];
  hex[1] = hexascii[val & 15];
  hex[2] = 0;
  return(hex);
}

void
cleanup_child(sig)
int sig;
{
  int result;
  
#ifdef DEBUGCONSOLE
  fprintf(console,"cleanup telnet session on %d\012\n",sig);
#endif
  close(sessionsock);
  close(sessionpty);
}

void
testsig(sig)
int sig;
{
  int result;
  
  fprintf(console,"signal(%d) for pid%d\012\n",sig, getpid());

  signal(sig, testsig);
}

void writewithlf(socket, buffer, len)
int socket;
char *buffer;
int len;
{
char  *cwp, *nlp;
int n;
        
         /* insert linefeeds */
  cwp = buffer;
  while(len > 0)
  {
    nlp = strchr(cwp, '\n');
   if(!nlp)
   {
      break;
   }
   else
   {
      nlp++;
      n = (int)nlp - (int)cwp;
      write(socket, cwp, n);
      write(socket, &linefeed, sizeof(linefeed));
      cwp = nlp;
      len -= n;
   }
 }
         
 if (len > 0)
   write(socket, cwp, len);

}

int telnet_session(din,dout,from)
int din,dout;
char *from;
{
  struct termstate ts;
  int ptfd[2];
  int fdmaster,fdslave;
  int n,rc,sessionpid;
  int last_was_cr;
  int last_read;
    
  fd_set fd_in;

  struct sgttyb slave_orig_term_settings; 
  struct sgttyb new_term_settings;
  int i,fd;
  char buffer[64];
  struct timeval timeout;

  /* telnet state */
  memset(&ts, 0, sizeof(struct termstate));
  ts.sock = din;
  ts.state = STATE_NORMAL;
  ts.term.type = TERM_VT100;
  ts.term.cols = 80;
  ts.term.lines = 30;

  /* Send initial options */
  sendopt(&ts, WILL, T_ECHO);
  sendopt(&ts, WILL, T_SGA);
  sendopt(&ts, WONT, TELOPT_LINEMODE);
  sendopt(&ts, DO, TELOPT_NAWS);

  /* no buffering */
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  /* build pty pair */
  rc = create_pty(ptfd);
  if (rc < 0)
  {
    fprintf(console, "Error %d on create_pty\012\n", errno);
    exit(1);
  }

  fdslave = ptfd[0];
  fdmaster = ptfd[1];

  sessionsock = din;
  sessionpty = fdslave;
  
  signal(SIGPIPE, cleanup_child);
  signal(SIGDEAD, cleanup_child);

  signal(SIGHUP, testsig);
  signal(SIGINT, testsig);
  signal(SIGQUIT, testsig);
  signal(SIGTERM, testsig);
  signal(SIGEMT, testsig);

  sessionpid = fork();
  if (sessionpid)
  {
    /* PARENT */
    console = fopen("/dev/console", "a");
    
    n = control_pty(fdmaster, PTY_INQUIRY, 0);
    n |= PTY_READ_WAIT; 
    control_pty(fdmaster, PTY_SET_MODE, n );

    /* motd; Uniflex maps \n to \r so force it in */
    writewithlf(dout, copyright, sizeof(copyright)-1);

#ifdef SHOW_MOTD
    fd = open("/etc/log/motd", O_RDONLY);
    if (fd < 0)
    {
      /* a default MOTD */
      writewithlf(dout, welcomemotd, sizeof(welcomemotd)-1);
    }
    else
    {
      while((i = (int)read(fd, buffer, sizeof(buffer)-1)) > 0)
      {
        buffer[i] = '\0';
        writewithlf(dout, buffer, i);
      }
      close(fd);
    }
#endif

    /* Close the slave side of the PTY */
    close(fdslave);

    last_was_cr = 0;
    last_read = 0;
    while(state != STOPPED)
    {
        /* Uniflex select appears to only expect actual socket fds */
        /* having stdin in the FD_SET wreaks havoc */
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        FD_ZERO(&fd_in);
        n = 0;
        
        FD_SET(din, &fd_in);
        if (din > n)
          n = din;

#ifdef __clang__
        FD_SET(fdmaster, &fd_in);
        if (fdmaster > n)
          n = fdmaster;
#else
        if(last_read > 0)
        {
          /* if there is input, for a few loops use a short timeout */
          timeout.tv_usec = 5000;
          last_read--;
        }
#endif

/*
        fprintf(stderr,"select(%d, %x,0,0,%d)\012\n",n+1,fd_in.fdmask[0],timeout.tv_usec);
        fflush(stderr);

*/
#ifdef DEBUGCONSOLE
fprintf(console, "**Select n(%d) timeout(%d)\012\n", n, timeout.tv_usec);
#endif
        rc = select(n + 1, &fd_in, NULL, NULL, &timeout);

        if (rc < 0 && errno != EINTR)
        {
            fprintf(console, "select(): error %d\n", errno);
            break;
        }


        if (rc == 0)
        {
          /* fprintf(console, "select(): nothing from %d\012\n", din); */
        }
        else

        /* read any socket input (din) and send to slave input */
        if (FD_ISSET(din, &fd_in))
        {
          if (ts.bi.start == ts.bi.end)
          {
#ifdef DEBUGCONSOLE
fprintf(console, "**Read din\012\n");
#endif
              n = (int)read(din, ts.bi.data, sizeof(ts.bi.data));

              if (n < 0)
              {
                if (errno != EINTR)
                {
                 fprintf(console, "read() error %d\n", errno);
                  break;
                }

                continue;
              }

#if 0
              /* Convert cr nul to cr lf. */
              for (i = 0; i < n; ++i) {
                unsigned char ch = ts.bi.data[i];
                if (ch == 0 && last_was_cr) ts.bi.data[i] = '\n';
                last_was_cr = (ch == '\r');
              }
#endif

              ts.bi.start = ts.bi.data;
              ts.bi.end = ts.bi.data + n;
          }

          /* Parse user input for telnet options */
          parse(&ts);

          /* Application ready to receive */
          if (ts.bi.start != ts.bi.end)
          {
#ifdef DEBUGCONSOLE
fprintf(console, "**Write master %d bytes\012\n", ts.bi.end - ts.bi.start);
#endif
              n = (int)write(fdmaster, ts.bi.start, ts.bi.end - ts.bi.start);

             /* Uniflex pty does not handle Ctrl-C! */
             if (ts.bi.start[0] == 0x03)
             {
               /* should skip iff RAW mode.. */
             
               /* we would like to kill any buffered output */
               control_pty(fdmaster, PTY_FLUSH_WRITE, 0);
               write(dout, "SIGINT\012\n", 8);

               fprintf(console, "sent SIGINT to %d\012\n", sessionpid);
               kill(sessionpid, SIGINT);
             }

             /* Uniflex pty does not handle Ctrl-D! */
             if (ts.bi.start[0] == 0x04)
             {
                 /* should skip iff RAW mode.. */

                 /* we would like to kill any buffered output */
                 control_pty(fdmaster, PTY_FLUSH_WRITE, 0);
                 write(dout, "SIGHUP\012\n", 8);

                 fprintf(console, "sent SIGHUP to %d\012\n", sessionpid);
                 kill(sessionpid, SIGHUP);

             }
             
             if (n < 0)
              {
                if (errno != EINTR)
                {
                 fprintf(console, "fdmaster write() error %d\n", errno);
                  break;
                }
                continue;
              }

              ts.bi.start += n;
          }
        }

        /* read any slave output and send to socket */
        /* select says bytes to read but read busy waits forever */
        /* select just doesn't work with pty descriptors */
        
#ifdef __clang__
        if (FD_ISSET(fdmaster, &fd_in))
#else
        n = control_pty(fdmaster, PTY_INQUIRY, 0);
        last_read = (n & PTY_OUTPUT_QUEUED) ? 20 : last_read;
        while (n & PTY_OUTPUT_QUEUED)
#endif
        {
          /* Data arrived from application */
          if (ts.bo.start == ts.bo.end)
          {
#ifdef DEBUGCONSOLE
fprintf(console, "**Read master\012\n");
#endif
            n = (int)read(fdmaster, ts.bo.data, sizeof(ts.bo.data));
            if (n < 0)
            {
              if (errno != EINTR)
              {
                 fprintf(console, "fdmaster read() error %d\n", errno);
                break;
              }
              continue;
            }

            if (n == 0)
            {
                fprintf(console,"broken connection\012\n");
                break;
            }
            else
            if (n == sizeof(ts.bo.data))
            {
              fprintf(console,"read chocked\n");
            }

            ts.bo.start = ts.bo.data;
            ts.bo.end = ts.bo.data + n;
          }
        
          if (ts.bo.start != ts.bo.end)
          {
#ifdef DEBUGCONSOLE
fprintf(console, "**Write dout %d bytes \012\n", ts.bo.end - ts.bo.start);
#endif
            n = (int)write(dout, ts.bo.start, ts.bo.end - ts.bo.start);
            if (n < 0)
            {
              break;
            }
            if (n < ts.bo.end - ts.bo.start)
            {
              fprintf(console, "output choked\n");
            }
            ts.bo.start += n;
          }

#ifndef __clang__
          /* check again if any output */
          n = control_pty(fdmaster, PTY_INQUIRY, 0);
          if (n & PTY_OUTPUT_QUEUED)
          {
            /* fprintf(console, "MORE slave output is waiting\012\n"); */
          }
#endif
        }

      }
    
      /* collect child process */
      kill(sessionpid, SIGQUIT);
      wait(&rc);
  }
  else
  {
    /* CHILD */
    signal(SIGHUP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    /* Close the master side of the PTY */
    close(fdmaster);

    /* make a friendly name */
    strcpy(sessionname, "telnet_");
    strcat(sessionname, from);
    sessionargv[0] = sessionname;
    sessionargv[1] = NULL;
      
    /* does opening the  ptty make it controlling? */
    /*
      https://stackoverflow.com/questions/19157202/how-do-terminal-size-changes-get-sent-to-command-line-applications-though-ssh-or/19157360
    */
    
    /* calls pty_make_controlling_terminal */
    system_control(2);
    
    dup2(fdslave, 0); /* PTY becomes standard input (0) */
    dup2(fdslave, 1); /* PTY becomes standard output (1) */
    dup2(fdslave, 2);  /* PTY becomes standard error (2) */

    /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
    /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
    
    /* ioctl(0, TIOCSCTTY, 1); */

    /* Now the original file descriptor is useless */
    /* close(fdslave); */

    /* Make the current process a new session leader */
    setsid();

    /* Execution of the program */
    {
      /* launch cmd */
      rc = execvp(telnetprocess[0], sessionargv);
      if (rc < 0)
      {
        fprintf(console, "Error %d on exec\n", errno);
      }
    }

    /* error */
    return errno;
  }

#ifdef DEBUG
  fprintf(console, "EXIT telnet_session\n");
#endif
  return errno;
}

void
cleanup_and_exit(sig)
int sig;
{
#ifdef DEBUG
  fprintf(stderr,"cleanup telnetd on %d\012\n",sig);
#endif
  close(sock);
  state = STOPPED;
}
void
cleanup_session(sig)
int sig;
{
  int rc,pid;
  
  pid = wait(&rc);
#ifdef DEBUG
  fprintf(stderr,"cleanup session: telnet proc(%d)\012\n",pid);
#endif

  signal(SIGDEAD, cleanup_session);
}

void logtime()
{
  time_t timestamp;
  struct tm *ts;

    timestamp = time(NULL);
    ts = localtime(&timestamp);
    fprintf(console, "%2.2d-%2.2d-%4.4d %2.2d:%2.2d",
        ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900,
        ts->tm_hour, ts->tm_min);
}

int
main(argc,argv)
int argc;
char **argv;
{
  int newsock;
  int rc;
  int pair[2];
  struct in_sockaddr serv_addr;
  struct in_sockaddr cli_addr;
  socklen_t cli_addr_len;
#ifndef __clang__
  socketopt opt;
#endif
  int reuse = 1;

  console = fopen(nget_str("console"), "a");

  state = RUNNING;

   /* listen for connections too? */
  if (argc > 1 && !strcmp(argv[1],"-s"))
  {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      fprintf(console, "socket: %s\n",strerror(errno));
      return 1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(IPO_TELNET);
    rc = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
    if (rc < 0) {
      fprintf(console, "bind: %s\n",strerror(errno));
      close(sock);
      return errno;
    }

     signal(SIGINT, SIG_IGN);
     signal(SIGQUIT, SIG_IGN);
     
     signal(SIGTERM, cleanup_and_exit);
     signal(SIGDEAD, cleanup_session);

    rc = listen(sock, 5);
    if (rc < 0) {
      fprintf(console, "listen: %s\n",strerror(errno));
      close(sock);
      return errno;
    }

    fprintf(console, "telnetd started\n");
    while (state == RUNNING)
    {
      newsock = 0;
      while (state == RUNNING && newsock <= 0)
      {
        newsock = accept(sock, (struct sockaddr *) & cli_addr, &cli_addr_len);
        if (newsock < 0){
          if (errno == EINTR) continue;
          if (errno == ETIMEDOUT) continue;
          if (errno == ENETUNREACH) continue;
          if (errno == EHOSTUNREACH) continue;
          if (errno == ECONNRESET) continue;
          if (errno == ENETDOWN) continue;
          if (errno == ENOPROTOOPT) continue;

          fprintf(console, "error %d (%s) in accept\n", errno, strerror(errno));
          close(sock);
          return errno;
        }
      }
      
      if (state == STOPPED) break;

#ifndef __clang__
      /* setsockopt(newsock, SOL_SOCKET, SO_DONTLINGER, (char *)0, 0); */
      /* is turning off Nagle supported? */
#else
      setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
#endif

      /* Uniflex accept() doesn't fill this in.. */
      cli_addr_len = sizeof(cli_addr);
      rc = getpeername(newsock, (struct sockaddr *) &cli_addr, &cli_addr_len);

      if (fork())
      {
        logtime();
        fprintf(console, ": connect from %s\012\n", inet_ntoa(cli_addr.sin_addr.s_addr));
      
        sleep(5);
        close(newsock);
      }
      else
      {
        argv[0] = "client";

        rc = telnet_session(newsock, newsock, inet_ntoa(cli_addr.sin_addr.s_addr));
      
        logtime();
        fprintf(console, ": disconnect from %s\012\n",  inet_ntoa(cli_addr.sin_addr.s_addr));

        break;
      }

    }
    
    close(sock);
  }
  else
  {
    struct stat s;
    
    fstat(0, &s);
    if (s.st_mode & S_IFPIPE)
    {
      /* use socketpair */
      rc = telnet_session(0, 1, "local");
    }
    else
    {
      fprintf(console, "stdin is not a pipe.\n");	
    }
  }

  fclose(console);  
  return 0;
}

