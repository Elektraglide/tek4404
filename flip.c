#include <stdio.h>
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
  fixed x,y;
  fixed dx,dy;

  struct FORM *back[2];
  int oldx[2],oldy[2];
} sprite;

struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
int pindex;
struct POINT page;

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
  page.x++;
  if (page.x > 384) page.x = 0;
#endif
}

void sh_timer(sig)
int sig;
{
  unsigned long nexttime;
  nexttime = EGetTime() + 50;

  signal(sig, sh_timer);
  ESetAlarm(nexttime);
}


void restoresprite(asprite)
sprite *asprite;
{

    /* restore previous */
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
}

void drawsprite(asprite)
sprite *asprite;
{

    /* save under */
    bb.srcform = screen;
    bb.srcpoint.x = FIX2INT(asprite->x);
    bb.srcpoint.y = FIX2INT(asprite->y) + (page.y);
    bb.destform = asprite->back[pindex];
	bb.destrect.x = 0;
    bb.destrect.y = 0;
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
 	bb.rule = bbS;
	BitBlt(&bb);
    asprite->oldx[pindex] = bb.srcpoint.x;
    asprite->oldy[pindex] = bb.srcpoint.y;


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

struct FORM *makesprite()
{
  struct POINT origin,p1,p2;
  struct RECT r;
  struct FORM *form;
   
   form = FormCreate(64,64);
   
   if (font && form)
   {
     /* NB text is formatted WRT the destrect, so make it large */
     bb.destform = form;
     bb.destrect.x = 0;
     bb.destrect.y = 0;
     bb.destrect.w = 1024;
     bb.destrect.h = 1024;
     bb.rule = bbS;
     bb.halftoneform = &BlackMask;
     bb.cliprect.x = 0;
     bb.cliprect.y = 0;
     bb.cliprect.w = form->w;
     bb.cliprect.h = form->h;

     origin.x = 0;
     origin.y = font->maps->line;
     StringDrawX("Tek", &origin, &bb, font);

   }

   return form;
}

void movesprite(asprite)
sprite *asprite;
{
    asprite->x += asprite->dx;
    if (asprite->x < INT2FIX(page.x)) 
    {
      asprite->x = INT2FIX(page.x); asprite->dx = -asprite->dx;
    }
    if (asprite->x > INT2FIX(page.x+640-64)) 
    {
      asprite->x = INT2FIX(page.x+640-64); asprite->dx = -asprite->dx;
    }

    asprite->y += asprite->dy;
    if (asprite->y > INT2FIX(480-64)) 
    {
      asprite->y = INT2FIX(480-64);
      asprite->dy = -asprite->dy;
    }

    /* gravity */
    asprite->dy += 4000; 
    if (asprite->dy > INT2FIX(50)) asprite->dy = INT2FIX(50);

}

main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i;
    fixed x,y,dx,dy;
    struct POINT origin,p2;
    struct RECT r;
    long frametime;

    signal(SIGMILLI, flip);

    screen = InitGraphics(FALSE);
    font = FontOpen("/fonts/PellucidaSans-Serif36.font");
   fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
      font->maps->fixed,font->maps->maxw, 
      font->maps->line, font->maps->baseline);

    printf("starting alarm\n");
    page.x = page.y = 0;
    pindex = 0;

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
   thesprite.oldx[0] = thesprite.oldx[1] = -1000;

   thesprite.back[0] = FormCreate(64,64);
   thesprite.back[1] = FormCreate(64,64);
   thesprite.dx = INT2FIX(3);
   thesprite.dy = INT2FIX(1);

   /* make a sprite2 to use */
   thesprite2.form = makesprite();
   thesprite2.x = INT2FIX(200);
   thesprite2.y = INT2FIX(40);
   thesprite2.oldx[0] = thesprite2.oldx[1] = -1000;

   thesprite2.back[0] = FormCreate(64,64);
   thesprite2.back[1] = FormCreate(64,64);
   thesprite2.dx = INT2FIX(-5);
   thesprite2.dy = INT2FIX(1);


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

   /* animate it */
   while(1)
   {
     frametime = EGetTime() + 50;

     restoresprite(&thesprite);
     restoresprite(&thesprite2);

     movesprite(&thesprite);
     movesprite(&thesprite2);

     drawsprite(&thesprite);
     drawsprite(&thesprite2);

     /* need an accurate way of flipping at 30Hz */
     flip();

     /* wait for 20Hz */
     while (frametime > EGetTime());

   }

    sleep(4);
    ClearScreen();
}
