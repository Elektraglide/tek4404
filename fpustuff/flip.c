#include <stdio.h>
#include <math.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>
#include <errno.h>

#include "mathf.h"

#define PAGE_FLIP
#define SPRITESxx

#ifndef unix
#info flipdemo
#info Version 1.2
#endif

extern void init3d();
extern int loadmodel();
extern void transform3d();
extern void draw3d();

#define INT2FIX(A)  ((A)<<16)
#define FIX2INT(A)  ((A)>>16)

typedef long fixed;

typedef struct
{
  struct FORM *form;
  fixed x,y;
  fixed dx,dy;

  struct FORM *back[2];
  int oldx[2],oldy[2];
} sprite;

struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
int pindex;
int scrolldir = 1;
struct POINT page;
unsigned long frametime;
unsigned int framenum;

sprite thesprite;
sprite thesprite2;

waitbutton(msg)
char *msg;
{
  struct POINT origin;
  char buffer[64];

     origin.x = 250;
     origin.y = 48;
     sprintf(buffer, "%s: page(%d) dst(%d,%d)", msg, pindex,bb.destrect.x,bb.destrect.y);
     StringDraw(buffer, &origin);

     while (GetButtons() == 0);
     while (GetButtons());
}

void flip()
{

  SetViewport(&page);

  page.y ^= 512;
  pindex ^= 1;


#if 0
  /* horizontal scrolling */
  page.x += scrolldir;
  if (page.x < 1 || page.x > 384) scrolldir = -scrolldir;
#endif
}

void sh_timer(sig)
int sig;
{

  frametime += 33;
  framenum++;

  signal(sig, sh_timer);
  ESetAlarm(frametime);
}


void restoreunder(asprite)
sprite *asprite;
{
#if 1
    register unsigned long* dst = ((char *)screen->addr) + 
    (asprite->oldy[pindex] << 7) + 
    ((asprite->oldx[pindex] & 0xffe0) >> 3);
    
    register unsigned long* src = asprite->back[pindex]->addr;
    register unsigned short i;

  if (asprite->oldx[pindex] < 0)
    return;

    /* restore previous */

    i = 128;
    while (i--)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst[8] = src[8];

        dst += 32;
        src += 9;
    }
#endif

#if 0
    bb.srcform = asprite->back[pindex];
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = asprite->oldx[pindex];
    bb.destrect.y = asprite->oldy[pindex];
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbS;
	BitBlt(&bb);
#endif
}

void saveunder(asprite)
sprite *asprite;
{
    /* save under */
    int sx = FIX2INT(asprite->x);
    int sy = FIX2INT(asprite->y) + (page.y);

#if 1
    register unsigned long* dst = asprite->back[pindex]->addr;
    register unsigned long* src = ((char *)screen->addr) + 
    (sy << 7) + 
    ((sx & 0xffe0) >> 3);
    register unsigned short i;

    i = 128;
    while (i--)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst[8] = src[8];
        dst += 9;
        src += 32;
    }
#endif

#if 0
    bb.srcform = screen;
    bb.srcpoint.x = sx;
    bb.srcpoint.y = sy;
    bb.destform = asprite->back[pindex];
	bb.destrect.x = 0;
    bb.destrect.y = 0;
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbS;
	BitBlt(&bb);
#endif
    asprite->oldx[pindex] = sx;
    asprite->oldy[pindex] = sy;
}

void drawsprite(asprite)
sprite *asprite;
{
    /* draw over */
    bb.srcform = asprite->form;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = FIX2INT(asprite->x);
    bb.destrect.y = FIX2INT(asprite->y) + (page.y);
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbSorD;
	BitBlt(&bb);

}

struct FORM *makeform()
{
  struct POINT origin,p1,p2;
  struct RECT r;
  struct FORM *form;
  int i;
     
   form = FormCreate(256,128);
   
   if (font && form)
   {
     /* NB text is formatted WRT the destrect, so make it large */
     bb.destform = form;
     bb.destrect.x = 0;
     bb.destrect.y = 0;
     bb.destrect.w = 1024;
     bb.destrect.h = 1024;
     bb.rule = bbSorD;
     bb.halftoneform = NULL;
     bb.cliprect.x = 0;
     bb.cliprect.y = 0;
     bb.cliprect.w = form->w;
     bb.cliprect.h = form->h;

     i = 16;
     while(--i >= 0)
     {
       if (i == 0) bb.rule = bbSnandD;
       
       origin.x = 0 + i;
       origin.y = font->maps->line + i;
       StringDrawX("Tektronix\n  4404", &origin, &bb, font);
     }


     bb.rule = bbSorD;
     bb.halftoneform = &GrayMask;
     bb.srcform = NULL;
     r.x = r.y = 4;
     r.w = 252-8;
     r.h = 124-8;
     RectBoxDrawX(&r,4, &bb);
   }

   return form;
}

void movesprite(asprite)
sprite *asprite;
{
    asprite->x += asprite->dx;
    if (asprite->x < 0) 
    {
      asprite->x = INT2FIX(page.x); asprite->dx = -asprite->dx;
    }
    else
    if (asprite->x > INT2FIX(640-256)) 
    {
      asprite->x = INT2FIX(640-256); asprite->dx = -asprite->dx;
    }

    asprite->y += asprite->dy;
    if (asprite->y > INT2FIX(480-128)) 
    {
      asprite->y = INT2FIX(480-128);
      asprite->dy = -asprite->dy;
    }

    /* gravity and clamp speed */
    asprite->dy += 8000; 
    if (asprite->dy > INT2FIX(50)) asprite->dy = INT2FIX(50);

}

void makesprite(asprite, dx, dy)
sprite* asprite;
fixed dx, dy;
{
    asprite->form = makeform();
    asprite->x = INT2FIX(60);
    asprite->y = INT2FIX(10);
    asprite->oldx[0] = asprite->oldx[1] = -1000;

    asprite->back[0] = FormCreate(256+32, 128);
    asprite->back[1] = FormCreate(256+32, 128);
    asprite->dx = dx;
    asprite->dy = dy;
}

void sh_int(sig)
int sig;
{
  struct POINT p;
  p.x = p.y = 0;
  SetViewport(&p);
  ExitGraphics();
  FontClose(font);

  exit(2);
}


int *mapped_fpu;
unsigned int oldwaiting[2];
main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i;
    fixed x,y,dx,dy;
    struct POINT origin,p2;
    struct RECT r;
    unsigned int currpage,waiting;
    int angle;
    
    /* map FPU into our memory space */
	mapped_fpu = phys(2);
	if (mapped_fpu == NULL)
	{
		fprintf(stderr, "no FPU mapped\n");
		exit(2);
	}

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    font = FontOpen("/fonts/PellucidaSans-Serif36.font");
    if (font)
    {
		fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
	      font->maps->fixed,font->maps->maxw, 
	      font->maps->line, font->maps->baseline);
	}
    else
    {
      fprintf(stderr, "FontOpen: FAILED: %s\n",strerror(errno));
      return;
    }
	
    screen = InitGraphics(FALSE);

    /* for controlling clipping etc */
    BbcomDefault(&bb);

    bb.srcform = screen;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
    bb.destrect.x = 0;
    bb.destrect.y = 512;
    bb.destrect.w = 1024;
    bb.destrect.h = 512;

      /* clear page 1 to gray */
      bb.srcform = NULL;
      bb.rule = bbS;
      bb.halftoneform = &GrayMask;
      RectDrawX(&bb.destrect, &bb);


   /* make a sprite to use */
    makesprite(&thesprite, INT2FIX(3), INT2FIX(1));
    makesprite(&thesprite2, INT2FIX(-5), INT2FIX(2));

    /* set up the bitblt command stuff */
    bb.srcform = (struct FORM *)NULL;		/* no source */
    bb.destform = screen;			/* dest is screen */
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;
    bb.halftoneform = &BlackMask;
 	bb.rule = bbS;
    bb.cliprect.x = 0;
    bb.cliprect.y = 0;
    bb.cliprect.w = 1024;
    bb.cliprect.h = 1024;

   init3d();
   if (argc > 1)
   {
   	 loadmodel(argv[1]);
   }

   /* hide cursor */
   CursorVisible(FALSE);

    /* ESetSignal(); */
    page.x = 0;
    page.y = 0;
    pindex = 0;
    frametime = EGetTime() + 500;
    framenum = 0;
#ifdef PAGE_FLIP
    printf("starting alarm\n");
    ESetAlarm(frametime);
#endif

    /* copy page0 to page1 */
    bb.srcform = screen;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
    bb.destrect.x = 0;
    bb.destrect.y = 512;
    bb.destrect.w = 1024;
    bb.destrect.h = 512;
 	bb.rule = bbS;
	BitBlt(&bb);
	bb.srcform = NULL;

   /* animate it */
   angle = 5;
   while(1)
   {
#ifdef SPRITES
     restoreunder(&thesprite);
/*     restoreunder(&thesprite2);  */
     movesprite(&thesprite);
/*     movesprite(&thesprite2);  */
     saveunder(&thesprite);
/*     saveunder(&thesprite2);   */
#endif

     angle += 2;
     transform3d(angle);  
     draw3d();

#ifdef SPRITES
     drawsprite(&thesprite);
/*     drawsprite(&thesprite2);   */
#endif

#ifdef PAGE_FLIP
     /* show draw time by how long we waited for VBLANK */
     waiting >>= 4;
     bb.srcform = NULL;
     bb.rule = bbS;
     r.x = page.x;
     r.y = page.y;
     r.w = waiting ;
     r.h = 4;
     RectDrawX(&r, &bb);
     if (waiting < oldwaiting[pindex])
     {
         bb.rule = bbZero;
         r.x += r.w;
         r.w = 2 + oldwaiting[pindex] - waiting;
         RectDrawX(&r, &bb);
     }
     oldwaiting[pindex] = waiting;

     /* wait for frame flip */
     waiting = 0;
     currpage = framenum;
     while (currpage == framenum)
         waiting++;
     currpage = framenum;

     flip();

     /* ensure 1 vblank has happened */
     while (currpage == framenum);
#endif

   }


}
