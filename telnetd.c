#include <stdio.h>
#include <string.h>
#include <errno.h> 
#include <sys/pty.h> 
#include <sys/sgtty.h>
#include <sys/fcntl.h>

#include <net/telnet.h>
#include <net/select.h>

#ifdef __clang__

#include "uniflexshim.h"

#else

typedef int fd_set;
#define FD_SET(A,SET)	*SET |= (1<<A)
#define FD_CLR(A,SET)	*SET &= ~(1<<A)
#define FD_ZERO(SET)	*SET = 0
#define FD_ISSET(A,SET)	(*SET & (1<<A))

void setsid() {}

#endif

/*

native: cc +v pty_test.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include pty_test.c 

*/

/***********************************/
int myntohs(data)
unsigned short data;
{
	return(data);
}

char *mystrdup(s)
char *s;
{
  int len;
  char *dup;

  len = strlen(s);
  dup = (char *)malloc(len + 1);
  strcpy(dup, s);
  return dup;
}

/***********************************/


/* States */

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_OPT    2
#define STATE_SB     3
#define STATE_OPTDAT 4
#define STATE_SE     5

/* Special telnet characters */

#define TELNET_SE    240   /* End of subnegotiation parameters */
#define TELNET_NOP   241   /* No operation */
#define TELNET_MARK  242   /* Data mark */
#define TELNET_BRK   243   /* Break */
#define TELNET_IP    244   /* Interrupt process */
#define TELNET_AO    245   /* Abort output */
#define TELNET_AYT   246   /* Are you there */
#define TELNET_EC    247   /* Erase character */
#define TELNET_EL    248   /* Erase line */
#define TELNET_GA    249   /* Go ahead */
#define TELNET_SB    250   /* Start of subnegotiation parameters */
#define TELNET_WILL  251   /* Will option code */
#define TELNET_WONT  252   /* Won't option code */
#define TELNET_DO    253   /* Do option code */
#define TELNET_DONT  254   /* Don't option code */
#define TELNET_IAC   255   /* Interpret as command */

/* Telnet options */

#define TELOPT_TRANSMIT_BINARY      0  /* Binary Transmission (RFC856) */
#define TELOPT_ECHO                 1  /* Echo (RFC857) */
#define TELOPT_SUPPRESS_GO_AHEAD    3  /* Suppress Go Ahead (RFC858) */
#define TELOPT_STATUS               5  /* Status (RFC859) */
#define TELOPT_TIMING_MARK          6  /* Timing Mark (RFC860) */
#define TELOPT_NAOCRD              10  /* Output Carriage-Return Disposition (RFC652) */
#define TELOPT_NAOHTS              11  /* Output Horizontal Tab Stops (RFC653) */
#define TELOPT_NAOHTD              12  /* Output Horizontal Tab Stop Disposition (RFC654) */
#define TELOPT_NAOFFD              13  /* Output Formfeed Disposition (RFC655) */
#define TELOPT_NAOVTS              14  /* Output Vertical Tabstops (RFC656) */
#define TELOPT_NAOVTD              15  /* Output Vertical Tab Disposition (RFC657) */
#define TELOPT_NAOLFD              16  /* Output Linefeed Disposition (RFC658) */
#define TELOPT_EXTEND_ASCII        17  /* Extended ASCII (RFC698) */
#define TELOPT_TERMINAL_TYPE       24  /* Terminal Type (RFC1091) */
#define TELOPT_NAWS                31  /* Negotiate About Window Size (RFC1073) */
#define TELOPT_TERMINAL_SPEED      32  /* Terminal Speed (RFC1079) */
#define TELOPT_TOGGLE_FLOW_CONTROL 33  /* Remote Flow Control (RFC1372) */
#define TELOPT_LINEMODE            34  /* Linemode (RFC1184) */
#define TELOPT_AUTHENTICATION      37  /* Authentication (RFC1416) */

/* Globals */

#define STOPPED 0
#define RUNNING 1

int port;
char *pgm;
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

  buf[0] = TELNET_IAC;
  buf[1] = (unsigned char) code;
  buf[2] = (unsigned char) option;
  write(ts->sock, buf, 3);
}

void parseopt(ts, code, option)
struct termstate *ts;
int code;
int option;
{

  switch (option) {
    case TELOPT_ECHO:
    case TELOPT_SUPPRESS_GO_AHEAD:
    case TELOPT_NAWS:
      break;

    case TELOPT_TERMINAL_TYPE:
    case TELOPT_TERMINAL_SPEED:
      sendopt(ts, TELNET_DO, option);
      break;

    default:
      if (code == TELNET_WILL || code == TELNET_WONT) {
        sendopt(ts, TELNET_DONT, option);
      } else {
        sendopt(ts, TELNET_WONT, option);
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

  /* fprintf(stderr, "OPTION %d data (%d bytes)\n", option, len); */

  switch (option) {
    case TELOPT_NAWS:
      if (len == 4) {
        cols = myntohs(*(unsigned short *) data);
        lines = myntohs(*(unsigned short *) (data + 2));
        if (cols != 0) ts->term.cols = cols;
        if (lines != 0) ts->term.lines = lines;
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
        if (c == TELNET_IAC) {
          ts->state = STATE_IAC;
        } else {
          *q++ = c;
        }
        break;

      case STATE_IAC:
        switch (c) {
          case TELNET_IAC:
            *q++ = c;
            ts->state = STATE_NORMAL;
            break;

          case TELNET_WILL:
          case TELNET_WONT:
          case TELNET_DO:
          case TELNET_DONT:
            ts->code = c;
            ts->state = STATE_OPT;
            break;

          case TELNET_SB:
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
        if (c == TELNET_IAC) {
          ts->state = STATE_SE;
        } else if (ts->optlen < sizeof(ts->optdata)) {
          ts->optdata[ts->optlen++] = c;
        }
        break;

      case STATE_SE:
        if (c == TELNET_SE) parseoptdat(ts, ts->code, ts->optdata, ts->optlen);
        ts->state = STATE_NORMAL;
        break;
    } 
  }

  ts->bi.end = q;
}

/************************************************/

int telnet_session(socket,argc,argv)
int socket;
int argc;
char **argv;
{
  struct termstate ts;
  int ptfd[2];
  int fdm,fds;
  char input[150];
  int n,rc;

  fd_set fd_in;

  struct sgttyb slave_orig_term_settings; 
  struct sgttyb new_term_settings;
  char **child_av;
  int i;

  /* telnet state */
  memset(&ts, 0, sizeof(struct termstate));
  ts.sock = socket;
  ts.state = STATE_NORMAL;
  ts.term.type = TERM_VT100;
  ts.term.cols = 80;
  ts.term.lines = 25;

  // Send initial options
  sendopt(&ts, TELNET_WILL, TELOPT_ECHO);
  sendopt(&ts, TELNET_WILL, TELOPT_SUPPRESS_GO_AHEAD);
  sendopt(&ts, TELNET_WONT, TELOPT_LINEMODE);
  sendopt(&ts, TELNET_DO, TELOPT_NAWS);

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

    while (1)
    {

      FD_ZERO(&fd_in);
      FD_SET(socket, &fd_in);
      FD_SET(fdm, &fd_in);
	  rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
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
          n = read(socket, ts.bi.data, sizeof(ts.bi.data));
          if (n < 0)
            return(1);

          ts.bi.start = ts.bi.data;
          ts.bi.end = ts.bi.data + n;
        }

	    /* Parse user input for telnet options */
	    parse(&ts);

	    /* Application ready to receive */
	    if (ts.bi.start != ts.bi.end)
	    {
		  n = write(fdm, ts.bi.start, ts.bi.end - ts.bi.start);
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
	      n = read(fdm, ts.bo.data, sizeof(ts.bo.data));
          if (n < 0)
            return(3);

	      ts.bo.start = ts.bo.data;
          ts.bo.end = ts.bo.data + n;
        }

        if (ts.bo.start != ts.bo.end) 
        {
	      n = write(socket, ts.bo.start, ts.bo.end - ts.bo.start);
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
	//new_term_settings.sg_flag |= CBREAK;
	//new_term_settings.sg_flag |= CNTRL;
	new_term_settings.sg_flag &= ~ECHO;
    stty(fds, &new_term_settings);

    /* The slave side of the PTY becomes the standard input and outputs of the child process */
    close(0); /* Close standard input (current terminal) */
    close(1); /* Close standard output (current terminal) */
    close(2); /* Close standard error (current terminal) */

    dup(fds); /* PTY becomes standard input (0) */
    dup(fds); /* PTY becomes standard output (1) */
    dup(fds); /* PTY becomes standard error (2) */

    /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
    /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
    /* FIXME ioctl(0, TIOCSCTTY, 1); */
    n = control_pty(fds, PTY_INQUIRY, 0);
    control_pty(fds, PTY_SET_MODE, n | PTY_REMOTE_MODE);

    /* Now the original file descriptor is useless */
    close(fds);

    /* Make the current process a new session leader */
    setsid();

    /* Execution of the program */
    {
      /* Build the command line */
      child_av = (char **)malloc(argc * sizeof(char *));
      for (i = 1; i < argc; i ++)
      {
        child_av[i - 1] = mystrdup(argv[i]);
      }
      child_av[i - 1] = NULL;

      /* launch cmd */
      rc = execvp(child_av[0], child_av);
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

char *
netaddr(addr)
unsigned long addr;
{
  static char ipstring[16];

  sprintf(ipstring, "%02x:%02x:%02x:%02x", (int)(addr>>24)&255,(int)(addr>>16)&255,(int)(addr>>8)&255,(int)(addr>>0)&255);
  return ipstring;
}

main(argc,argv)
int argc;
char **argv;
{
  int newsock;
  int rc;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  socklen_t cli_addr_len;

  port = 8023;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);
  rc = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
  if (rc < 0) {
    return 1;
  }

  rc = listen(sock, 5);
  if (rc < 0) {
    return 1;
  }

  fprintf(stderr, "telnetd started on port %d\n", port);
  state = RUNNING;
  while (1) {

    newsock = accept(sock, (struct sockaddr *) & cli_addr, &cli_addr_len);
    if (state == STOPPED) break;
    if (newsock < 0) {
      fprintf(stderr, "error %d (%s) in accept\n", errno, strerror(errno));
      return 1;
    }

    fprintf(stderr, "client connected from %s\n", netaddr(cli_addr.sin_addr.s_addr));

    //setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));

    telnet_session(newsock, argc, argv);

    fprintf(stderr, "client disconnected from %s\n", netaddr(cli_addr.sin_addr.s_addr));
  }
}


