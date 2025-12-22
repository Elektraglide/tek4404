#include <stdio.h>
#include <errno.h> 
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/modes.h>
#include <sys/pty.h>
#include <sys/sgtty.h>

#include <varargs.h>
#include <net/socket.h>
#include <net/in.h>

#define CODEWIN 10
#define CMDWIN 23

typedef struct
{
    short y, h;
    char back[80 * 8];
} retainedwin;


char buffer[4096];
retainedwin regwindow;

int sockpair[2] = { 0,0 };
int readerpid = 0;
char* debugprocess[] = { "/bin/debug", NULL,NULL };

void dbgcleanup(sig)
int sig;
{
    int i, pid;

    if (readerpid)
    {
        kill(readerpid, SIGTERM);
        sleep(1);

        readerpid = 0;

#if 0  /* why does this hang indefinitely? */
        fprintf(stderr, "waiting to reap ttyreader\012\n");
        pid = wait(&i);
        fprintf(stderr, "pid=%d\012\n", pid);
#endif
    }
}

void cleanup_child(sig)
int sig;
{

    fprintf(stderr,"signal(%d): child of parent %d\n", sig, getpid());
    exit(sig);
}

int debug_session(ptfd, argv1)
int* ptfd;
char* argv1;
{
    int fdmaster, fdslave;
    int n, rc;
    int pid;

    struct sgttyb slave_orig_term_settings;
    struct sgttyb new_term_settings;

    /* build pty pair */
    rc = create_pty(ptfd);
    if (rc < 0)
    {
        fprintf(stderr, "Error %d on create_pty\n", errno);
        exit(1);
    }
    n = control_pty(ptfd[1], PTY_INQUIRY, 0);
    n |= (PTY_READ_WAIT | PTY_HANDSHAKE_MODE); 
    n &= ~(PTY_REMOTE_MODE);
    control_pty(ptfd[1], PTY_SET_MODE, n);

    fdslave = ptfd[0];
    fdmaster = ptfd[1];

    pid = fork();
    if (pid)
    {
        /* Close the slave side of the PTY */
        close(fdslave);
    }
    else
    {
        /* CHILD */
        signal(SIGHUP, cleanup_child);
        signal(SIGINT, cleanup_child);
        signal(SIGQUIT, cleanup_child);
        signal(SIGTERM, cleanup_child);
        signal(SIGPIPE, cleanup_child);
        signal(SIGDEAD, SIG_IGN);

        /* Close the master side of the PTY */
        close(fdmaster);

        /* Save the defaults parameters of the slave side of the PTY */
        rc = gtty(fdslave, &slave_orig_term_settings);
        new_term_settings = slave_orig_term_settings;
        new_term_settings.sg_flag &= ~ECHO;
        new_term_settings.sg_flag |= RAW;
#if 0
        new_term_settings.sg_prot |= ESC;
        new_term_settings.sg_prot |= OXON;
        new_term_settings.sg_prot |= TRANS;
        new_term_settings.sg_prot |= ANY;
#endif
        stty(fdslave, &new_term_settings);

        dup2(fdslave, 0); /* PTY becomes standard input (0) */
        dup2(fdslave, 1); /* PTY becomes standard output (1) */
        dup2(fdslave, 2); /* PTY becomes standard error (2) */

#ifndef __clang__
        /* calls pty_make_controlling_terminal */
        system_control(2, fdslave);
#endif

        /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
        /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
        /* FIXME ioctl(0, TIOCSCTTY, 1); */

        /* Now the original file descriptor is useless */
        close(fdslave);

        /* Execution of the session shell */
        debugprocess[1] = argv1;
        rc = execvp(debugprocess[0], debugprocess);
        if (rc < 0)
        {
            fprintf(stderr, "Error %d on exec\n", errno);
        }

        /* error */
        return -1;
    }

    return pid;
}


void closedown()
{


  exit(0);
}

int logit(_varargs)
int _varargs;
{
    va_list p;
    char* fmt;
    unsigned int val1;
    unsigned int val2;
    unsigned int val3;
    unsigned int val4;

    va_start(p);
    fmt = va_arg(p, char*);

    val1 = va_arg(p, unsigned int);
    val2 = va_arg(p, unsigned int);
    val3 = va_arg(p, unsigned int);
    val4 = va_arg(p, unsigned int);
    va_end(p);

    printf("\033[25;33r\033[8;1H");
    printf(fmt, val1, val2, val3, val4);

    /* we dont know what is the current window, so.. */
    printf("\033[1;20r");

    return 0;
}


int sighandler(sig)
int sig;
{
    logit("\033[1;33rclosing down\n");
    dbgcleanup(sig);
    closedown();

    signal(sig, sighandler);
}

int readdebugline(master, buf, buflen)
int master;
char* buf;
int buflen;
{
    int n, rc, total;

    /* ensure it has been consumed */
    n = control_pty(master, PTY_INQUIRY, 0);
    while (n & PTY_INPUT_QUEUED)
    {
        yield_CPU();
        n = control_pty(master, PTY_INQUIRY, 0);
    }

    total = 0;
    rc = read(master, buf, buflen);
    if (rc > 0)
    {
        buf += rc;
        buflen -= rc;
        total += rc;
    }

    return total;
}

int readdebug(master,buf, buflen)
int master;
char* buf;
int buflen;
{
    int n,rc, total;

    /* ensure it has been consumed */
    n = control_pty(master, PTY_INQUIRY, 0);
    while (n & PTY_INPUT_QUEUED)
    {
        yield_CPU();
        n = control_pty(master, PTY_INQUIRY, 0);
    }

    total = 0;
    while (1)
    {
        rc = read(master, buf, buflen);
        if (rc > 0)
        {
            buf += rc;
            buflen -= rc;
            total += rc;

            /* debugger responds with ? when done */
            if (buf[-2] == 0x3f && buf[-1] == 0x20)
            {
                buf[-2] = 0;
                break;
            }
        }
    }
    *buf = 0;
    return total;
}

printdiffclear(win, buf)
retainedwin* win;
char* buf;
{
    char *srcbeg,*dstbeg;
    short i;
    int linenum;

    linenum = win->y;

    srcbeg = buf;
    dstbeg = win->back;

    while (linenum < win->h)
    {
        char* src;
        char* dst;
        char* srcend;
        char* dstend;
        char* startsrc;
        short inside_run;

        srcend = strchr(srcbeg, '\n');
        dstend = strchr(dstbeg, '\n');

        /*stop if no line */
        if (!srcend)
            break;

        if (!dstend)
        {
            dstend = dstbeg + (srcend - srcbeg);
        }

        /* compare pair of lines */
        src = srcbeg;
        dst = dstbeg;
        inside_run = 0;
        while (src < srcend)
        {
            char c, d;
            int offset;

            c = *src++;
            d = *dst++;

            if (c != d)
            {
                if (inside_run == 0)
                {
                    startsrc = src-1;
                    inside_run = 1;
                }
            }
            else
            if (inside_run)
            {
                c = src[0];
                src[0] = '\0';
                offset = startsrc - srcbeg + 1;
                printf("\033[%d;%dH%s", linenum, offset, startsrc);
                src[0] = c;

                inside_run = 0;
            }
        }

        if (inside_run)
        {
            char c;
            int offset;

            c = src[0];
            src[0] = '\0';
            offset = startsrc - srcbeg + 1;
            printf("\033[%d;%dH%s", linenum, offset, startsrc);
            src[0] = c;
        }

        /* if its shorter, clear out old */
        if (dst < dstend)
            printf("\033[0K");

        /* start of next lines */
        srcbeg = srcend + 1;
        dstbeg = dstend + 1;
        linenum++;
    }

    strcpy(win->back, buf);
}

printclear(buf, n)
char* buf;
int n;
{
    char *ptr;

    ptr = strtok(buf, "\n");
    while (--n >= 0 && ptr)
    {
        printf(ptr);
        printf("\033[0K\n");
        ptr = strtok(NULL, "\n");
    }
}

printlastclear(buf, n)
char* buf;
int n;
{
    char* ptr;
    char* curr;

    curr = ptr = strtok(buf, "\n");
    while (--n > 0 && ptr)
    {
        curr = ptr;
        ptr = strtok(NULL, "\n");
    }
    printf(curr);
    printf("\033[0K");
}

main(argc, argv)
int argc;
char **argv;
{
    int ptfd[2];
	int rc;
	char *ptr, ch, input[32];
    int master, slave;
    int refresh;

    signal(SIGINT, sighandler);


  readerpid = debug_session(ptfd, argv[1]);
  if (readerpid < 0)
  {
    fprintf(stderr, "error %d (%s) in popen\n", errno, strerror(errno));
    closedown();
  }

  master = ptfd[1];
  slave = ptfd[0];

  printf("\033[1;32r\033[2J\033[%d;1H",CMDWIN-1);
  printf("===== Dbg v1.0 %s [pid:%d] =====\n", argv[1], readerpid);
  sleep(1);
  rc = readdebug(master, buffer, sizeof(buffer));
  printf("\033[32;1H");
  printf(buffer);

  /* retained window for reg display */
  regwindow.y = 1;
  regwindow.h = 7;

  refresh = 1;
  while (1)
  {
      /* make a 10 line scroll region */
      printf("\033[%d;%dr\033[10;1H>", CMDWIN, CMDWIN+10);
      fflush(stdout);
      rc = (int)read(0, input, sizeof(input));
      if (rc > 1) ch = input[0];
      if (ch == '?')
      {
          printf("x - create process\n");
          printf("s - singlestep\n");
          printf("g - go\n");
          printf("t - trace\n");
          printf("q - quit\n");
      }
      if (ch == 'q')
      {
          sighandler(0);

          write(master, "q\n", 2);
          readdebug(master, buffer, sizeof(buffer));
          printf(buffer);
      }
      if (ch == 'x')
	  {
			write(master, "x\n", 2);
            readdebug(master, buffer, sizeof(buffer));
			printf(buffer);
            refresh = 1;
      }
	  if (ch == 'g')
	  {
		  write(master, "g\n", 2);
          readdebug(master, buffer, sizeof(buffer));
		  printf(buffer);
	  }
      if (ch == 's')
      {
          write(master, "s\n", 2);
          readdebug(master, buffer, sizeof(buffer));
          /* printf(buffer); */
      }
      if (ch == 't')
      {
          write(master, "T\n", 2);

          /* show lines as they are produced */
          while (1)
          {
              readdebugline(master, buffer, sizeof(buffer));
              if (buffer[0] == 0x3f)
                  break;
              printf(buffer);
          }
      }

	  /* refresh top of screen */
      printf("\033[1;20r\033[1;1H");

      /* get register state */
	  write(master, "r\n", 2);
	  rc = readdebug(master, buffer, sizeof(buffer));
      if (refresh)
      {
          memcpy(regwindow.back, buffer, sizeof(regwindow.back));
          printclear(regwindow.back, 5);
      }
      else
      {
          printdiffclear(&regwindow, buffer);
      }

      /* get disassembly (more than we need because we want N line) */
      write(master, "i .-0,20\n", 9);
      rc = readdebug(master, buffer, sizeof(buffer));
      /* full redraw or 1 line redraw */
      if (1 || refresh)
      {
          printf("\033[%d;1H",CODEWIN);
          printclear(buffer, 7);
      }
      else
      {
          /* scroll region 1 line */
          printf("\033[%d;%dr\033[17;1H",CODEWIN,CODEWIN+7);
          printlastclear(buffer, 8);
          printf("\n");
      }

      refresh = 0;
  }
  
   /* restore */
  dbgcleanup(0);
}
