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

#include "vtemu.h"

extern int rand();

/**********************************/
#define WINTITLEBAR 24
#define WINBORDER 8
#define CLOSEBOX

struct BBCOM bb;

typedef struct _win
{
  struct _win *next;
  char title[32];
  struct RECT oldrect;
  struct RECT windowrect;
  struct RECT contentrect;
  int pfd[2];
  int pid;
  
  /* term emulator */
  VTemu vt;
} Window;

Window *wintopmost;
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
	
	  new_term_settings.sg_flag |= CBREAK;
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

  if (win != wintopmost)
  {
    /* find it and put at the front */
    prevwin = NULL;
    awin = wintopmost;
    while(awin)
    {
      if (awin == win)
      {
        if (prevwin)
          prevwin->next = win->next;

        win->next = wintopmost;
        wintopmost = win;
        break;
      }
      
      prevwin = awin;
      awin = awin->next;
    }
    
    fprintf(stderr, "topwindow: %s\n", win->title);

    win->vt.dirty |= 2;
  }
  
  bb.cliprect = win->windowrect;

  return 0;
}

int WindowCreate(title, x, y, islogger)
char *title;
int x,y;
int islogger;
{
  int pid;
  Window *win = allwindows + numwindows;
  struct RECT r;

  /* term emu */
	win->vt.cols = 80;
	win->vt.rows = 25;
  VTreset(&win->vt);

  /* size of text block */
  RCToRect(&r, win->vt.rows, win->vt.cols);
  
  /* bounds of content */
	win->contentrect.x = x;
	win->contentrect.y = y;
	win->contentrect.w = r.x;
	win->contentrect.h = r.y;

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
    win->next = wintopmost;
    wintopmost = win;

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

int WindowMove(win, x, y)
Window *win;
int x, y;
{

  if (win == wintopmost)
  {
    bb.cliprect = win->windowrect;
  }

  if (x != win->windowrect.x || win->windowrect.y != y)
  {
    win->windowrect.x = x;
    win->windowrect.y = y;

    /* update window content too */
    win->contentrect.x = win->windowrect.x + WINBORDER;
    win->contentrect.y = win->windowrect.y + WINTITLEBAR;

    /* frame is dirty */
    win->vt.dirty |= 3;
    
    return 1;
  }
  else
  {
    return 0;
  }
}

int WindowRender(win, forcedirty)
Window *win;
int forcedirty;
{
#ifdef BLINK
  static long newtime,oldtime = 0;
#endif
  struct RECT r;
  struct POINT origin;
  int i,j,k,n,style;
  char line[128];
  char cursor;
  
  if (forcedirty || win->vt.dirty)
  {
    if (forcedirty || (win->vt.dirty & 2))
    {
      // render (just) frame
      r = win->windowrect;
      r.h = WINTITLEBAR;
      bb.halftoneform = &BlackMask;
      RectDrawX(&r, &bb);
      r = win->windowrect;
      r.w = WINBORDER;
      RectDrawX(&r, &bb);
      r.x += win->windowrect.w - WINBORDER;
      RectDrawX(&r, &bb);
      r = win->windowrect;
      r.y += win->windowrect.h - WINBORDER;;
      r.h = WINBORDER;
      RectDrawX(&r, &bb);

      // some accent lines
      bb.halftoneform = &WhiteMask;
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
      r.x += 4;
      r.y += 4;
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
      /* size of single glyph */
      RCToRect(&r, 0, 0);

      origin.y = win->contentrect.y;
      for(j=0; j<win->vt.rows; j++)
      {
        origin.x = win->contentrect.x;
        
        /* will it be clipped away */
        r.x = origin.x;
        r.y = origin.y;
        r.w = win->contentrect.w;
        if (RectIntersects(&r, &bb.cliprect))
        {
          // NB we may have embedded \0 where cursor has been warped as well as handle style attributes
          style = win->vt.attrib[win->vt.cols*j];
          bb.rule = style & 2 ? bbnS : bbS;
          n = 0;
          for(k=0; k<win->vt.cols; k++)
          {
            /* change of style */
            if (win->vt.attrib[win->vt.cols*j+k] != style)
            {
              /* flush so far */
              line[n] = '\0';
              StringDrawX(line, &origin, &bb);

              /* bold needs drawing again */
              if (style & 1)
              {
                origin.x += 1;
                bb.rule = bbSorD;
                StringDrawX(line, &origin, &bb);
                origin.x -= 1;
              }

              origin.x += n * 8;
              n = 0;

              style = win->vt.attrib[win->vt.cols*j+k];
              bb.rule = style & 2 ? bbnS : bbS;
            }
            
            line[n] = win->vt.buffer[win->vt.cols*j+k];
            if (line[n] == 0)
              line[n] = ' ';

            n++;
          }

          line[n] = '\0';
          StringDrawX(line, &origin, &bb);
        }
        
        origin.y += r.h;
      }
      
      bb.rule = bbS;
    }
    
#ifndef BLINK
    if ((win == wintopmost) && (win->vt.hidecursor == 0) && (win->vt.cx < win->vt.cols))
    {
        RCToRect(&r, win->vt.cy, win->vt.cx);
        origin.x = win->contentrect.x + r.x;
        origin.y = win->contentrect.y + r.y;
        cursor = win->vt.buffer[win->vt.cols*win->vt.cy+win->vt.cx];
        CharDrawX(' '+95, &origin, &bb);
    }
#endif
  }

  /* focus window has blinking cursor regardless of whether its dirty */
#ifdef BLINK
  if ((win == winchain) && (win->vt.hidecursor == 0) && (win->vt.cx < win->vt.cols))
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

      CharDrawX(newtime & 1 ? ' '+95 : cursor, &origin, &bb);

      oldtime = newtime;
    }
  }
#endif
  
  return 0;
}

static int counter;

void Paint(win, r, forcedirty)
Window *win;
struct RECT *r;
int forcedirty;
{
  struct RECT overlap;
  struct QUADRECT quadrect;
  int i;
  
  if (win == wintopmost)
    counter++;
  
  /* paint desktop */
  if (win == NULL)
  {
    bb.cliprect.x = 0;
    bb.cliprect.y = 0;
    bb.cliprect.w = ScrWidth;
    bb.cliprect.h = ScrHeight;
    bb.halftoneform =  &GrayMask;

    if ((counter & 1) == 0)
    {
      i = rand() & 255;
  //    RectDebug(r, i,i,i);
    }
    
    RectDrawX(r, &bb);
  }
  else
  {
    /* repaint self */
    RectIntersect(r, &win->windowrect, &overlap);
    if (overlap.w > 0 && overlap.h > 0)
    {
      bb.cliprect = overlap;
      WindowRender(win, forcedirty);
      if ((counter & 1) == 0)
      {
      //  RectDebug(&overlap, rand() & 255,rand() & 255,rand() & 255);
      }
    }

    /* recurse for parts outside this window */
    RectAreasOutside(r, &win->windowrect, &quadrect);
    for(i=0; i<quadrect.next; i++)
    {
      Paint(win->next, &quadrect.region[i], forcedirty);
    }
  }
}

int WindowMin(Window *win)
{
  if (win->windowrect.h == WINTITLEBAR)
  {
    win->windowrect.w = win->oldrect.w;
    win->windowrect.h = win->oldrect.h;
    win->contentrect.h = win->windowrect.h - WINTITLEBAR - WINBORDER;
  }
  else
  {
    win->oldrect = win->windowrect;
    win->windowrect.w = 256;
    win->windowrect.h = WINTITLEBAR;
    win->contentrect.h = 0;
  }
  
  Paint(wintopmost, &win->oldrect, TRUE);

  return 0;
}

void WindowDestroy(wid)
int wid;
{
  Window *awin,*prevwin;
  struct RECT oldrect;
  int i;

  /* unlink it */
  prevwin = NULL;
  awin = wintopmost;
  for(i=0; i<numwindows; i++)
  {
    if (awin == &allwindows[wid])
    {
      if (prevwin)
        prevwin->next = allwindows[wid].next;
      else
        wintopmost = allwindows[wid].next;
      
      // continue walking so we determine last/bottom window
    }
    
    prevwin = awin;
    awin = awin->next;
  }

  oldrect = allwindows[wid].windowrect;
  
  /* reclaim a window slot */

  /* update refs TO last element (which is about to be moved) */
  if (prevwin->next == &allwindows[numwindows-1])
    prevwin->next = &allwindows[wid];

  /* update refs FROM last element (which is about to be moved) */
  if (allwindows[numwindows-1].next == &allwindows[wid])
    allwindows[numwindows-1].next = allwindows[wid].next;

  /* update top window */
  if (wintopmost == &allwindows[numwindows-1])
    wintopmost = &allwindows[wid];

  /* move last element to fill gap of deleted window */
  memcpy(&allwindows[wid], &allwindows[numwindows-1], sizeof(Window));
  numwindows--;

  if (numwindows > 0)
    WindowTop(&allwindows[0]);

  Paint(wintopmost, &oldrect, TRUE);
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
  signal(SIGHUP, cleanup);
  signal(SIGDEAD, cleanup);
  signal(SIGABORT, cleanup);

  InitGraphics(TRUE);

  /* for controllering clipping etc */
  BbcomDefault(&bb);

  /* clear desktop */
  bb.halftoneform = &GrayMask;
  RectDrawX(&bb.cliprect, &bb);

  EventEnable();
  SetKBCode(0);

  // window chain
  wintopmost = NULL;
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
        if (wintopmost)
          n = (int)write(wintopmost->pfd[1], inputbuffer, n);
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
        if (wintopmost)
          n = (int)write(wintopmost->pfd[1], &ch, 1);
      }

      if (GetButtons() & M_LEFT)
      {
        /* walk from frontmost windows back to find which we are over */
        GetMPosition(&origin);
        win = wintopmost;
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
              if (WindowMove(win, p.x - offset.x, p.y - offset.y))
              {
                /* repaint only difference of old and new */
                RectAreasOutside(&r2, &win->windowrect, &quadrect);
                for(j=0; j<quadrect.next; j++)
                  Paint(wintopmost, &quadrect.region[j], TRUE);

                bb.cliprect = win->windowrect;
                WindowRender(win, FALSE);
                
                SDLrefreshwin();
              }
            }
            
            /* was it a Click on top left? */
            if (offset.x < 20 && offset.y < 20)
            {
              WindowMin(win);
            }
            
            /* content is dirty */
            win->vt.dirty |= 3;
            
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

      /* repaint any dirty windows */
      win = wintopmost;
		  while(win)
		  {
        if (win->vt.dirty)
        {
          Paint(wintopmost, &win->windowrect, FALSE);
          win->vt.dirty = 0;
        }

        win = win->next;
		  }
    
      /* this is need in SDL emulator to flip window buffer */
      SDLrefreshwin();

	  }
  }
  
  ExitGraphics();
}
