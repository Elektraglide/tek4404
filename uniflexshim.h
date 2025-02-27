typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned int caddr_t;

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/select.h>
#include <util.h>
// #include <sys/sgtty.h> filename clashes with Tek4404 header so we need to include its contents
#include <sys/cdefs.h>
#define  USE_OLD_TTY
#define _SGTTYB_
#include <sys/ioctl.h>
#define  gtty(fd, buf)  ioctl(fd, TIOCGETP, buf)
#define  stty(fd, buf)  ioctl(fd, TIOCSETP, buf)


// unix kernel calls without stdlib.h
extern void *malloc();
extern void free();
extern void exit();

extern int strcmp();
extern char *strerror(int errnum);
extern int errno;

#undef gethostname
static char *nget_str(char *name)
{
	static char buffer[64];
	
  if (!strcmp(name, "cur_tty"))
    return ttyname(fileno(stdin));
		
  if (!strcmp(name, "console"))
    return "/dev/console";

  if (!strcmp(name, "my_name"))
  {
  	gethostname(buffer, sizeof(buffer));
    return buffer;
	}

  return "unknown";
}

static char *nserror()
{
  return strerror(errno);
}

// implementations of Uniflex calls
static int create_pty(int *ptfd)
{

  // NB master,slave is reverse of slave,master on Tek
  return openpty(&ptfd[1], &ptfd[0], NULL, NULL, NULL);;
}

#ifdef sgtty_h

static int control_pty(int fd, int code, int cval)
{
  int i;
  
  if (code ==  PTY_INQUIRY)
  {
    // should be only if PTY_READ_WAIT
    return PTY_OUTPUT_QUEUED;
  }

  if (code == PTY_SET_MODE)
  {
	  if (cval & PTY_REMOTE_MODE)
    {
      i = (cval & PTY_REMOTE_MODE);
  	  //ioctl(fd, TIOCREMOTE, &i);
    }

	  if (cval & PTY_READ_WAIT)
      tcdrain(fd);

#if 0
	  if (cval & PTY_HANDSHAKE_MODE)
  	  ioctl_tty(fd, TIOCGPKT, 0);

	  if (cval & PTY_SLAVE_HOLD)
  	  ioctl_tty(fd, TIOCGPKT, 0);

	  if (cval & PTY_EOF)
  	  ioctl_tty(fd, TIOCGPKT, 0);

	  if (cval & PTY_OUTPUT_QUEUED)
  	  ioctl_tty(fd, TIOCGPKT, 0);

	  if (cval & PTY_INPUT_QUEUED)
  	  ioctl_tty(fd, TIOCGPKT, 0);
#endif
      return(0);
    }

  if (code == PTY_START_OUTPUT)
  {
      ioctl(fd, TIOCSTART , 0);
      return(0);
  }

  if (code == PTY_STOP_OUTPUT)
  {
      ioctl(fd, TIOCSTOP , 0);
      return(0);
  }

  if (code == PTY_FLUSH_READ)
  {
      tcflush(fd, TCIFLUSH);
      return(0);
  }

  if (code == PTY_FLUSH_WRITE)
  {
      tcflush(fd, TCOFLUSH);
      return(0);
  }

    return 0;
}
#endif

static char *phys(int code)
{
  switch(code)
    {
      // framebuffer memory
      case 1:
        return 0;
      case -1:
        return 0;

      default:
        fprintf(stderr, "phys:  unknown resource %d\n", code);
        return 0;
    }

}

#ifdef bbZeros

extern void SDLrefreshwin();

extern void SaveDisplayState(struct DISPSTATE *state);
extern void RestoreDisplayState(struct DISPSTATE *state);

extern int BbcomDefault(struct BBCOM *bbcom);

extern void ClearScreen();
static void CursorVisible(int mode) {};
static void CursorTrack(int mode) {};

extern int ReleaseCursor();
extern int ProtectCursor(struct RECT *rl,struct RECT *r2);
extern int PanCursorEnable(int mode);
extern int PanDiskEnable(int mode);

extern int CharWidth(char ch, struct FontHeader *font);
extern int CharDraw(char ch, struct POINT *loc);
extern int CharDrawX(char ch, struct POINT *loc, struct BBCOM *bbcom, struct FontHeader *font);
extern int StringWidth(char *string,struct FontHeader *font);
extern int StringDraw(char *ch, struct POINT *loc);
extern int StringDrawX(char *ch, struct POINT *loc, struct BBCOM *bbcom, struct FontHeader *font);

extern int BitBlt(struct BBCOM *bbcom);

extern int SetKBCode(int val);
extern int EGetCount();
extern int EGetNewCount();
extern unsigned long EGetNext();
extern unsigned long EGetTime();
extern int ESetSignal();
extern int ESetAlarm(unsigned long time);

extern int EventDisable();
extern int EventEnable();

extern int FontClose(struct FontHeader *font);

extern struct FontHeader * FontOpen(char *filename);

extern int GetButtons();
extern int GetMBounds(struct POINT *ulpoint,struct POINT *lrpoint);
extern int GetMPosition(struct POINT *point);
extern int SetMPosition(struct POINT *point);

extern int GetRealMachineType();

extern int GetViewport(struct POINT *point);
extern int SetViewport(struct POINT *point);

extern int PointDistance(struct POINT *point1, struct POINT *point2);

extern int PointFromUser(struct POINT *point);

extern int PointMax(struct POINT *point1,struct POINT *point2,struct POINT *point3);
extern int PointMidpoint(struct POINT *point1,struct POINT *point2,struct POINT *point3);
extern int PointMin(struct POINT *point1,struct POINT *point2,struct POINT *point3);
extern int PointsToRect(struct POINT *point1,struct POINT *point2, struct RECT *rect);
extern void PointToRC(int *row,int *col, struct POINT *point);

extern void RCToRect(struct RECT *rect, int row, int col);

extern int RectBoxDraw(struct RECT *rect, int width);

extern int RectBoxDrawX(struct RECT *rect, int width, struct BBCOM *bbcom);

extern int RectContainsPoint(struct RECT *rect, struct POINT *point);

extern int RectContainsRect(struct RECT *rect1, struct RECT *rect2);

extern int RectDraw(struct RECT *rect);

extern int RectDrawX(struct RECT *rect, struct BBCOM *bbcom);

extern int RectDebug(struct RECT *rect, int r, int g, int b);

extern int RectFromUser(struct RECT *rect);

extern int RectFromUserX(struct POINT *minsize, struct FORM *mask, struct RECT *rect);

extern int RectAreasDiffering(struct RECT *rect1,struct RECT *rect2, struct QUADRECT *quadrect);

extern int RectAreasOutside(struct RECT *rect1,struct RECT *rect2, struct QUADRECT *quadrect);

extern int RectIntersect(struct RECT *rect1,struct RECT *rect2,struct RECT *rect3);

extern int RectIntersects(struct RECT *rect1, struct RECT *rect2);

extern int RectMerge(struct RECT *rect1,struct RECT *rect2,struct RECT *rect3);

extern int MenuSelect();

extern void ExitGraphics();

extern struct FORM *InitGraphics(int mode);

#endif

