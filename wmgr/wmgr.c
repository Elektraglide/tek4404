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

#include <stdarg.h>
#include "uniflexshim.h"

#define	AF_INTRA	AF_UNIX
extern int kill();
extern int wait();
extern char *getenv();


#else

#include <varargs.h>
#include <net/in.h>
#include <net/socket.h>
#include "../fdset.h"

void setsid() {}

#endif

#include "vtemu.h"
#include "events.h"
#include "win.h"

extern int rand();
extern int open();

extern int system_control(); /* arg=1 does something, arg=2 makes controlling termnal */

#ifdef __clang__
#define POLLINGPTYx
#define USE_EVENTS		
#define USE_TTYREADERx
#define POLLINGINPx
#else
#define POLLINGPTY			/* cannot select() pty; only way to check for pty input is to poll using control_pty() */
#define USE_EVENTS			/* use Event system to get input */
#define USE_TTYREADERx		/* cannot select() stdin; use a socketpair + child process */
#define POLLINGINPx			/* PLAN Z;  Poll stdin by checking (sg_speed & INCHR) */
#endif

#ifdef USE_TTYREADER
int sockpair[2] = {0,0};
int readerpid = 0;
char *droneprocess[] = {"drone", NULL};

void ttycleanup(sig)
int sig;
{
  int i,pid;
	
  if (readerpid)
  {
    kill(readerpid,SIGTERM);
    sleep(1);

    close(sockpair[1]);
    readerpid = 0;

#if 0  /* why does this hang indefinitely? */
    fprintf(stderr, "waiting to reap ttyreader\012\n");	
    pid = wait(&i);  
    fprintf(stderr, "pid=%d\012\n", pid);
#endif
  }
}

char *getttysock(sockinput)
int *sockinput;
{
  struct sgttyb new_term_settings;
  int rc,n,fd;
  char inputbuffer[128];
	
  if (sockpair[1] > 0)
  {
    *sockinput = sockpair[1];
    return  nget_str("cur_tty");
  }

  /* setup an input reader process */
  rc = socketpair(AF_INTRA,SOCK_DGRAM,0, sockpair);
  if (rc < 0)
  {
    fprintf(stderr, "Error %s on socketpair\n", nserror());
    exit(errno);
  }
  
  if ((readerpid = fork()))
  {
    /* close(sockpair[0]); */
    *sockinput = sockpair[1];
    return nget_str("cur_tty");
  }
  else
  {
	  signal(SIGDEAD, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_IGN);
  
    fd = fileno(stdin);
		
    /* CBREAK input */
    rc = gtty(fd, &new_term_settings);
    new_term_settings.sg_flag |= CBREAK | RAW;
    stty(fd, &new_term_settings);

#ifdef __clang__
  /* modern OS no longer support stty as above */
  struct termios t = {};
  tcgetattr(fd, &t);
  t.c_lflag &= ~ICANON;
  tcsetattr(fd, TCSANOW, &t);

    /* read stdin & write forever */
    while ((n = (int)read(fd, inputbuffer, sizeof(inputbuffer))) > 0)
    {
      n = write(sockpair[0], inputbuffer, n);
    }
#endif

    /* NB drone is the tek4404 pattern for helper process to read stdin */
    dup2(fd, 0);
    dup2(sockpair[0], 1);
    close(2);
    execvp(droneprocess[0], droneprocess);

    fprintf(stderr, "ttyreader exit\012\n");
    exit(1);
  }
}

#endif

int fdtty;

/**********************************/

#define DEBUGREPAINTxx

#define FORCEPAINT 	0x0001
#define DRAGGED 		0x8000

#define DIRTYFRAME 2

struct DISPSTATE ds;
struct FontHeader *font;
struct BBCOM bb;
struct FORM *screen;
struct RECT screenrect;

char *items[6] = {"shell","refresh", "quit to console"};
int flags[6] = {0,MENU_LINE,0};
struct MENU *menu;

int usecustomblit = 0;
long newtime,oldtime = 0;
char cursor;


Window *wintopmost;
Window allwindows[32];
int numwindows = 0;
#ifdef __clang__
char *logprocess[] = {"tail","-f", "/var/log/system.log", NULL};
char *winprocess[] = {"sh", NULL};
#else
/* native tail is kinda noddy, this one supports follow (-f) */
char *logprocess[] = {"/bin/UNIXtools/tail","-f", "/etc/log/system.log", NULL};
char *winprocess[] = {"ash", NULL};
#endif

char welcome[] = "Welcome to Tektronix 4404\012\n";

void SetClip(r)
struct RECT *r;
{
    /* always bound by screenrect */
    RectIntersect(r, &screenrect, &bb.cliprect);
}

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
     return 1;
  else
   return 0;
}

int myRectIntersects(r1,r2)
struct RECT *r1;
struct RECT *r2;
{
  if (r1->x < r2->x+r2->w &&
      r2->x < r1->x+r1->w &&
      r1->y < r2->y+r2->h &&
      r2->y < r1->y+r1->h)
    return 1;
    
  return 0;	
}

#ifdef __clang__
int WindowLog(char *fmt, ...);
#endif
void cleanup_child(sig)
int sig;
{

  WindowLog("signal(%d): child of parent %d\n", sig, getpid());
  exit(sig);
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
    n = control_pty(fdmaster, PTY_INQUIRY, 0);
    control_pty(fdmaster, PTY_SET_MODE, n | PTY_READ_WAIT);

    /* Close the slave side of the PTY */
    /* Needed to get ttyname() close(fdslave);  */
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
    new_term_settings.sg_flag |= CBREAK;
    new_term_settings.sg_flag &= ~ECHO;
#if 1
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

    /* use preferred shell */
    if (getenv("SHELL"))
      winprocess[0] = (char *)getenv("SHELL");
    
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

void addcursor(win)
Window *win;
{
  struct RECT r;
  
  if ((win->vt.hidecursor == 0) && (win->vt.cx < win->vt.cols))
  {
    newtime = EGetTime() / 204;
    if (oldtime != newtime)
    {
      r.x = win->contentrect.x + win->vt.cx * font->maps->maxw;
      r.y = win->contentrect.y + win->vt.cy * font->maps->line + 2;
      cursor = win->vt.buffer[RC2OFF(win->vt.cy,win->vt.cx)];

      if (!cursor)
        cursor = ' ';

      SetClip(&win->contentrect);
      
      bb.rule = bbSxorD;

      r.w = font->maps->maxw;
      r.h = font->maps->baseline - 2;
      bb.halftoneform = &BlackMask;
      RectDrawX(&r, &bb);
      bb.rule = bbS;

      oldtime = newtime;
    }
  }
}

void removecursor(win)
Window *win;
{
  struct POINT origin;

  if ((win->vt.cx < win->vt.cols))
  {
    origin.x = win->contentrect.x + win->vt.cx * font->maps->maxw;
    origin.y = win->contentrect.y + win->vt.cy * font->maps->line + font->maps->baseline;
    cursor = win->vt.buffer[RC2OFF(win->vt.cy,win->vt.cx)];
    if (!cursor)
      cursor = ' ';

    SetClip(&win->contentrect);
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = screen->w;
    bb.destrect.h = screen->h;
    CharDrawX(cursor, &origin, &bb, font);
  }
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
        /* unlink from current position in list */
        if (prevwin)
          prevwin->next = win->next;

        /* append to list */
        win->next = wintopmost;
        wintopmost = win;
        break;
      }
      
      prevwin = awin;
      awin = awin->next;
    }
    
    /* fprintf(stderr, "topwindow: %s\n", win->title); */

    VTfocus(win->vt, win->master);

    win->dirty |= DIRTYFRAME;
    win->vt.dirtylines |= ALLDIRTY;
  }
  
  SetClip(&win->windowrect);

  return 0;
}


int WindowOutput(wid, msg, n)
int wid;
char *msg;
int n;
{
  Window *win = allwindows + wid;

  if (win == wintopmost)
   removecursor(win);

  VToutput(&win->vt, msg, n, win->master);

  return 0;
}

#ifdef __clang__
int WindowLog(char *fmt, ...)
{
va_list p;
char buffer[256];
unsigned int val1;
unsigned int val2;
unsigned int val3;
unsigned int val4;

	va_start(p, fmt);

#else
int WindowLog(_varargs)
int _varargs;
{
va_list p;
char buffer[256];
char *fmt;
unsigned int val1;
unsigned int val2;
unsigned int val3;
unsigned int val4;

	va_start(p);
  fmt = va_arg(p, char *);
#endif
    val1 = va_arg(p, unsigned int);
    val2 = va_arg(p, unsigned int);
    val3 = va_arg(p, unsigned int);
    val4 = va_arg(p, unsigned int);
	va_end(p);
    
	sprintf(buffer, fmt, val1, val2, val3, val4);
	if(wintopmost)
 		WindowOutput(0, buffer, strlen(buffer));
 		
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
  win->vt.rows = 32;
  VTreset(&win->vt);

  /* size of text block */
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
    
    sprintf(win->title, "%s %s [pid:%d]", title, ttyname(win->slave), pid);

    /* needs rendering */
    win->dirty = DIRTYFRAME;
    win->vt.dirtylines = ALLDIRTY;

    /* link it */
    win->next = wintopmost;
    wintopmost = win;

    if (!islogger)
	  	WindowOutput(numwindows, welcome, sizeof(welcome));

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

int WindowMove(win, x, y)
Window *win;
int x, y;
{

  if (win == wintopmost)
  {
    SetClip(&win->windowrect);
  }

  if ((x & -8) != win->windowrect.x || win->windowrect.y != y)
  {
    /* byte boundary for faster blit? */
    win->contentrect.x = (x + WINBORDER) & -8;
    win->contentrect.y = y + WINTITLEBAR;

    win->windowrect.x = win->contentrect.x - WINBORDER;
    win->windowrect.y = win->contentrect.y - WINTITLEBAR;

    /* frame is dirty */
    win->dirty |= DIRTYFRAME;
    win->vt.dirtylines |= ALLDIRTY;
    
    return 1;
  }
  else
  {
    return 0;
  }
}

void renderstring(line, n, ox, oy)
register char *line;
int n,ox,oy;
{
  register short j,k,rows,ch;
  register unsigned char *src,*dst;
  
  /* assumes font is fixed 8px and font bitmap stride=1024 and framebuffer stride=1024 */
  src = (unsigned char *)font->bitmap - 1;
  dst = (unsigned char *)screen->addr + ((oy-font->maps->baseline)<<7) + (ox>>3);
  for(k=0; k<n; k++)
  {
    ch = line[k];

    j = 0;
    rows = 12;
    while(rows-- > 0)
    {
      dst[k+j] = src[ch+j];
      j += 128;
    }
  }
}

void renderstring2(line, n, ox, oy)
register char *line;
int n,ox,oy;
{
  int j,k,rows,ch;
  unsigned char *src,*dst,*p;
  
  /* assumes font is fixed 8px and font bitmap stride=1024 and framebuffer stride=1024 */
  src = (unsigned char *)font->bitmap - 1;
  dst = (unsigned char *)screen->addr + ((oy-font->maps->baseline)<<7) + (ox>>3);
	rows = 12;
	while(rows-- > 0)
	{
		for(k=0; k<n; k++)
		{
			ch = line[k];
			dst[k] = src[ch];
		}
		src += 128;
		dst += 128;
	}
}


/* adjust line[] to be inside clipprect */
static int cropline(aline, len, origin, glyph)
char *aline;
int len;
struct POINT *origin;
struct RECT *glyph;
{
  int excess;

	/* crop top and bottom */
	if (origin->y + glyph->h < bb.cliprect.y || origin->y > bb.cliprect.y + bb.cliprect.h)
	{
		len = 0;
	}
	else
	{
    /* crop left */
    excess = bb.cliprect.x - origin->x;
    if (excess > 0)
    {
      memcpy(aline, aline + excess / glyph->w, len);
      origin->x += excess;
      len -= excess / glyph->w;
    }

    /* crop right */
    excess = (origin->x + len * glyph->w) - (bb.cliprect.x + bb.cliprect.w);
    if (excess > 0)
    {
      len -= excess / glyph->w;
    }
	}
	
	return len;
}

int WindowRender(win, forcedirty)
Window *win;
int forcedirty;
{
  struct RECT r,r2,glyph;
  struct POINT origin;
  struct POINT strpos;
  register int j,k;
  int i,n,style,numlines,ch,rows;
  char line[160];
  unsigned char *src,*dst;

  /* size of single glyph */
  RCToRect(&glyph, 0, 0);
  glyph.w = font->maps->maxw;
  glyph.h = font->maps->line;
  if (forcedirty || win->dirty || win->vt.dirtylines)
  {
    bb.rule = bbS;
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = screen->w;
    bb.destrect.h = screen->h;
    
    if (forcedirty || (win->dirty & DIRTYFRAME))
    {
      /* render (just) frame */
      r.x = win->windowrect.x;
      r.y = win->windowrect.y + WINTITLEBAR;
      r.w = win->windowrect.w;
      r.h = win->windowrect.h - WINTITLEBAR;
      bb.halftoneform = &WhiteMask;
      RectDrawX(&r, &bb);

      /* margins */
      bb.halftoneform = &BlackMask;

      /* left */
      r.x = win->windowrect.x + 1;
      r.y = win->windowrect.y;
      r.w = WINBORDER - 2;
      r.h = win->windowrect.h;
      RectBoxDrawX(&r, 1, &bb);

      /* right */
      r.x += win->windowrect.w - 1 - (WINBORDER-1);
      RectBoxDrawX(&r, 1, &bb);

      /* bottom */
      r.x = win->windowrect.x;
      r.y = win->windowrect.y + win->windowrect.h - 1 - (WINBORDER-1) + 1;
      r.w = win->windowrect.w;
      r.h = WINBORDER - 2;
      RectBoxDrawX(&r, 1, &bb);

      /* title bar */
      r.y = win->windowrect.y;
      r.h = WINTITLEBAR;
      bb.halftoneform = &VeryLightGrayMask;
      RectDrawX(&r, &bb);
      r.x += 1;
      r.y += 1;
      r.w -= 2;
      r.h -= 2;
      bb.halftoneform = &BlackMask;
      RectBoxDrawX(&r, 1, &bb);

#ifdef CLOSEBOX
      /* render closebox */
      bb.halftoneform = &WhiteMask;
      r.x = win->windowrect.x + ((WINBORDER-CLOSEBOX)>>1);
      r.y = win->windowrect.y + ((WINTITLEBAR-CLOSEBOX)>>1);
      r.w = CLOSEBOX;
      r.h = CLOSEBOX;
      RectDrawX(&r, &bb);
      bb.halftoneform = &BlackMask;
      RectBoxDrawX(&r, 1, &bb);
#endif

      /* render centered title */
      bb.halftoneform = &BlackMask;
      strcpy(line, win->title);
      if (win->contentrect.h == 0)
      	line[24] = '\0';

      j = StringWidth(line, font);
      origin.x = win->windowrect.x + win->windowrect.w / 2 - j/2;
      origin.y = win->windowrect.y + (WINTITLEBAR - glyph.h)/2 + font->maps->baseline;
      StringDrawX(line, &origin, &bb, font);
    }

    /* during dragging do not update content */
    if (forcedirty & DRAGGED)
    {
       return 0;
    }

		/* make all lines dirty */
    if (forcedirty)
			win->vt.dirtylines = ALLDIRTY;
			
    /* render contents */
    if (win->vt.dirtylines)
    {
      bb.halftoneform = NULL;
    
      ProtectCursor(&win->contentrect, NULL);
      numlines = 0;
      origin.y = win->contentrect.y + font->maps->baseline;
      for(j=0; j<win->vt.rows; j++)
      {
        origin.x = win->contentrect.x;
        
        /* is it dirty AND not clipped away */
        r.x = origin.x;
        r.y = origin.y - font->maps->baseline;
        r.w = win->contentrect.w;
        r.h = glyph.h;
        if ((win->vt.dirtylines & (1<<j)) && myRectIntersects(&r, &bb.cliprect))
        {
          /* NB we may have embedded \0 where cursor has been warped as well as handle style attributes */
          style = win->vt.attrib[win->vt.cols*j];
          bb.rule = style & vtINVERTED ? bbnS : bbS;
          n = 0;
          /* NB <= because linelengths[] is actually last cursorx */
          for(k=0; k<=win->vt.linelengths[j]; k++)
          {
            /* change of style */
            if (win->vt.attrib[RC2OFF(j,k)] != style)
            {
              /* flush so far */
              line[n] = '\0';
              strpos = origin;
              StringDrawX(line, &strpos, &bb, font);

              /* bold needs drawing again */
              if (style & vtBOLD)
              {
                origin.x += 1;
                bb.rule = bbSorD;
                strpos = origin;
                StringDrawX(line, &strpos, &bb, font);
                origin.x -= 1;
              }

              origin.x += n * glyph.w;
              n = 0;

              style = win->vt.attrib[RC2OFF(j,k)];
              bb.rule = style & vtINVERTED ? bbnS : bbS;
            }
            
            line[n] = win->vt.buffer[RC2OFF(j,k)];
            if (line[n] == 0)
              line[n] = ' ';

            n++;
          }

          n = cropline(line, n, &origin, &glyph);
#ifdef DEBUGREPAINT
          r2.x = origin.x;
          r2.y = origin.y - font->maps->baseline;
          r2.w = n * glyph.w;
          r2.h = glyph.h;

          RectDebug(&r2, rand() & 255,rand() & 255,rand() & 255);
#else
          if (n > 0)
          {
            line[n] = '\0';
            if (usecustomblit && !style && n < 64)
            {
              renderstring2(line, n, origin.x, origin.y);
            }
            else
            {
              strpos = origin;
              StringDrawX(line, &strpos, &bb, font);
            }
          }
#endif

          numlines++;
        }
        
        origin.y += glyph.h;
      }
      ReleaseCursor();
            
      bb.rule = bbS;
    }
    
		/* this is now clean */
		if (forcedirty)
		{
      win->dirty = 0;
      win->vt.dirtylines = 0;
		}

  }

  return 0;
}


void movedisplaylines(vt, dst, src, n)
VTemu *vt;
int dst;
int src;
int n;
{
struct POINT cur;
  
  Window *win = (Window *)vt;

  if (win != wintopmost)
    return;

  SetClip(&win->contentrect);

  /* flush any dirty lines before we blit */
  WindowRender(win, FALSE); 
  
  bb.srcform = bb.destform = screen;
  bb.destrect.x = win->contentrect.x;
  bb.destrect.y = win->contentrect.y + dst  * font->maps->line;
  bb.destrect.w = win->contentrect.w;
  bb.destrect.h = n * font->maps->line;
  bb.halftoneform = &BlackMask;
  bb.rule = bbS;

  bb.srcpoint.x = win->contentrect.x;
  bb.srcpoint.y = win->contentrect.y + src * font->maps->line;
  
  BitBlt(&bb);
  
  bb.srcform = NULL;
  bb.srcpoint.x = 0;
  bb.srcpoint.y = 0;
  bb.destrect.x = 0;
  bb.destrect.y = 0;
  bb.destrect.w = screen->w;
  bb.destrect.h = screen->h;
}

void cleardisplaylines(vt, dst, n)
VTemu *vt;
int dst;
int n;
{
struct RECT r;
  
  Window *win = (Window *)vt;

  if (win != wintopmost)
    return;

  SetClip(&win->contentrect);
  bb.halftoneform = &WhiteMask;
  bb.rule = bbS;

  r.x = win->contentrect.x;
  r.y = win->contentrect.y + dst * font->maps->line;
  r.w = win->contentrect.w;
  r.h = n * font->maps->line;
  RectDrawX(&r, &bb);
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

#ifdef DEBUGREPAINT
		if ((counter & 1) == 0)
    {
      i = rand() & 255;
     RectDebug(r, i,i,i);
    }
#endif
    
    RectDrawX(r, &bb);
  }
  else
  {
    /* recurse for parts outside this window */
    quadrect.next = 0;
    RectAreasOutside(r, &win->windowrect, &quadrect);
    for(i=0; i<quadrect.next; i++)
    {
      Paint(win->next, &quadrect.region[i], forcedirty);
    }

    /* repaint self */
    RectIntersect(r, &win->windowrect, &overlap);
    if (overlap.w > 0 && overlap.h > 0)
    {
    	struct RECT oldclip;
     
      oldclip.x  = bb.cliprect.x;
      oldclip.y  = bb.cliprect.y;
      oldclip.w  = bb.cliprect.w;
      oldclip.h  = bb.cliprect.h;
      SetClip(&overlap);

#ifdef DEBUGREPAINT
      if ((counter & 1) == 0)
      {
      	 RectDebug(&overlap, rand() & 255,rand() & 255,rand() & 255);
      }
#endif

      WindowRender(win, forcedirty);
      
      bb.cliprect = oldclip;
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
  
  Paint(wintopmost, &win->oldrect, FORCEPAINT);

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
      
      /* continue walking so we determine last/bottom window */
    }
    
    prevwin = awin;
    awin = awin->next;
  }

  oldrect = allwindows[wid].windowrect;
  
  /* reclaim a window slot */
  
  /* is top window being destroyed? */
  if (wintopmost == &allwindows[wid])
    wintopmost = wintopmost->next;

  /* update refs TO last element (which is about to be moved) */
  if (prevwin->next == &allwindows[numwindows-1])
    prevwin->next = &allwindows[wid];

  /* update refs FROM last element (which is about to be moved) */
  if (allwindows[numwindows-1].next == &allwindows[wid])
    allwindows[numwindows-1].next = allwindows[wid].next;

  /* update top window ref to last element (which is about to be moved) */
  if (wintopmost == &allwindows[numwindows-1])
    wintopmost = &allwindows[wid];

  /* move last element to fill gap of deleted window */
  memcpy(&allwindows[wid], &allwindows[numwindows-1], sizeof(Window));
  numwindows--;
  
  if (!wintopmost || numwindows < wid)
    wintopmost = &allwindows[numwindows-1];

  if (numwindows > 0)
    WindowTop(wintopmost);

	SetClip(&oldrect);
  Paint(wintopmost, &oldrect, FORCEPAINT);
}

void
cleanup_and_exit(sig)
int sig;
{
  struct POINT p;
  int i;
	
  fprintf(stderr, "cleanup on %d\n", sig);

#ifdef USE_TTYREADER
  ttycleanup(sig);
#else
  close(fdtty);
#endif

  /* kill all windows */
  for(i=0; i<numwindows; i++)
  {
    kill(allwindows[i].pid, SIGTERM);
  }
  sleep(1);
  
  /* reset viewport */
  p.x = p.y = 0;
  SetViewport(&p);

  EventDisable();
  SetKBCode(1);
  ExitGraphics();
  FontClose(font);
  RestoreDisplayState(&ds);

  fprintf(stderr, "Sic transit gloria Tektronix 4404\n");
  exit(2);
}

void
cleanup_window(sig)
int sig;
{
  int i,pid;

  pid = wait(&i);
  WindowLog("child proc(%d) died with exit code %d\012\n", pid, i);

  for(i=0; i<numwindows; i++)
  {
    if (allwindows[i].pid == pid)
    {
      WindowDestroy(i);
      break;
    }
  }

  signal(SIGDEAD, cleanup_window);
}

void sh_timer(sig)
int sig;
{

  WindowLog("timer signal\012\n");
  signal(sig, sh_timer);
  ESetAlarm(EGetTime() + 1000);
}

void sh_event(sig)
int sig;
{

  WindowLog("event signal\012\n");
  signal(sig, sh_event);
  ESetSignal();
}

void sh_input(sig)
int sig;
{

  WindowLog("input signal\012\n");
  signal(sig, sh_input);
}


int
main(argc, argv)
int argc;
char **argv;
{
  int n,i,j,k,rc,dummysock;
#ifdef POLLINGINP
  struct in_sockaddr serv_addr;
#endif
  fd_set fd_in;
  char ch, *name, inputbuffer[4096];
  struct timeval timeout;
  Window *win;
  struct RECT r,r2;
  struct POINT origin,offset, p;
  struct QUADRECT quadrect;
  struct sgttyb new_term_settings;
  union EVENTUNION ev;
  int framenum = 0;
  int last_read;
  
  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);
  
  signal(SIGDEAD, cleanup_window);

#if 0
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
  fprintf(stderr, "bitmap: %8.8x (%d x %d)\n", 
      font->bitmap, font->width, font->height);

  /* can we use a custom text render? */
  usecustomblit = (font->maps->maxw == 8 && font->maps->line == 12 && font->bitmap);
  fprintf(stderr,"usecustom=%d\n",usecustomblit);

  menu = MenuCreateX(3,items,flags,0,font);

  screen = InitGraphics(TRUE);
  screenrect.x = 0;
  screenrect.y = 0;
  screenrect.w = screen->w;
  screenrect.h = screen->h;
  
  /* for controlling clipping etc */
  BbcomDefault(&bb);

  /* clear desktop */
  bb.halftoneform = &GrayMask;
  RectDrawX(&bb.cliprect, &bb);

  CursorVisible(1);
  CursorTrack(1);

#ifdef USE_EVENTS
  EventEnable();
  SetKBCode(0);

/*  ESetSignal();
  signal(SIGEVT, sh_event); */
#endif

  /* does SIGINPUT get delivered? */
/*  signal(SIGINPUT, sh_input);  */

  /* window chain */
  wintopmost = NULL;
  numwindows = 0;

  /* Uniflex maps \n to \r so force it in */
  welcome[sizeof(welcome)-1] = 0x0a;
 
  /* create a logger of /var/log/system.log OR run a shell */
#if 0 
  WindowCreate("SysLog", 50, 50, TRUE);
#else
  WindowCreate("Console", 64-WINBORDER, 50, TRUE);
#endif

#ifdef USE_TTYREADER

  name = getttysock(&fdtty);
  /* fprintf(stderr, "reading %s on socket(%d)\012\n", name, fdtty); */

#endif


#ifdef POLLINGINP
  /* keyboard */
  fdtty = open(ttyname(fileno(stdin)), O_RDWR);
  /* fprintf(stderr, "reading %s using fd(%d)\012\n", ttyname(fileno(stdin)), fdtty); */
  /* CBREAK input */
  rc = gtty(fdtty, &new_term_settings);
  new_term_settings.sg_flag |= CBREAK | RAW;
  stty(fdtty, &new_term_settings);

#ifdef __clang__
  /* modern OS no longer support stty as above */
  struct termios t = {};
  tcgetattr(fdtty, &t);
  t.c_lflag &= ~(ICANON | ISIG | IEXTEN);
  t.c_lflag |= (ECHO);
  
  tcsetattr(fdtty, TCSANOW, &t);
#endif
#endif

dummysock = fdtty;

#ifdef POLLINGINP
  /* this is just so we can get a ms timeout! */
  dummysock = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 12345;
  bind(dummysock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
#endif

	/* we want this to be snappy */
	nice(-32);

  WindowLog("Window Manager started as pid%d\012\n", getpid());

  /* mainloop */
  last_read = 0;
  while (1)
  {
    /* check any outputs from master sides and track highest fd */
    FD_ZERO(&fd_in);
    FD_SET(dummysock, &fd_in);
    n = dummysock;

#ifndef POLLINGPTY
    for(i=0; i<numwindows; i++)
    {
      FD_SET(allwindows[i].master, &fd_in);
      if (allwindows[i].master > n)
        n = allwindows[i].master;
    }
#endif

    /* 20Hz updating */
    timeout.tv_sec = 0;
    timeout.tv_usec = 25000;
    if (last_read > 0)
    {
      timeout.tv_usec = 50000;
      last_read--;
    }
    rc = select(n + 1, &fd_in, NULL, NULL, &timeout);

  if (rc < 0)
  {
    if (errno != EINTR)
    {
      fprintf(stderr, "select(%d) error %s\n", n, strerror(errno));
      exit(23);
    }
    else
    {
      /* allow reading events */
    }
  }
  else
#ifdef POLLINGPTY
	rc = 1;
#endif
#ifdef POLLINGINP
	rc = 1;
#endif
#ifdef USE_EVENTS
	rc = 1;
#endif

  if (rc > 0)
  {

#ifdef USE_EVENTS

      /* tek4404 event system only used for keypress input */
      n = readkeyboardevents(inputbuffer, 32);

      /* window#0 is immortal */
      if (n > 0 && wintopmost > allwindows)
      {
          last_read = 5;

          n = (int)write(wintopmost->master, inputbuffer, n);

          /* Uniflex pty does not handle Ctrl-C! */
          if (inputbuffer[0] == 0x03)
          {
              control_pty(wintopmost->master, PTY_FLUSH_WRITE, 0);
              kill(wintopmost->pid, SIGINT);

              WindowLog("Sent SIGINT to window%d\012\n", wintopmost - allwindows);
          }

          if (inputbuffer[0] == 0x04)
          {
              control_pty(wintopmost->master, PTY_FLUSH_WRITE, 0);
              kill(wintopmost->pid, SIGHUP);

              WindowLog("Sent SIGHUP to window%d\012\n", wintopmost - allwindows);
          }

          if (inputbuffer[0] == 0x1c)
          {
              control_pty(wintopmost->master, PTY_FLUSH_WRITE, 0);
              kill(wintopmost->pid, SIGQUIT);

              WindowLog("Sent SIGQUIT to window%d\012\n", wintopmost - allwindows);
          }

/* we dont know whether we are in RAW mode.. */
#if 0
          /* flow control DC1 / DC3 */
          if (inputbuffer[0] == 0x11)
          {
              control_pty(wintopmost->master, PTY_START_OUTPUT, 0);
              WindowLog( "XON\012\n");
          }
          if (inputbuffer[0] == 0x13)
          {
              control_pty(wintopmost->master, PTY_STOP_OUTPUT, 0);
              WindowLog( "XOFF\012\n");
          }
#endif

      }

#else
    /* read any stdio input and send to topwindow input */
#ifdef POLLINGINP
    gtty(fdtty, &new_term_settings);
    last_read = (new_term_settings.sg_speed & INCHR) ? 5 : last_read;
    if (new_term_settings.sg_speed & INCHR)
#else
    if (FD_ISSET(fdtty, &fd_in))
#endif
    {
      n = (int)read(fdtty, inputbuffer, sizeof(inputbuffer));
      if (n>0 && wintopmost > allwindows)
      {
        n = (int)write(wintopmost->master, inputbuffer, n);
        
        /* Uniflex pty does not handle Ctrl-C! */
        if (inputbuffer[0] == 0x03)
        {
          control_pty(wintopmost->master, PTY_FLUSH_WRITE, 0);
          WindowOutput(wintopmost - allwindows, "SIGINT\012\n", 8);
          kill(wintopmost->pid, SIGINT);
      }
      
        if (n < 0)
        {
       	  WindowDestroy((int)(wintopmost - allwindows));
        }
      }
    }
#endif


    /* read any output from window process */
    for(i=0; i<numwindows; i++)
    {
#ifdef POLLINGPTY
      n = control_pty(allwindows[i].master, PTY_INQUIRY, 0);
      last_read = (n & PTY_OUTPUT_QUEUED) ? 5 : last_read;
      while (n & PTY_OUTPUT_QUEUED)
#else
      if (FD_ISSET(allwindows[i].master, &fd_in))
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
          rc = 0;
          wait(&rc);
          WindowDestroy(i);
          WindowLog("window%d destroyed with exit code 0x%04x\012\n", i, rc);
          break;
        }
        
#ifdef POLLINGPTY
          /* check again if any output */
          n = control_pty(allwindows[i].master, PTY_INQUIRY, 0);
#endif

      }
    }
  }

  /* manage windows */
  {

    if (GetButtons() & M_MIDDLE)
    {
        int choice = MenuSelect(menu);
        if (choice == 0)
        {
            GetMPosition(&origin);
            WindowCreate("Window", origin.x - 32, origin.y - 32, FALSE);
        }
        else
        if (choice == 1)
        {
            /* repaint everything */
            SetClip(&screenrect);
            Paint(wintopmost, &screenrect, FORCEPAINT);
        }
        else
        if (choice == 2)
        {
            cleanup_and_exit(0);
            exit(0);
        }
    }

      if (GetButtons() & M_LEFT)
      {
        /* walk from frontmost windows back to find which we are over */
        GetMPosition(&origin);

        win = wintopmost;
        while (win)
        {
          /* docs wrong; returns 0 if contained */
          if (myRectContainsPoint(&win->windowrect, &origin))
          {
            WindowTop(win);

            /* record offset from window boundary */
            offset.x = origin.x - win->windowrect.x;
            offset.y = origin.y - win->windowrect.y;


            /* track drag of window frame (ie not in contentrect) */
            if (!myRectContainsPoint(&win->contentrect, &origin))
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
                  Paint(wintopmost->next, &quadrect.region[j], FORCEPAINT | DRAGGED);

                SetClip(&win->windowrect);
                WindowRender(win, FORCEPAINT | DRAGGED);

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

            /* repaint everything */
            Paint(wintopmost, &screenrect, FORCEPAINT);

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

    /* debugging dirtylines */
#if 0
    if (wintopmost)
    {
      /* show dirty lines */

      r = wintopmost->contentrect;
      r.x -= WINBORDER + font->maps->line + 64;
      SetClip(&r);

      for (i=0; i<25; i++)
      {
        /* render status box */
        bb.halftoneform = wintopmost->vt.dirtylines & (1<<i) ? &DarkGrayMask : &WhiteMask;
        r.x = wintopmost->contentrect.x - WINBORDER - font->maps->line ;
        r.y = bb.cliprect.y + font->maps->line * i;
        r.w = font->maps->line;
        r.h = font->maps->line;
        RectDrawX(&r, &bb);
        bb.halftoneform = &BlackMask;
        RectBoxDrawX(&r, 1, &bb);
        
        sprintf(inputbuffer, "%3d", wintopmost->vt.linelengths[i]);
        r.x -= font->maps->maxw * 3;
        r.y += font->maps->line;
        StringDrawX(inputbuffer, (struct POINT *)&r, &bb, font);
        
      }
    }
#endif

  /* repaint any dirty windows */
  win = wintopmost;
  while(win)
  {
    if (win->dirty )
    {
    	SetClip(&win->windowrect);
      Paint(wintopmost, &win->windowrect, FALSE);
      win->dirty = 0;
      win->vt.dirtylines = 0;
    }
    else
    if (win->vt.dirtylines)
    {
    	SetClip(&win->contentrect);
      Paint(wintopmost, &win->contentrect, FALSE);
      win->vt.dirtylines = 0;
    }

    win = win->next;
  }
  
  /* drop shadow for top window ? */

  /* focus cursor */
  if (wintopmost && wintopmost->contentrect.h != 0)
    addcursor(wintopmost);


      /* this is need in SDL emulator to flip window buffer */
#ifdef __clang__
      SDLrefreshwin();
#endif

    }
  }

  cleanup_and_exit(0);
}
