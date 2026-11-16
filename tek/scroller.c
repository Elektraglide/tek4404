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

  frametime += 16;
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

#define MAPWIDTH 1024
#define MAPHEIGHT 30
unsigned char cellmap[MAPHEIGHT][MAPWIDTH];
struct FORM* tile[8];

fillcol(x,y,cellx)
int x,y;
int cellx;
{
    register unsigned char *cellcol;
    short i;

    bb.halftoneform = NULL;
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.rule = bbS;
    cellcol = (unsigned char *)cellmap + cellx;
    for (i = 0; i < MAPHEIGHT; i++)
    {
        bb.srcform = tile[(*cellcol) & 7];
        BitBlt(&bb);
        bb.destrect.y += 16;
        cellcol += MAPWIDTH;
    }
}

void fillscreen(pixx,y)
int pixx,y;
{
    int i, pagex, fbx, cellx;

    /* which cell are we starting on */
    cellx = pixx >> 4;

    /* which framebuffer window are we in? */
    fbx = pixx / 384;

    /* find scroll window offset */
    pagex = pixx - (fbx * 384);

    for(i=0; i<40; i++)
    {
        fillcol(pagex + (i << 4), y, cellx + i);
    }
}

void showinfo(buffer)
char *buffer;
{
    struct POINT origin;

    /* avoid wrapping text */
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;

    origin.x = page.x;
    origin.y = page.y + font->maps->baseline;
    StringDraw(buffer, &origin);
}


int *mapped_fpu;
unsigned int oldwaiting[2];
main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i, col, oldleftcol,oldrightcol;
    fixed pfx,pfdir;
    struct POINT origin,p2;
    struct RECT r;
    unsigned int currpage,waiting;
    float angle;
    char infoline[64];

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
    tile[0] = maketile("  ");
    tile[1] = maketile("1 ");
    tile[2] = maketile("2 ");
    tile[3] = maketile("3 ");
    tile[4] = maketile("4 ");
    tile[5] = maketile("5 ");
    tile[6] = maketile("6 ");
    tile[7] = maketile("7 ");
    bb.destform = screen;

    /* random playfield */
    for (col = 0; col < MAPWIDTH; col += 8)
    {
        i = rand()% 25;
        cellmap[i][col + 0] = col / 8;
        cellmap[i][col + 1] = col / 8;
        cellmap[i][col + 2] = col / 8;
        cellmap[i][col + 3] = col / 8;
        i++;
        cellmap[i][col + 0] = col / 8;
        cellmap[i][col + 1] = rand();
        cellmap[i][col + 2] = rand();
        cellmap[i][col + 3] = col / 8;
        i++;
        cellmap[i][col + 0] = col / 8;
        cellmap[i][col + 1] = col / 8;
        cellmap[i][col + 2] = col / 8;
        cellmap[i][col + 3] = col / 8;
    }

    /* playfield start position */
    pfx = INT2FIX(0);
    pfdir = 128;

    /* initial screen fill */
    fillscreen(FIX2INT(pfx), 0);
    fillscreen(FIX2INT(pfx), 512);

    /* animate it */
    while(1)
    {
        int fbx, cellx, pixx;

        /* top left coordinate */
        pixx = FIX2INT(pfx);

        /* which cell are we starting on */
        cellx = pixx >> 4;

        /* which framebuffer window are we in? */
        fbx = pixx / 384;

        /* find scroll window offset */
        page.x = pixx - (fbx * 384);
        if (argc == 1) SetViewport(&page);

        /* infoline */
        sprintf(infoline, "pfx(%d) cellx(%d) fbx(%d)", pixx, cellx, fbx);

        /* has leading edge column changed? render a new column */
        col = (page.x + 640) >> 4;
        if (col != oldrightcol)
        {
            oldrightcol = col;

            /* fill new right column just out of view */
            fillcol(col << 4, page.y, cellx + 40);

            /* fill on next page too */
            col = (page.x - 1) >> 4;
            fillcol(256 + (col << 4), page.y ^ 512, cellx + 40);

            /* the region that overlaps far left and far right scroll window position also needs filling */
            if (col < 16)
            {
                fillcol((col << 4), page.y ^ 512, cellx + 24);
            }
        }

        showinfo(infoline);

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

        /* have we crossed into new framebuffer window and need to flip? */
        if (fbx != (FIX2INT(pfx) / 384))
        {
            page.y ^= 512;
        }
    }
}
