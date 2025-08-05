#include <stdio.h>
#include <errno.h> 
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/modes.h>
#include <sys/pty.h>
#include <sys/sgtty.h>

#include <net/socket.h>
#include <net/in.h>

char buffer[4096];

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
        system_control(2);
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

int sighandler(sig)
int sig;
{
    printf("\033[1;32r\033[31;1H");
    fprintf(stderr, "closing down\n");
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

printclear(buf)
char* buf;
{
    char *ptr,*eol;

    ptr = strtok(buf, "\n");
    while (ptr)
    {
        printf(ptr);
        printf("\033[0K\n");
        ptr = strtok(NULL, "\n");
    }
}

main(argc, argv)
int argc;
char **argv;
{
    int ptfd[2];
	int rc;
	char *ptr, ch, input[32];
    int master, slave;

    signal(SIGINT, sighandler);


  readerpid = debug_session(ptfd, argv[1]);
  if (readerpid < 0)
  {
    fprintf(stderr, "error %d (%s) in popen\n", errno, strerror(errno));
    closedown();
  }

  master = ptfd[1];
  slave = ptfd[0];

  printf("\033[2J\033[24;1H");
  printf("===== Dbg v1.0 %s [pid:%d] =====\n", argv[1], readerpid);
  sleep(2);
  rc = readdebug(master, buffer, sizeof(buffer));
  printf(buffer);
 
  while (1)
  {
      /* make a 8 line scroll region */
      printf("\033[25;33r\033[8;1H>"); fflush(stdout);
      rc = (int)read(0, input, sizeof(input));
      ch = input[0];
	  if (ch == 'x')
	  {
			write(master, "x\n", 2);
            readdebug(master, buffer, sizeof(buffer));
			printf(buffer);
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
          printf(buffer);
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

	  write(master, "r\n", 2);
	  rc = readdebug(master, buffer, sizeof(buffer));
      printclear(buffer);

      printf("\033[7;1H"); 
      
      write(master, "i .-0,16\n", 9);
	  rc = readdebug(master, buffer, sizeof(buffer));
	  printclear(buffer);

  }
  
   /* restore */
  dbgcleanup(0);
}
