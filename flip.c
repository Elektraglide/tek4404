#include <font.h>
#include <graphics.h>
#include <signal.h>

#info flipdemo
#info Version 1.0
#tstmp

#define INT2FIX(A)  ((A)<<16)
#define FIX2INT(A)  ((A)>>16)

typedef long fixed;

typedef struct
{
  struct FORM *form;
  struct FORM *back[2];
  fixed x,y;
  int oldx[2],oldy[2];
} sprite;

struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;

sprite thesprite;


struct POINT page;
void flip()
{

  SetViewport(&page);
  page.y ^= 512;

#if 0
  /* horizontal scrolling */
  page.x++;
  if (page.x > 384) page.x = 0;
#endif
}

void drawsprite(asprite)
sprite *asprite;
{
  struct FORM *tmp;

#if 0
	bb.destrect.x = asprite->oldx[0];
    bb.destrect.y = asprite->oldy[0];
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = 64;
	bb.rule = bbZero;
	BitBlt(&bb);
#endif

    bb.srcform = asprite->back[0];
	bb.destrect.x = asprite->oldx[0];
    bb.destrect.y = asprite->oldy[0];
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbSorD;
	BitBlt(&bb);
    asprite->oldx[0] = asprite->oldx[1];
    asprite->oldy[0] = asprite->oldy[1];
    tmp = asprite->back[0];
    asprite->back[0] = asprite->back[1];
    asprite->back[1] = tmp;

    /* save under */
    bb.srcform = screen;
    bb.destform = asprite->back[1];
	bb.destrect.x = FIX2INT(asprite->x);
    bb.destrect.y = FIX2INT(asprite->y) + (page.y);
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbS;
	BitBlt(&bb);

    /* draw over */
    bb.srcform = asprite->form;
    bb.destform = screen;
	bb.destrect.x = FIX2INT(asprite->x);
    bb.destrect.y = FIX2INT(asprite->y) + (page.y);
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbSorD;
	BitBlt(&bb);
    asprite->oldx[1] = bb.destrect.x;
    asprite->oldy[1] = bb.destrect.y;
    
}

void sh_timer(sig)
int sig;
{
  unsigned long nexttime;
  nexttime = EGetTime() + 50;

  signal(sig, sh_timer);
  ESetAlarm(nexttime);
}

struct FORM *makesprite()
{
  struct POINT origin;
  struct RECT r;
  struct FORM *form;
   
   form = FormCreate(64,64);
   
   font = FontOpen("/fonts/PellucidaSans-Serif36.font");
   if (font)
   {
     bb.destform = form;
     bb.halftoneform = &BlackMask;
     bb.destrect.x = 0;
     bb.destrect.y = 0;
     bb.destrect.w = form->w;
     bb.destrect.h = form->h;

     r.x = 5;
     r.y = 5;
     r.w = 20;
     r.h = 40;
     RectDrawX(&r, &bb);

     origin.x = 0;
     origin.y = 48;
     StringDrawX("ABaj", &origin, &bb, font);
   }

   return form;
}

main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i;
    fixed x,y,dx,dy;
    struct POINT origin;

    signal(SIGMILLI, flip);

    screen = InitGraphics(FALSE);

    printf("starting alarm\n");
    page.x = page.y = 0;
    ESetSignal();
    ESetAlarm(EGetTime() + 500);

    /* for controlling clipping etc */
    BbcomDefault(&bb);

    bb.srcform = screen;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;			/* dest is screen */
    bb.destrect.x = 0;
    bb.destrect.y = 512;
    bb.destrect.w = 1024;
    bb.destrect.h = 512;

      /* clear page 1 to gray */
      bb.srcform = NULL;
      bb.rule = bbS;
      bb.halftoneform = &GrayMask;
      RectDrawX(&bb.destrect, &bb);

    /* copy page0 to page1 */
    bb.srcform = screen;
    bb.halftoneform = &BlackMask;
    bb.destrect.h = 480;
 	bb.rule = bbS;
	BitBlt(&bb);

   /* make a sprite to use */
   thesprite.form = makesprite();
   thesprite.x = INT2FIX(20);
   thesprite.y = INT2FIX(10);

   thesprite.back[0] = FormCreate(64,64);
   thesprite.back[1] = FormCreate(64,64);

    /* set up the bitblt command stuff */
    bb.srcform = (struct FORM *)NULL;		/* no source */
    bb.destform = screen;			/* dest is screen */
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;
    bb.halftoneform = &BlackMask;
 	bb.rule = bbS;

   /* animate it */
   dx = INT2FIX(3);
   dy = INT2FIX(1);
   while(1)
   {
     drawsprite(&thesprite);

     thesprite.x += dx;
     if (thesprite.x < INT2FIX(page.x)) {thesprite.x = INT2FIX(page.x); dx = -dx;}
     if (thesprite.x > INT2FIX(page.x+640-64)) {thesprite.x = INT2FIX(page.x+640-64); dx = -dx;}

     thesprite.y += dy;
     if (thesprite.y > INT2FIX(480-64)) dy = -dy;

     /* gravity */
     dy += 4000; 
     if (dy > INT2FIX(50)) dy = INT2FIX(50);

     ret = 3000;
     while(--ret > 0);

     flip();
   }

    sleep(4);
    ClearScreen();
}
