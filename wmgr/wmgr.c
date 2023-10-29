#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/pty.h>
#include <sys/sgtty.h>
#include <sys/fcntl.h>
#include <net/ftype.h>
#include <signal.h>

#include <font.h>
#include <graphics.h>

#ifdef __clang__

#include "uniflexshim.h"

#else

#include <net/in.h>
#include <net/socket.h>
#include "../fdset.h"

void setsid() {}

#endif

#include "vtemu.h"

extern int rand();
extern int open();

#ifdef __clang__
#define NOTPOLLING
#else
#define POLLING
#endif

/**********************************/
#define WINTITLEBAR 24
#define WINBORDER 8
#define CLOSEBOX

struct DISPSTATE ds;
struct FontHeader *font;
struct BBCOM bb;
struct FORM *screen;

typedef struct _win
{
  /* term emulator; NB always first so we can cast it to a Window */
  VTemu vt;

  struct _win *next;
  char title[32];
  struct RECT oldrect;
  struct RECT windowrect;
  struct RECT contentrect;
  int master,slave;
  int pid;
  int dirty;
 
} Window;

Window *wintopmost;
Window allwindows[32];
int numwindows = 0;
#ifdef __clang__
char *winprocess[] = {"sh", NULL};
#else
char *winprocess[] = {"shell", NULL};
#endif
char *logprocess[] = {"tail", "-f", "/var/log/system.log", NULL};

char welcome[] = "Welcome to Tektronix 4404\r\n";

void RectInset(r, d)
struct RECT *r;
int d;
{
  r->x += d;
  r->y += d;
  r->w -= d * 2;
  r->h -= d * 2;
}

/* graphiclib is wrong */
int myRectContainsPoint(rect, point)
struct RECT *rect;
struct POINT *point;
{
   if (point->x > rect->x && point->x < rect->x + rect->w &&
       point->y > rect->y && point->y < rect->y + rect->h)
     return 0;
  else
   return 1;
}


int window_session(ptfd, islogger)
int *ptfd;
int islogger;
{
  int fdmaster,fdslave;
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

  fdslave = ptfd[0];
  fdmaster = ptfd[1];

  pid = fork();
  if (pid)
  {
  
    /* PARENT */

    n = control_pty(fdmaster, PTY_INQUIRY, 0);
    control_pty(fdmaster, PTY_SET_MODE, n | PTY_REMOTE_MODE);

    /* Close the slave side of the PTY */
    //close(fdslave);
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
    new_term_settings.sg_flag |= RAW;
    new_term_settings.sg_flag |= CRMOD;
    new_term_settings.sg_flag |= XTABS;
  
    new_term_settings.sg_flag |= CBREAK;
    new_term_settings.sg_flag &= ~ECHO;
    stty(fdslave, &new_term_settings);

    /* The slave side of the PTY becomes the standard input and outputs of the child process */
    close(0); /* Close standard input (current terminal) */
    close(1); /* Close standard output (current terminal) */
    close(2); /* Close standard error (current terminal) */

    dup2(fdslave, 0); /* PTY becomes standard input (0) */
    dup2(fdslave, 1); /* PTY becomes standard output (1) */
    dup2(fdslave, 2); /* PTY becomes standard error (2) */

    /* As the child is a session leader, set the controlling terminal to be the slave side of the PTY */
    /* (Mandatory for programs like the shell to make them manage correctly their outputs) */
    /* FIXME ioctl(0, TIOCSCTTY, 1); */

    /* Now the original file descriptor is useless */
    close(fdslave);

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

void movedisplaylines(vt, dst, src, n)
VTemu *vt;
int dst;
int src;
int n;
{
  struct POINT cur;
  
  Window *win = (Window *)vt;

  // blit block of lines somewhere
  bb.srcform = bb.destform = screen;
  bb.cliprect = win->windowrect;
  bb.destrect.x = win->contentrect.x;
  bb.destrect.y = win->contentrect.y + dst  * font->maps->line;
  bb.destrect.w = win->contentrect.w;
  bb.destrect.h = n * font->maps->line;
  bb.halftoneform = NULL;
  bb.rule = bbS;

  bb.srcpoint.x = win->contentrect.x;
  bb.srcpoint.y = win->contentrect.y + src  * font->maps->line;
  
  BitBlt(&bb);
}

int WindowTop(win)
Window *win;
{
  Window *awin,*prevwin;

  if (win != wintopmost)
  {
    if (wintopmost)
      VTblur(wintopmost->vt, wintopmost->master);
  
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
    
    /* fprintf(stderr, "topwindow: %s\n", win->title); */

    VTfocus(win->vt, win->master);

    win->dirty |= 2;
    win->vt.dirty |= 0xffffffff;
  }
  
  bb.cliprect = win->windowrect;

  return 0;
}


int WindowOutput(wid, msg, n)
int wid;
char *msg;
int n;
{
  Window *win = allwindows + wid;

  VToutput(&win->vt, msg, n, win->master);

  return 0;
}


int WindowCreate(title, x, y, islogger)
char *title;
int x,y;
int islogger;
{
  int pid,ptfd[2];
  Window *win = allwindows + numwindows;
  struct RECT r,glyph;

  /* term emu */
  win->vt.cols = 80;
  win->vt.rows = 25;
  VTreset(&win->vt);

  /* size of text block */
  RCToRect(&r, win->vt.rows, win->vt.cols);
  r.x = win->vt.cols * font->maps->maxw;
  r.y = win->vt.rows * font->maps->line;
  
  /* on 8 pixel boundary */
  x &= -8;
  
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

  /* create a pty process; first window is always a logger */
  pid = window_session(ptfd, islogger);
  if (pid > 0)
  {
    win->pid = pid;
    win->master = ptfd[1];
    win->slave = ptfd[0];
    
/*
    fprintf(stderr, "WindowCreate with pid%d\n", pid);
    fprintf(stderr, "WindowCreate pty(%d, %d)\n", win->slave, win->master);
 */
   
    sprintf(win->title, "%s %s [pid:%d]", title, ttyname(win->master), pid);

    /* needs rendering */
    win->dirty = 2;
    win->vt.dirty = 0xffffffff;

    /* link it */
    win->next = wintopmost;
    wintopmost = win;

    numwindows++;
    WindowTop(win);

    WindowOutput(numwindows-1, welcome, sizeof(welcome));

    return numwindows;
  }
  else
  {
    /* failed to create window */
    return -1;
  }
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
    /* byte boundary for faster blit? */

    win->windowrect.x = x;
    win->windowrect.y = y;

    /* update window content too */
    win->contentrect.x = win->windowrect.x + WINBORDER;
    win->contentrect.y = win->windowrect.y + WINTITLEBAR;

    /* frame is dirty */
    win->dirty |= 2;
    win->vt.dirty |= 0xffffffff;
    
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
  struct RECT r,glyph;
  struct POINT origin;
  int i,j,k,n,style;
  char line[128];
  char cursor;
  
  /* size of single glyph */
  RCToRect(&glyph, 0, 0);
  glyph.w = font->maps->maxw;
  glyph.h = font->maps->line;
  if (forcedirty || win->dirty || win->vt.dirty)
  {
    if (forcedirty || (win->dirty & 2))
    {
      /* render (just) frame */

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
      r.y += win->windowrect.h - WINBORDER;
      r.h = WINBORDER;
      RectDrawX(&r, &bb);

      /* some accent lines */
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
      bb.halftoneform = &BlackMask;
      j = StringWidth(win->title, font);
      origin.x = win->windowrect.x + win->windowrect.w / 2 - j/2;
      origin.y = win->windowrect.y + (WINTITLEBAR - glyph.h)/2 + glyph.h;
      StringDrawX(win->title, &origin, &bb, font);
    }

    /* during dragging do not update content */
    if (forcedirty && win == wintopmost)
    {
       return 0;
    }

    /* render contents */
    if (forcedirty || (win->vt.dirty))
    {
      bb.halftoneform = NULL;
    
      origin.y = win->contentrect.y + glyph.h;
      for(j=0; j<win->vt.rows; j++)
      {
        origin.x = win->contentrect.x;
        
        /* is it dirty AND not clipped away */
        r.x = origin.x;
        r.y = origin.y - glyph.h;
        r.w = win->contentrect.w;
        r.h = glyph.h;
        if ((win->vt.dirty & (1<<j)) && RectIntersects(&r, &bb.cliprect))
        {
          /* NB we may have embedded \0 where cursor has been warped as well as handle style attributes */
          style = win->vt.attrib[win->vt.cols*j];
          bb.rule = style & vtINVERTED ? bbnS : bbS;
          n = 0;
          for(k=0; k<win->vt.cols; k++)
          {
            /* change of style */
            if (win->vt.attrib[win->vt.cols*j+k] != style)
            {
              /* flush so far */
              line[n] = '\0';
              StringDrawX(line, &origin, &bb, font);

              /* bold needs drawing again */
              if (style & vtBOLD)
              {
                origin.x += 1;
                bb.rule = bbSorD;
                StringDrawX(line, &origin, &bb, font);
                origin.x -= 1;
              }

              origin.x += n * glyph.w;
              n = 0;

              style = win->vt.attrib[win->vt.cols*j+k];
              bb.rule = style & vtINVERTED ? bbnS : bbS;
            }
            
            line[n] = win->vt.buffer[win->vt.cols*j+k];
            if (line[n] == 0)
              line[n] = ' ';

            n++;
          }

          line[n] = '\0';
          StringDrawX(line, &origin, &bb, font);
          
#if 0
          dst = (char *)screen->addr + screen->inc * origin.y + origin.x/8;
          for(k=0; k<n; k++)
          {
             dst[k] = line[k];
          }
#endif
          
        }
        
        origin.y += glyph.h;
      }
      
      bb.rule = bbS;
    }
    
#ifndef BLINK
    if ((win == wintopmost) && (win->vt.hidecursor == 0) && (win->vt.cx < win->vt.cols))
    {
        origin.x = win->contentrect.x + win->vt.cx * glyph.w;
        origin.y = win->contentrect.y + win->vt.cy * glyph.h - 2;
        cursor = win->vt.buffer[win->vt.cols*win->vt.cy+win->vt.cx];
        CharDrawX(' '+95, &origin, &bb, font);
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
      origin.x = win->contentrect.x + win->vt.cx * glyph.w;
      origin.y = win->contentrect.y + win->vt.cy * glyph.h + glyph.h;
      cursor = win->vt.buffer[win->vt.cols*win->vt.cy+win->vt.cx];
      if (!cursor)
        cursor = ' ';

      CharDrawX(newtime & 1 ? ' '+95 : cursor, &origin, &bb, font);

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
    bb.halftoneform =  &GrayMask;

    if ((counter & 1) == 0)
    {
      i = rand() & 255;
  /*    RectDebug(r, i,i,i); */
    }
    
    bb.cliprect = *r;
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
      /*  RectDebug(&overlap, rand() & 255,rand() & 255,rand() & 255); */
      }
    }

    /* recurse for parts outside this window */
    quadrect.next = 0;
    RectAreasOutside(r, &win->windowrect, &quadrect);
    for(i=0; i<quadrect.next; i++)
    {
      Paint(win->next, &quadrect.region[i], forcedirty);
    }
  }
}

int WindowMin(win)
Window *win;
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
      
      /* continue walking so we determine last/bottom window */
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
cleanup(sig)
int sig;
{
  printf( "cleanup on %d\n", sig);
  
  EventDisable();
  ExitGraphics();
  FontClose(font);
  RestoreDisplayState(&ds);
  
  exit(2);
}

void
handleinput(sig)
int sig;
{
  printf("handleinput on %d\n", sig);
  
}

int
main(argc, argv)
int argc;
char **argv;
{
  int n,i,j,k,rc,fdtty, dummysock;
#ifdef POLLING
  struct in_sockaddr serv_addr;
#endif
  fd_set fd_in;
  char ch, inputbuffer[512];
  struct timeval timeout;
  Window *win;
  struct RECT r,r2;
  struct POINT origin,offset, p;
  struct QUADRECT quadrect;
  struct sgttyb new_term_settings;
  union EVENTUNION ev;
  int framenum = 0;

  /* how do we detect child is dead? */
  signal(SIGINT, cleanup);
  signal(SIGTERM, cleanup);
  signal(SIGABORT, cleanup);

#ifdef __clang__XXXX
  /* watchdog */
  signal(SIGALRM, cleanup);
  alarm(120);
#endif

  SaveDisplayState(&ds);

  font = FontOpen(argv[1] ? argv[1] : "/fonts/MagnoliaFixed7.font");
  fprintf(stderr, "name: %s: face: %s\n", font->name, font->face);
  fprintf(stderr, "size: %d res: %d\n", font->ptsize, font->resolution);
  fprintf(stderr, "fixed: %d width:%d height:%d\n", 
      font->maps->fixed,font->maps->maxw, font->maps->line);

  /* keyboard */
  fdtty = open(ttyname(fileno(stdin)), O_RDWR);
  fprintf(stderr, "reading %d from %s\n", fdtty, ttyname(fileno(stdin)));

sleep(2);

  screen = InitGraphics(TRUE);

  /* for controlling clipping etc */
  BbcomDefault(&bb);

  /* clear desktop */
  bb.halftoneform = &GrayMask;
  RectDrawX(&bb.cliprect, &bb);

  CursorVisible(1);
  CursorTrack(1);

#ifdef __clang__
  EventEnable();
  SetKBCode(0);
#else
  SetKBCode(1);
#endif

  /* window chain */
  wintopmost = NULL;
  numwindows = 0;

  /* Uniflex maps \n to \r so force it in */
  welcome[sizeof(welcome)-1] = 0x0a;
 
  /* create a logger of /var/log/system.log OR run a shell */
#if 0 
  WindowCreate("SysLog", 50, 50, TRUE);
#else
  WindowCreate("Console", 64-WINBORDER, 50, FALSE);
#endif

  /* CBREAK input */
  rc = gtty(fdtty, &new_term_settings);
  new_term_settings.sg_flag |= CBREAK | RAW;
  stty(fdtty, &new_term_settings);

#ifdef __clang__
  /* modern OS no longer support stty as above */
  struct termios t = {};
  tcgetattr(fdtty, &t);
  t.c_lflag &= ~ICANON;
  tcsetattr(fdtty, TCSANOW, &t);
#endif

   /* experiment with non-blocking reading of stdin */
/*
   fcntl(fdtty, FNOBLOCK,0);
*/
   signal(SIGINPUT, handleinput);
   signal(SIGEVT, handleinput);

#ifdef POLLING
  /* this is just so we can get a ms timeout! */
  dummysock = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 12345;
  bind(dummysock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
#else
  dummysock = fdtty;
#endif
  
  /* mainloop */
  while (1)
  {
#if 0
     framenum++;
     i = EGetCount();
     if (i > 0)
     {


     }

     printf("%c[Hframe(%d) ev(%d) %d %d \n",0x1b, framenum, ev.evalue, EGetTime(), i);
#endif

    /* check any outputs from master sides and track highest fd */
    FD_ZERO(&fd_in);
    FD_SET(dummysock, &fd_in);
    n = dummysock;
#ifdef NOTPOLLING
    for(i=0; i<numwindows; i++)
    {
      FD_SET(allwindows[i].master, &fd_in);
      if (allwindows[i].master > n)
        n = allwindows[i].master;
    }
#endif
    /* 20Hz updating */
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    rc = select(n + 1, &fd_in, NULL, NULL, &timeout);

  if (rc < 0 && errno != EINTR) /* what is Uniflex equiv? */
  {
    fprintf(stderr, "select(%d) error %d\n", n, errno);
    exit(23);
  }
  else
#ifdef NOTPOLLING
  if (rc > 0)
#else
  if (rc == 0)
#endif
  {
    /* read any stdio input and send to topwindow input */
#ifdef NOTPOLLING
    if (FD_ISSET(fdtty, &fd_in))
#else
    gtty(fdtty, &new_term_settings);
    if (new_term_settings.sg_speed & INCHR)
#endif
    {
      n = (int)read(fdtty, inputbuffer, sizeof(inputbuffer));
     if (n>0 && wintopmost)
       n = (int)write(wintopmost->master, inputbuffer, n);
    }

    /* read any output from window process */
    for(i=0; i<numwindows; i++)
    {
      n = control_pty(allwindows[i].master, PTY_INQUIRY, 0);
#ifdef NOTPOLLING
      if (FD_ISSET(allwindows[i].master, &fd_in))
#else
      if ((n & PTY_OUTPUT_QUEUED))
#endif
      {
        n = (int)read(allwindows[i].master, inputbuffer, sizeof(inputbuffer));

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
    /* manage windows */
  {

#ifdef __clang__
     /*  THIS CALL TOTALLY HANGS THE MACHINE */
      /* tek4404 event system only used for keypress input */
      ev = (union EVENTUNION)EGetNext();
      if (ev.estruct.etype == E_PRESS)
      {
        char ch = ev.estruct.eparam;
        if (wintopmost)
          n = (int)write(wintopmost->master, &ch, 1);
      }
#endif

if (GetButtons() & M_MIDDLE)
{
  break;
}

      if (GetButtons() & M_LEFT)
      {
        /* walk from frontmost windows back to find which we are over */
        GetMPosition(&origin);

        win = wintopmost;
        while (win)
        {
          /* docs wrong; returns 0 if contained */
          if (!myRectContainsPoint(&win->windowrect, &origin))
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
                quadrect.next = 0;
                RectAreasOutside(&r2, &win->windowrect, &quadrect);
                for(j=0; j<quadrect.next; j++)
                  Paint(wintopmost->next, &quadrect.region[j], TRUE);

                bb.cliprect = win->windowrect;
                WindowRender(win, TRUE);
                
#ifdef __clang__
                SDLrefreshwin();
#endif
              }
            }
            
            /* was it a Click on top left? */
            if (offset.x < 20 && offset.y < 20)
            {
              WindowMin(win);
            }
            
            /* content is dirty */
            win->vt.dirty |= 0xffffffff;
            
            /* now has focus */
            break;
          }
          
          win = win->next;
        }
      }
      else
      if (GetButtons() & M_RIGHT)
      {
          /* pop up menu */

          while (GetButtons() & M_RIGHT);

          GetMPosition(&origin);
          WindowCreate("Window", origin.x - 32, origin.y - 32, FALSE);
      }

// show dirty lines
bb.cliprect.x = 0;
bb.cliprect.y = 0;
bb.cliprect.w = 32;
bb.cliprect.h = 32 * font->maps->line;
for (i=0; i<25; i++)
{
      /* render closebox */
      bb.halftoneform = wintopmost->vt.dirty & (1<<i) ? &DarkGrayMask : &WhiteMask;
      r.x = 0;
      r.y = font->maps->line * i;
      r.w = font->maps->line;
      r.h = font->maps->line;
      RectDrawX(&r, &bb);
      bb.halftoneform = &BlackMask;
      RectBoxDrawX(&r, 1, &bb);
}


      /* repaint any dirty windows */
      win = wintopmost;
     while(win)
     {
       if (win->dirty || win->vt.dirty)
       {
          Paint(wintopmost, &win->windowrect, FALSE);
          win->dirty = 0;
          win->vt.dirty = 0;
       }

        win = win->next;
     }
    
      /* this is need in SDL emulator to flip window buffer */
#ifdef __clang__
      SDLrefreshwin();
#endif

    }
  }
  SetKBCode(1);
  EventDisable();
  ExitGraphics();
  FontClose(font);
  RestoreDisplayState(&ds);
  fprintf(stderr, "Done\n");
}
