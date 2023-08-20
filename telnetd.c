#include <stdio.h>
#include <string.h>
#include <errno.h> 
#include <sys/pty.h> 
#include <sys/sgtty.h>
#include <sys/fcntl.h>
#include <signal.h>

#include <net/telnet.h>
#include <net/select.h>
#include <net/inet.h>

#ifndef __clang__

typedef int fd_set;
#define FD_SET(A,SET)	*SET |= (1<<A)
#define FD_CLR(A,SET)	*SET &= ~(1<<A)
#define FD_ZERO(SET)	*SET = 0
#define FD_ISSET(A,SET)	(*SET & (1<<A))

void setsid() {}

socketopt opt;
char *telnetprocess[] = {"shell", NULL};


#else

#define	IPO_TELNET		23
char *telnetprocess[] = {"bash", NULL};

#include "uniflexshim.h"

#endif

/*

native: cc +v pty_test.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o telnetd telnetd.c 

*/

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

int sock = -1;
int state = STOPPED;

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

  fprintf(stderr, "parseoptdat: OPTION %d data (%d bytes)\n", option, len);

  switch (option) {
    case TELOPT_NAWS:
      if (len == 4) {
        cols = ntohs(*(unsigned short *) data);
        lines = ntohs(*(unsigned short *) (data + 2));
        if (cols != 0) ts->term.cols = cols;
        if (lines != 0) ts->term.lines = lines;
        fprintf(stderr, "parseoptdat: term(%d,%d)\n",ts->term.cols,ts->term.lines);
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
cleanup(int sig)
{
  fprintf(stderr,"cleanup telnetd\n");
  exit(2);
}

int telnet_session(socket,argc,argv)
int socket;
int argc;
char **argv;
{
  struct termstate ts;
  int ptfd[2];
  int fdm,fds;
  int n,rc;
  int last_was_cr;

  fd_set fd_in;

  struct sgttyb slave_orig_term_settings; 
  struct sgttyb new_term_settings;
  int i;

  /* telnet state */
  memset(&ts, 0, sizeof(struct termstate));
  ts.sock = socket;
  ts.state = STATE_NORMAL;
  ts.term.type = TERM_VT100;
  ts.term.cols = 80;
  ts.term.lines = 25;

  // Send initial options
  sendopt(&ts, WILL, T_ECHO);
  sendopt(&ts, WILL, T_SGA);
  sendopt(&ts, WONT, TELOPT_LINEMODE);
  sendopt(&ts, DO, TELOPT_NAWS);

  /* build pty pair */
  rc = create_pty(ptfd);
  if (rc < 0)
  {
    fprintf(stderr, "Error %d on create_pty\n", errno);
    exit(1);
  }

  fds = ptfd[0];
  fdm = ptfd[1];

  if (fork())
  {

    /* PARENT */

    /* Close the slave side of the PTY */
    close(fds);

    last_was_cr = 0;
    while (1)
    {

      FD_ZERO(&fd_in);
      FD_SET(socket, &fd_in);
      FD_SET(fdm, &fd_in);
	    rc = select(max(fdm,socket) + 1, &fd_in, NULL, NULL, NULL);
      if (rc < 0)
      {
          fprintf(stderr, "select() error %d\n", rc);
          exit(23);
      }

      /* read any socket input and send to slave input */
      if (FD_ISSET(socket, &fd_in))
      {
	    if (ts.bi.start == ts.bi.end)
	    {
          n = (int)read(socket, ts.bi.data, sizeof(ts.bi.data));
          if (n < 0)
            return(1);

          // Convert cr nul to cr lf.
          for (i = 0; i < n; ++i) {
            unsigned char ch = ts.bi.data[i];
            if (ch == 0 && last_was_cr) ts.bi.data[i] = '\n';
            last_was_cr = (ch == '\r');
          }

          ts.bi.start = ts.bi.data;
          ts.bi.end = ts.bi.data + n;
        }

	    /* Parse user input for telnet options */
	    parse(&ts);

	    /* Application ready to receive */
	    if (ts.bi.start != ts.bi.end)
	    {
		  n = (int)write(fdm, ts.bi.start, ts.bi.end - ts.bi.start);
		  if (n < 0)
	        return(2);
          ts.bi.start += n;
	     }
	   }

      /* read any slave output and send to socket */
      if (FD_ISSET(fdm, &fd_in))
      {
        // Data arrived from application
        if (ts.bo.start == ts.bo.end) 
        {
	      n = (int)read(fdm, ts.bo.data, sizeof(ts.bo.data));
          if (n < 0)
            return(3);

		  if (n == 0)
		  {
		    close(socket);
		    return(0);
		  }

	      ts.bo.start = ts.bo.data;
          ts.bo.end = ts.bo.data + n;
        }

        if (ts.bo.start != ts.bo.end) 
        {
	      n = (int)write(socket, ts.bo.start, ts.bo.end - ts.bo.start);
	      if (n < 0)
	        return(4);
	      ts.bo.start += n;
        }
      }
    }
  }
  else
  {
    /* CHILD */

    /* Close the master side of the PTY */
    close(fdm);

    /* Save the defaults parameters of the slave side of the PTY */
    rc = gtty(fds, &slave_orig_term_settings);

    /* Set RAW mode on slave side of PTY */
    new_term_settings = slave_orig_term_settings;
	new_term_settings.sg_flag |= RAW;
	//new_term_settings.sg_flag |= CRMOD;
	//new_term_settings.sg_flag |= XTABS;
	
  new_term_settings.sg_flag |= CBREAK;
	//new_term_settings.sg_flag |= CNTRL;
	new_term_settings.sg_flag &= ~ECHO;
    stty(fds, &new_term_settings);

    /* The slave side of the PTY becomes the standard input and outputs of the child process */
    close(0); /* Close standard input (current terminal) */
    close(1); /* Close standard output (current terminal) */
    close(2); /* Close standard error (current terminal) */

    dup2(fds, 0); /* PTY becomes standard input (0) */
    dup2(fds, 1); /* PTY becomes standard output (1) */
    dup2(fds, 2); /* PTY becomes standard error (2) */

    /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
    /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
    /* FIXME ioctl(0, TIOCSCTTY, 1); */
    n = control_pty(fds, PTY_INQUIRY, 0);
    control_pty(fds, PTY_SET_MODE, n | PTY_REMOTE_MODE);

    /* Now the original file descriptor is useless */
    close(fds);

    /* Make the current process a new session leader */
    setsid();

    signal(SIGHUP, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

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
    return 1;
  }

  fprintf(stderr, "EXIT\n");
  return 0;
}

int
main(argc,argv)
int argc;
char **argv;
{
  int newsock;
  int rc;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  socklen_t cli_addr_len;
  int reuse = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return 1;
  }

#ifndef __clang__
  opt.so_optlen = sizeof(reuse);
  opt.so_optdata = (char *)&reuse;
  setopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt);
#else
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
#endif

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(IPO_TELNET);
  rc = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
  if (rc < 0) {
    fprintf(stderr,"bind: failed\n");
    return errno;
  }

  rc = listen(sock, 5);
  if (rc < 0) {
    return errno;
  }

  fprintf(stderr, "telnetd started\n");
  state = RUNNING;
  while (1) {

    newsock = accept(sock, (struct sockaddr *) & cli_addr, &cli_addr_len);
    if (state == STOPPED) break;
    if (newsock < 0) {
      fprintf(stderr, "error %d (%s) in accept\n", errno, strerror(errno));
      return errno;
    }

#ifndef __clang__
    /* is turning off Nagle supported? */
#else
    setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
#endif

    if (fork())
    {
      fprintf(stderr, "client connected from %s\n", inet_ntoa(cli_addr.sin_addr.s_addr));
      sleep(1);
      close(newsock);
    }
    else
    {
      rc = telnet_session(newsock, argc, argv);
     fprintf(stderr, "client disconnected (%d) from %s\n", rc, inet_ntoa(cli_addr.sin_addr.s_addr));
    }
  }
  
  return 0;
}

