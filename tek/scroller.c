#include <stdio.h>
#include <math.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

#ifndef unix
#info scrolldemo
#info Version 1.0
#tstmp
#endif

#define INT2FIX(A)  ((A)<<8)
#define FIX2INT(A)  ((A)>>8)

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


#if 1
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
    if (asprite->x < INT2FIX(page.x)) 
    {
      asprite->x = INT2FIX(page.x); asprite->dx = -asprite->dx;
    }
    if (asprite->x > INT2FIX(page.x+640-256)) 
    {
      asprite->x = INT2FIX(page.x+640-256); asprite->dx = -asprite->dx;
    }

    asprite->y += asprite->dy;
    if (asprite->y > INT2FIX(480-128)) 
    {
      asprite->y = INT2FIX(480-128);
      asprite->dy = -asprite->dy;
    }

    /* gravity */
    asprite->dy += 8000; 
    if (asprite->dy > INT2FIX(50)) asprite->dy = INT2FIX(50);

}

void makesprite(asprite, dx, dy)
sprite* asprite;
fixed dx, dy;
{
    asprite->form = makeform();
    asprite->x = INT2FIX(20);
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

struct FORM *maketile(name)
char *name;
{
  struct FORM *tile;
  struct POINT origin;
  struct RECT r;

    tile = FormCreate(16, 16);
    bb.destform = tile;
    bb.rule = bbS;
    bb.halftoneform = &WhiteMask;
    r.x = r.y = 0;
    r.w = tile->w;
    r.h = tile->h;
    RectDrawX(&r, &bb);

    bb.rule = bbS;
    bb.halftoneform = NULL;
    origin.x = 0;
    origin.y = font->maps->baseline;
    StringDrawX(name, &origin, &bb, font);

    bb.halftoneform = &BlackMask;
    r.x = r.y = 0;
    r.w = tile->w - 1;
    r.h = tile->h - 1;
    RectBoxDrawX(&r, 1, &bb);

    return tile;
}

fillcol(x,y,tile)
int x,y;
struct FORM *tile;
{
    short i;

    bb.srcform = tile;
    bb.halftoneform = NULL;
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.rule = bbS;
    for (i = 0; i < 480; i += 16)
    {
        BitBlt(&bb);
        bb.destrect.y += 16;
    }
}

void showinfo(buffer)
char *buffer;
{
    struct POINT origin;

    origin.x = 250;
    origin.y = page.y + font->maps->baseline;
    StringDraw(buffer, &origin);
}


unsigned char cellmap[32][256];

int *mapped_fpu;
unsigned int oldwaiting[2];
main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i,col, oldleftcol,oldrightcol;
    fixed pfx,pfdir;
    struct POINT origin,p2;
    struct RECT r;
    unsigned int currpage,waiting;
    float angle;
    struct FORM *tile[4];
    int tilex;
    char buffer[64];

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    screen = InitGraphics(TRUE);

    font = FontOpen("/fonts/PellucidaSans-Serif10.font");
    fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
      font->maps->fixed,font->maps->maxw, 
      font->maps->line, font->maps->baseline);

    ESetSignal();
    page.x = page.y = 0;
    pindex = 0;
    frametime = EGetTime() + 500;
    framenum = 0;
    ESetAlarm(frametime);

    /* for controlling clipping etc */
    BbcomDefault(&bb);

    bb.srcform = NULL;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;
    bb.halftoneform = NULL;
 	bb.rule = bbS;
    bb.cliprect.x = 0;
    bb.cliprect.y = 0;
    bb.cliprect.w = 1024;
    bb.cliprect.h = 1024;

   /* some tiles */
   tile[0] = maketile("0 ");
   tile[1] = maketile("1 ");
   tile[2] = maketile("2 ");
   tile[3] = maketile("3 ");
   bb.destform = screen;

   /* playfield start position */
   pfx = INT2FIX(0);
   pfdir = 1024;

   /* animate it */
   while(1)
   {
       int fbx;

        /* which cell are we starting on */
        col = FIX2INT(pfx) >> 4;

        /* which framebuffer window are we in? */
        fbx = FIX2INT(pfx) / 384;

        /* position scroll window relative to this framebuffer window */
        page.x = FIX2INT(pfx) - (fbx * 384);
        if (argc == 1) SetViewport(&page);

        sprintf(buffer, "pfx(%d) col(%d) fbx(%d)", FIX2INT(pfx), col, fbx);

        /* has leading edge column changed? */
        col = (page.x + 640) >> 4;
        if (col != oldrightcol)
        {
            oldrightcol = col;

            /* fill new right column just out of view */
            fillcol(col << 4, page.y, tile[0]);
        }

        /* has trailing column changed? */
        col = (page.x - 1) >> 4;
        if (col != oldleftcol && col >= 0)
        {
            oldleftcol = col;

            /* backfill new left column on next row */
            fillcol(256 + (col << 4), page.y ^ 512, tile[0]);

            /* amortised 0-256 */
            if (col < 16)
            {
                fillcol((col << 4), page.y ^ 512, tile[3]);
            }
        }

        showinfo(buffer);

        /* wait for frame flip */
         waiting = 0;
         currpage = framenum;
         while (currpage == framenum)
             waiting++;
         currpage = framenum;
         /* ensure 1 vblank has happened */
         while (currpage == framenum);

         /* scroll */
         pfx += pfdir;

         /* do we need to swap to next row? */
         if (fbx != (FIX2INT(pfx) / 384))
         {
             struct FORM *tmp;

             page.y ^= 512;

             tmp = tile[0];
             tile[0] = tile[1];
             tile[1] = tile[2];
             tile[2] = tile[3];
             tile[3] = tmp;
         }
   }
}
