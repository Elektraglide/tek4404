#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/pty.h>
#include <sys/sgtty.h>
#include <sys/fcntl.h>
#include <signal.h>

#include <font.h>
#include <graphics.h>
#include <net/ftype.h>

#ifdef __clang__

#define TEK_GFX
#include "uniflexshim.h"

#else

typedef int fd_set;
#define FD_SET(A,SET)	*SET |= (1<<A)
#define FD_CLR(A,SET)	*SET &= ~(1<<A)
#define FD_ZERO(SET)	*SET = 0
#define FD_ISSET(A,SET)	(*SET & (1<<A))

void setsid() {}

#endif

#include "vtemu.h"

/**********************************/
#define WINTITLEBAR 16
#define WINBORDER 8

struct BBCOM bb;

typedef struct _win
{
  struct _win *next;
  char title[32];
  struct RECT windowrect;
  struct RECT contentrect;
  int pfd[2];
  int pid;
  
  /* term emulator */
  VTemu vt;
} Window;

Window *winchain;
Window allwindows[32];
int numwindows = 0;
char *winprocess[] = {"bash", NULL};
char *logprocess[] = {"tail", "-f", "/var/log/system.log", NULL};


void RectInset(struct RECT *r, int d)
{
  r->x += d;
  r->y += d;
  r->w -= d * 2;
  r->h -= d * 2;
}

int window_session(ptfd, islogger)
int *ptfd;
int islogger;
{
  int fdm,fds;
  int n,rc;
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

  fds = ptfd[0];
  fdm = ptfd[1];

  pid = fork();
  if (pid)
  {
	
    /* PARENT */

    /* Close the slave side of the PTY */
    close(fds);
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

    /* Execution of the session shell */
    rc = islogger ? execvp(logprocess[0], logprocess) : execvp(winprocess[0], winprocess);
    if (rc < 0)
    {
      fprintf(stderr, "Error %d on exec\n", errno);
	}

    /* error */
    return -1;
  }

  return pid;
}

int WindowTop(Window *win)
{
  Window *awin,*prevwin;

  if (win != winchain)
  {
    /* find it and put at the front */
    prevwin = NULL;
    awin = winchain;
    while(awin)
    {
      if (awin == win)
      {
        if (prevwin)
          prevwin->next = win->next;

        win->next = winchain;
        winchain = win;
        break;
      }
      
      prevwin = awin;
      awin = awin->next;
    }
    
    fprintf(stderr, "topwindow: %s\n", win->title);
  }
  
  bb.cliprect = win->windowrect;
  win->vt.dirty |= 2;

  return 0;
}

int WindowCreate(title, x, y, islogger)
char *title;
int x,y;
int islogger;
{
  int pid;
  Window *win = allwindows + numwindows;
  
  /* term emu */
	win->vt.cols = 80;
	win->vt.rows = 25;
  VTreset(&win->vt);

  /* bounds of content */
	win->contentrect.x = x;
	win->contentrect.y = y;
	win->contentrect.w = win->vt.cols*7;
	win->contentrect.h = win->vt.rows*9;

  /* outset for window border */
  win->windowrect = win->contentrect;
  RectInset(&win->windowrect, -WINBORDER);
  win->windowrect.y -= WINTITLEBAR - WINBORDER;
  win->windowrect.h += WINTITLEBAR - WINBORDER;

	// create a pty process; first window is always a logger
	pid = window_session(win->pfd, islogger);
  if (pid > 0)
  {
    fprintf(stderr, "WindowCreate with pid%d\n", pid);
    
    win->pid = pid;
    sprintf(win->title, "%s [pid:%d]", title, pid);

    /* needs rendering */
    win->vt.dirty = 3;

    /* link it */
    win->next = winchain;
    winchain = win;

    numwindows++;
    WindowTop(win);

		return numwindows;
	}
	else
	{
		/* failed to create window */
		return -1;
	}
}


int WindowOutput(wid, msg, n)
int wid;
char *msg;
int n;
{
  Window *win = allwindows + wid;

  VToutput(&win->vt, msg, n, win->pfd[1]);

  return 0;
}

void WindowMove(win, x, y)
Window *win;
int x, y;
{

  if (x != win->windowrect.x || win->windowrect.y != y)
  {
    win->windowrect.x = x;
    win->windowrect.y = y;

    /* update window content too */
    win->contentrect.x = win->windowrect.x + WINBORDER;
    win->contentrect.y = win->windowrect.y + WINTITLEBAR;

    win->vt.dirty |= 2;
  }
  
  if (win == winchain)
  {
    bb.cliprect = win->windowrect;
  }
}

int WindowRender(win, forcedirty)
Window *win;
int forcedirty;
{
  static long newtime,oldtime = 0;
  struct RECT r;
  struct POINT origin;
  int j,k;
  char line[128];
  char cursor;
  
  if (forcedirty || win->vt.dirty)
  {
    if (forcedirty || (win->vt.dirty & 2))
    {
      // render frame
      bb.halftoneform = &WhiteMask;
      RectDrawX(&win->windowrect, &bb);

      bb.halftoneform = &BlackMask;
      r = win->windowrect;
      r.h = WINTITLEBAR;
      RectBoxDrawX(&r, 1, &bb);
      r = win->windowrect;
      r.y += WINTITLEBAR;
      r.h -= WINTITLEBAR;
      RectBoxDrawX(&r, 1, &bb);

#ifdef CLOSEBOX
      /* render closebox */
      bb.halftoneform = &VeryLightGrayMask;
      r = win->windowrect;
      r.w = 16;
      r.h = 16;
      RectDrawX(&r, &bb);
      bb.halftoneform = &BlackMask;
      RectBoxDrawX(&r, 1, &bb);
#endif

      /* render centered title */
      j = StringWidth(win->title, NULL);
      origin.x = win->windowrect.x + win->windowrect.w / 2 - j/2;
      origin.y = win->windowrect.y + 4;
      StringDrawX(win->title, &origin, &bb);
    }

    // render contents
    if (forcedirty || (win->vt.dirty & 1))
    {
      origin.x = win->contentrect.x;
      origin.y = win->contentrect.y;
      for(j=0; j<win->vt.rows; j++)
      {
        // NB we may have embedded \0 where cursor has been warped, so we need to handle this
        for(k=0; k<win->vt.cols; k++)
        {
          line[k] = win->vt.buffer[win->vt.cols*j+k];
          if (line[k] == 0)
            line[k] = ' ';
        }
        line[win->vt.cols] = '\0';
        
        StringDraw(line, &origin);
        origin.y += 9;
      }
    }
  }

  /* focus window has blinking cursor */
  if ((win->vt.dirty & 2) == 0)
  {
    if ((win == winchain) && (win->vt.hidecursor == 0))
    {
      newtime = EGetTime() / 256;
      if (oldtime != newtime)
      {
        RCToRect(&r, win->vt.cy, win->vt.cx);
        origin.x = win->contentrect.x + r.x;
        origin.y = win->contentrect.y + r.y;
        cursor = win->vt.buffer[win->vt.cols*win->vt.cy+win->vt.cx];
        if (!cursor)
          cursor = ' ';
        CharDraw(newtime & 1 ? '_' : cursor, &origin);

        oldtime = newtime;
      }
    }
  }
  
  win->vt.dirty = 0;

  return 0;
}

void Paint(win, r)
Window *win;
struct RECT *r;
{
  struct RECT overlap;
  struct QUADRECT quadrect;
  int i;

  /* paint desktop */
  if (win == NULL)
  {
    bb.cliprect.x = 0;
    bb.cliprect.y = 0;
    bb.cliprect.w = ScrWidth;
    bb.cliprect.h = ScrHeight;
    bb.halftoneform =  &GrayMask;
    RectDrawX(r, &bb);
  }
  else
  {
    /* repaint self */
    RectIntersect(r, &win->windowrect, &overlap);
    if (overlap.w && overlap.h)
    {
      bb.cliprect = overlap;
      WindowRender(win, TRUE);
    }

    /* recurse for parts outside this window */
    RectAreasOutside(r, &win->windowrect, &quadrect);
    for(i=0; i<quadrect.next; i++)
    {
      Paint(win->next, &quadrect.region[i]);
    }
  }
}

void WindowDestroy(wid)
int wid;
{
  Window *awin,*prevwin;
  struct RECT oldrect;
  int i;

  /* unlink it */
  prevwin = NULL;
  awin = winchain;
  for(i=0; i<numwindows; i++)
  {
    if (awin == &allwindows[wid])
    {
      if (prevwin)
        prevwin->next = allwindows[wid].next;
      else
        winchain = allwindows[wid].next;
        
      break;
    }
    
    prevwin = awin;
    awin = awin->next;
  }

  oldrect = allwindows[wid].windowrect;
  memcpy(&allwindows[wid], &allwindows[numwindows-1], sizeof(Window));
  numwindows--;
  
  if (numwindows > 0)
    WindowTop(&allwindows[0]);
    
  Paint(winchain, &oldrect);
}

void
cleanup(int sig)
{
  exit(2);
}

int
main(int argc, char *argv[])
{
  int n,i,j,k,rc;
  fd_set fd_in;
  char inputbuffer[4096];
  struct timeval timeout;
  Window *win;
  struct RECT r,r2;
  struct POINT origin,offset, p;
  struct QUADRECT quadrect;
  struct sgttyb new_term_settings;

  /* how do we detect child is dead? */
  //signal(SIGHUP, cleanup);
  //signal(SIGDEAD, cleanup);
  //signal(SIGABORT, cleanup);

  InitGraphics(TRUE);

  /* for controllering clipping etc */
  BbcomDefault(&bb);

  /* clear desktop */
  bb.halftoneform = &GrayMask;
  RectDrawX(&bb.cliprect, &bb);

  EventEnable();
  SetKBCode(0);

  // window chain
  winchain = NULL;
  numwindows = 0;
  
  // create a logger of /var/log/system.log OR run a shell
  //WindowCreate("SysLog", 50, 50, TRUE);
  WindowCreate("Console", 50, 50, FALSE);

  /* CBREAK input */
  rc = gtty(0, &new_term_settings);
  new_term_settings.sg_flag |= CBREAK;
  stty(0, &new_term_settings);

#ifdef __clang__
  /* modern OS no longer support stty as above */
  struct termios t = {};
  tcgetattr(0, &t);
  t.c_lflag &= ~ICANON;
  tcsetattr(0, TCSANOW, &t);
#endif

  /* mainloop */
  while (1)
  {
    /* check any outputs from master sides and track highest fd */
    FD_ZERO(&fd_in);
    FD_SET(0, &fd_in);
    n = 0;
    for(i=0; i<numwindows; i++)
    {
      FD_SET(allwindows[i].pfd[1], &fd_in);
      if (allwindows[i].pfd[1] > n)
        n = allwindows[i].pfd[1];
    }

    /* 20Hz updating */
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    rc = select(n + 1, &fd_in, NULL, NULL, &timeout);
    if (rc < 0 && errno != EINTR) /* what is Uniflex equiv? */
    {
        fprintf(stderr, "select() error %d\n", errno);
        exit(23);
    }
	  else
	  if (rc > 0)
	  {
		  /* read any stdio input and send to topwindow input */
		  if (FD_ISSET(0, &fd_in))
		  {
			  n = (int)read(0, inputbuffer, sizeof(inputbuffer));
        if (winchain)
          n = (int)write(winchain->pfd[1], inputbuffer, n);
		  }

      /* read any output from window process */
		  for(i=0; i<numwindows; i++)
		  {
        if (FD_ISSET(allwindows[i].pfd[1], &fd_in))
        {
          n = (int)read(allwindows[i].pfd[1], inputbuffer, sizeof(inputbuffer));

          /* window process terminated */
          if (n > 0)
          {
            WindowOutput(i, inputbuffer, n);
          }
          else
          if (n == 0)
          {
            fprintf(stderr,"window%d destroyed\n", i);
            WindowDestroy(i);
            break;
          }
        }
		  }
	  }
	  else  /* manage windows */
	  {
      /* tek4404 event system only used for keypress input */
      union EVENTUNION ev = (union EVENTUNION)EGetNext();
      if (ev.estruct.etype == E_PRESS)
      {
        char ch = ev.estruct.eparam;
        if (winchain)
          n = (int)write(winchain->pfd[1], &ch, 1);
      }

      if (GetButtons() & M_LEFT)
      {
        /* walk from frontmost windows back to find which we are over */
        GetMPosition(&origin);
        win = winchain;
        while (win)
        {
          if (RectContainsPoint(&win->windowrect, &origin))
          {
            WindowTop(win);

            /* record offset from window boundary */
            offset.x = origin.x - win->windowrect.x;
            offset.y = origin.y - win->windowrect.y;

            /* track drag */
            while (GetButtons() & M_LEFT)
            {
              GetMPosition(&p);

              r2 = win->windowrect;
              WindowMove(win, p.x - offset.x, p.y - offset.y);

              /* repaint only difference of old and new */
              RectAreasOutside(&r2, &win->windowrect, &quadrect);
              for(j=0; j<quadrect.next; j++)
                Paint(winchain, &quadrect.region[j]);
              WindowRender(win, FALSE);
            }

            /* repaint union of old and new */
            r.x = origin.x - offset.x;
            r.y = origin.y - offset.y;
            r.w = win->windowrect.w;
            r.h = win->windowrect.h;
            RectMerge(&r, &win->windowrect, &r2);
            Paint(winchain, &r2);

            win->vt.dirty |= 1;
            WindowRender(win, FALSE);
            
            /* now has focus */
            break;
          }
          
          win = win->next;
        }
      }
      else
      if (GetButtons() & M_RIGHT)
      {
          // pop up menu

          while (GetButtons() & M_RIGHT);

          GetMPosition(&origin);
          WindowCreate("Window", origin.x - 32, origin.y - 32, FALSE);

      }

      win = winchain;
		  while(win)
		  {
        WindowRender(win, FALSE);
        win = win->next;
		  }

	  }
  }
  
  ExitGraphics();
}
