#include <stdio.h>
#include <string.h>
#include <errno.h> 
#include <sys/fcntl.h>
#include <sys/pty.h>
#include <sys/sgtty.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include <net/telnet.h>
#include <net/select.h>
#include <net/inet.h>

#ifndef __clang__
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

typedef int socklen_t;

void setsid() {}

socketopt opt;
char *telnetprocess[] = {"shell", NULL};


#else

extern int open();

#define in_sockaddr sockaddr_in
#define  IPO_TELNET	  23
char *telnetprocess[] = {"sh", NULL};

#include "uniflexshim.h"

#endif

/*

native: cc +v telnetd.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o telnetd telnetd.c 

*/

#define DEBUGxx

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
char copyright[] = "Telnet Server Version 1.0 Copyright (C) 2023 By Adam Billyard\r\n";
char welcomemotd[] = "Welcome to Tektronix 4404 Uniflex\r\n\r\n";
int sock = -1;
int state = STOPPED;
int sessionsock;

int off = 0;
int on = 1;

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
        /* fprintf(stderr, "parseoptdat: term(%d,%d)\n",ts->term.cols,ts->term.lines); */

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
void
cleanup2(sig)
int sig;
{
  fprintf(stderr,"cleanup telnet session on %d\n",sig);
  close(sessionsock);
  exit(0);
}

char hex[3];
char *int2hex(val)
int val;
{
  hex[0] = hexascii[val >> 4];
  hex[1] = hexascii[val & 15];
  hex[2] = 0;
  return(hex);
}

int telnet_session(socket,argc,argv)
int socket;
int argc;
char **argv;
{
  struct termstate ts;
  int ptfd[2];
  int fdmaster,fdslave;
  int n,rc,sessionpid;
  int last_was_cr;

  fd_set fd_in;

  struct sgttyb slave_orig_term_settings; 
  struct sgttyb new_term_settings;
  int i,fd, readspin;
  char buffer[64];
  struct timeval timeout;

  /* telnet state */
  memset(&ts, 0, sizeof(struct termstate));
  ts.sock = socket;
  ts.state = STATE_NORMAL;
  ts.term.type = TERM_VT100;
  ts.term.cols = 80;
  ts.term.lines = 25;

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
    fprintf(stderr, "Error %d on create_pty\n", errno);
    exit(1);
  }

  fdslave = ptfd[0];
  fdmaster = ptfd[1];

    fprintf(stderr, "create_pty() => %d %d\n", fdmaster,fdslave);

    sessionsock = socket;
    signal(SIGQUIT, cleanup2);
    signal(SIGHUP, cleanup2);
    signal(SIGINT, cleanup2);
    signal(SIGTERM, cleanup2);
    signal(SIGPIPE, cleanup2);
    signal(SIGDEAD, cleanup2);

  sessionpid = fork();
  if (sessionpid)
  {
    argv[0] = "dodah";

    /* PARENT */
    
    n = control_pty(fdmaster, PTY_INQUIRY, 0);
    n |= PTY_REMOTE_MODE;
#if 1
    n |= PTY_READ_WAIT; 
#endif
    control_pty(fdmaster, PTY_SET_MODE, n );

    /* motd; Uniflex maps \n to \r so force it in */
    copyright[sizeof(copyright)-1] = 0x0a;
    write(socket, copyright, sizeof(copyright));

     fd = open("/etc/log/motd", O_RDONLY);
     if (fd < 0)
     {
       /* a default MOTD */
       write(socket, welcomemotd, sizeof(welcomemotd));
     }
     else
     {
       while((i = (int)read(fd, buffer, sizeof(buffer))) > 0)
       {
         write(socket, buffer, i);
       }
       close(fd);
     }

    /* Close the slave side of the PTY */
    close(fdslave);
    
   last_was_cr = 0;
    while (1)
    {
      /* Uniflex select appears to only expect actual socket fds */
      /* having stdin in the FD_SET wreaks havoc */
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;
      FD_ZERO(&fd_in);
      FD_SET(socket, &fd_in);
#ifdef __clang__
      FD_SET(fdmaster, &fd_in);
#endif
/*
      fprintf(stderr,"select(%x) on %d,fdmaster%d\n",fd_in.fdmask[0],socket, fdmaster);
      fflush(stderr);
*/
      rc = select(max(fdmaster,socket) + 1, &fd_in, NULL, NULL, &timeout);
/*
      fprintf(stderr,"select(%x) => %d\n", fd_in.fdmask[0], rc);
      fflush(stderr);
*/

      if (rc < 0)
      {
          fprintf(stderr, "select() error %d\n", errno);
          exit(errno);
      }

      if (rc == 0)
      {
        sprintf(buffer, "nothing from %d\n%c", socket, 0x0a);
        /* write(socket, buffer, strlen(buffer));  */
      }
      else
      /* read any socket input and send to slave input */
      if (FD_ISSET(socket, &fd_in))
      {
        if (ts.bi.start == ts.bi.end)
        {
            n = (int)read(socket, ts.bi.data, sizeof(ts.bi.data));
#ifdef DEBUG
  fprintf(stderr, "read %d bytes from %d\n", n, socket);
#endif

            if (n < 0)
            {
              if (errno != EINTR)
              {
                exit(1);
              }

              continue;
            }

#ifndef WHY_DO_WE_NEED_THIS
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
            n = (int)write(fdmaster, ts.bi.start, ts.bi.end - ts.bi.start);
#ifdef DEBUG
  fprintf(stderr, "write %d bytes to fdmaster%d\n", n, fdmaster);
#endif

            if (n < 0)
            {
              if (errno != EINTR)
              {
                exit(errno);
              }
              continue;
            }

            ts.bi.start += n;
        }
      }

      /* read any slave output and send to socket */
      /* select says bytes to read but read busy waits forever */
      /* select just doesn't work with pty descriptors */
      
      n = control_pty(fdmaster, PTY_INQUIRY, 0);
#ifdef DEBUG
      if ((n & PTY_OUTPUT_QUEUED))
      {
        fprintf(stderr, "fdmaster has OUTPUT_QUEUED\n");
        fflush(stderr);
      }
#endif

#ifdef __clang__
      if (FD_ISSET(fdmaster, &fd_in))
#else
      if ((n & PTY_OUTPUT_QUEUED))
#endif
      {
        /* Data arrived from application */
        if (ts.bo.start == ts.bo.end) 
        {
#ifdef DEBUG
          fprintf(stderr, "about to try read from fdmaster%d\n", fdmaster);
          fflush(stderr);
#endif
          n = (int)read(fdmaster, ts.bo.data, sizeof(ts.bo.data)); 
          if (n < 0)
          {
            if (errno != EINTR)
            {
              exit(errno);
            }
            continue;
          }

          if (n == 0)
          {
#ifndef __clang__
           /* this normally means half closed, but we get these all the time */
            if (errno != 0)
#endif
            {
              fprintf(stderr,"broken connection\n");
              close(socket);
              exit(errno);
            }

            readspin++;
          }
          else
          {
#ifdef DEBUG
            fprintf(stderr, "after %d spins, read %d bytes from fdmaster%d\n", readspin, n, fdmaster);
            for(i=0; i<n; i++)
              fprintf(stderr, "0x%s ", int2hex((int)ts.bo.data[i]));
            fprintf(stderr, "\n");
#endif
            readspin = 0;
          }

          ts.bo.start = ts.bo.data;
          ts.bo.end = ts.bo.data + n;
        }

        while (ts.bo.start != ts.bo.end) 
        {
#ifdef DEBUG
          fprintf(stderr, "writing %d bytes to %d\n", ts.bo.end - ts.bo.start, socket);
          fflush(stderr);
#endif
          n = (int)write(socket, ts.bo.start, ts.bo.end - ts.bo.start);
          if (n < 0)
          {
            exit(errno);
          }
          ts.bo.start += n;
        }
      }
    }
  }
  else
  {
    /* CHILD */

    /* Close the master side of the PTY */
    close(fdmaster);

    /* Save the defaults parameters of the slave side of the PTY */
    rc = gtty(fdslave, &slave_orig_term_settings);

    /* Set RAW mode on slave side of PTY */
    new_term_settings = slave_orig_term_settings;
    /*
    new_term_settings.sg_flag |= XTABS;
    new_term_settings.sg_flag |= CNTRL;
    new_term_settings.sg_flag |= RAW;
  */
 
    new_term_settings.sg_flag |= RAW;
    new_term_settings.sg_flag |= CBREAK;
    new_term_settings.sg_flag &= ~ECHO;
    stty(fdslave, &new_term_settings);
    fprintf(stderr,"terminal sg_flag(%x)\n", new_term_settings.sg_flag);

    fprintf(stderr, "telnet session: isatty(%d) ttyname(%s)\n", isatty(fdslave),ttyname(fdslave));


#if 1
    /* The slave side of the PTY becomes the stdin/stdout/stderr of process */
    close(0); /* Close standard input (current terminal) */
    close(1); /* Close standard output (current terminal) */
    close(2); /* Close standard error (current terminal) */
#endif

    /* does opening the  ptty make it controlling? */
    /*
      https://stackoverflow.com/questions/19157202/how-do-terminal-size-changes-get-sent-to-command-line-applications-though-ssh-or/19157360
    */
    
    dup2(fdslave, 0); /* PTY becomes standard input (0) */
    dup2(fdslave, 1); /* PTY becomes standard output (1) */
    dup2(fdslave, 2);  /* PTY becomes standard error (2) */

    /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
    /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
    
    /* ioctl(0, TIOCSCTTY, 1); */

    /* Now the original file descriptor is useless */
   close(fdslave); 

    /* Make the current process a new session leader */
    setsid();

    /* Execution of the program */
    {
      /* launch cmd */
      rc = execvp(telnetprocess[0], telnetprocess);
      if (rc < 0)
      {
        fprintf(stderr, "Error %d on exec\n", errno);
      }
    }

    /* error */
    return errno;
  }

  fprintf(stderr, "EXIT\n");
  return 0;
}

void
cleanup(sig)
int sig;
{
  fprintf(stderr,"cleanup telnetd on %d\n",sig);

  close(sock);
  state = STOPPED;
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

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "socket: %s\n",strerror(errno));
    return 1;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(IPO_TELNET);
  rc = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
  if (rc < 0) {
    fprintf(stderr, "bind: %s\n",strerror(errno));
    close(sock);
    return errno;
  }

    signal(SIGQUIT, cleanup);
    signal(SIGHUP, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

  rc = listen(sock, 5);
  if (rc < 0) {
    fprintf(stderr, "listen: %s\n",strerror(errno));
    close(sock);
    return errno;
  }

  fprintf(stderr, "telnetd started\n");
  state = RUNNING;
  while (state == RUNNING) {
    fprintf(stderr,"telnet: waiting to accept\n");

    newsock = accept(sock, (struct sockaddr *) & cli_addr, &cli_addr_len);
    if (state == STOPPED) break;
    if (newsock < 0) {
      fprintf(stderr, "error %d (%s) in accept\n", errno, strerror(errno));
      close(sock);
      return errno;
    }

#ifndef __clang__
    setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (char *)0, 0);
    /* is turning off Nagle supported? */
#else
    setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
#endif

    if (fork())
    {
      fprintf(stderr, "connect from %s\n", inet_ntoa(cli_addr.sin_addr.s_addr));
       sleep(5);
       close(newsock); 
    }
    else
    {
      rc = telnet_session(newsock, argc, argv);
     fprintf(stderr, "client disconnected (%d) from %s\n", rc, inet_ntoa(cli_addr.sin_addr.s_addr));
     break;
    }

  }
  close(sock); 
  return 0;
}

