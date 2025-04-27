#include <stdio.h>
#include <math.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

#include "mathf.h"

#ifndef unix
#info flipdemo
#info Version 1.0
#tstmp
#endif


typedef struct
{
    vec3 wc;
    vec3 cc;
    short vpx, vpy;
} Vertex3d;

real zero,fov,one,two,camdist;
vec3 spinaxis;


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


short vcount;
Vertex3d vertices[500];

short lcount;
unsigned short lines[500][2];

mat33 ltm;

int addvertex(x, y, z)
int x, y, z;
{
    vertices[vcount].wc.x.fv = x * 1.0;
    vertices[vcount].wc.y.fv = y * 1.0;
    vertices[vcount].wc.z.fv = z * 1.0;
    
    return vcount++;
}

void addline(a, b)
int a, b;
{
    lines[lcount][0] = a;
    lines[lcount][1] = b;
    lcount++;
}

void init3d()
{
    zero.fv = 0.0;
    fov.fv = 2.0;
    one.fv = 1.0;
    two.fv = 2.0;
    camdist.fv = 5.0;

    spinaxis.x.fv = 0.666;
    spinaxis.y.fv = 0.666;
    spinaxis.z.fv = 0.333;

    vcount = 0;

    addvertex(-1, -1, -1);
    addvertex( 1, -1, -1);
    addvertex( 1,  1, -1);
    addvertex(-1,  1, -1);

    addvertex(-1, -1,  1);
    addvertex( 1, -1,  1);
    addvertex( 1,  1,  1);
    addvertex(-1,  1,  1);

    lcount = 0;

    addline(0, 1);
    addline(1, 2);
    addline(2, 3);
    addline(3, 0);

    addline(4, 5);
    addline(5, 6);
    addline(6, 7);
    addline(7, 4);

    addline(0, 4);
    addline(1, 5);
    addline(2, 6);
    addline(3, 7);
    
    printf("model has %d vertices %d edges\n", vcount, lcount);
}

void scalev(s, v)
int s;
vec3* v;
{
    v->x.iv = mulf(v->x.iv, s);
    v->y.iv = mulf(v->y.iv, s);
    v->z.iv = mulf(v->z.iv, s);
}

void rotatem(mat, radians, v)
mat33* mat;
double radians;
vec3* v;
{
    vec3 leading,crossed,scaled;
    real a, cosf, sinf;

    a.fv = cos(radians);
    cosf.iv = subf(one.iv, a.iv);
    sinf.fv = sin(radians);

    leading.x.iv = subf(one.iv, mulf(v->x.iv, v->x.iv));
    leading.y.iv = subf(one.iv, mulf(v->y.iv, v->y.iv));
    leading.z.iv = subf(one.iv, mulf(v->z.iv, v->z.iv));
    scalev(cosf.iv, &leading);

    crossed.x.iv = mulf(v->y.iv, v->z.iv);
    crossed.y.iv = mulf(v->z.iv, v->x.iv);
    crossed.z.iv = mulf(v->x.iv, v->y.iv);
    scalev(cosf.iv, &crossed);

    scaled.x.iv = v->x.iv;
    scaled.y.iv = v->y.iv;
    scaled.z.iv = v->z.iv;
    scalev(sinf.iv, &scaled);

    mat->row0.x.iv = subf(one.iv, leading.x.iv);
    mat->row0.y.iv = addf(crossed.z.iv, scaled.z.iv);
    mat->row0.z.iv = subf(crossed.y.iv, scaled.y.iv);
    mat->row1.x.iv = subf(crossed.z.iv, scaled.z.iv);
    mat->row1.y.iv = subf(one.iv, leading.y.iv);
    mat->row1.z.iv = addf(crossed.x.iv, scaled.x.iv);
    mat->row2.x.iv = addf(crossed.y.iv, scaled.y.iv);
    mat->row2.y.iv = subf(crossed.x.iv, scaled.x.iv);
    mat->row2.z.iv = subf(one.iv, leading.z.iv);
}

void rotateY(mat, radians)
mat33* mat;
double radians;
{
    float cf = cos(radians);
    float sf = sin(radians);

    mat->row0.x.fv = cf;
    mat->row0.y.iv = zero.iv;
    mat->row0.z.fv = -sf;
    mat->row1.x.iv = zero.iv;
    mat->row1.y.iv = one.iv;
    mat->row1.z.iv = zero.iv;
    mat->row2.x.fv = sf;
    mat->row2.y.iv = zero.iv;
    mat->row2.z.fv = cf;
}



void transform3d(mat, radians)
mat33* mat;
double radians;
{
    Vertex3d* src = vertices;
    int count = vcount;
    real vpw, vph;

    rotatem(mat, radians, &spinaxis);

    vpw.fv = 240.0;
    vph.fv = 240.0;
    while (count--)
    {
    	real ooz;
    	
        /* view space */
        src->cc.x.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.x.iv), vmmulf(src->wc.y.iv, mat->row1.x.iv)), vmmulf(src->wc.z.iv, mat->row2.x.iv));
        src->cc.y.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.y.iv), vmmulf(src->wc.y.iv, mat->row1.y.iv)), vmmulf(src->wc.z.iv, mat->row2.y.iv));
        src->cc.z.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.z.iv), vmmulf(src->wc.y.iv, mat->row1.z.iv)), vmmulf(src->wc.z.iv, mat->row2.z.iv));

        /* parallel projection + viewport transform */
        ooz.iv = divf(fov.iv, vmaddf(camdist.iv, src->cc.z.iv)); 
       
        src->vpx = ftoi(vmmulf(vmmulf(src->cc.x.iv, ooz.iv), vpw.iv)) + 320;
        src->vpy = ftoi(vmmulf(vmmulf(src->cc.y.iv, ooz.iv), vph.iv)) + 240 + (page.y);

        src++;
    }
}

void draw3d()
{
    struct POINT p2;
    int i;

      bb.srcform = NULL;
      bb.rule = bbSxorD;
      bb.halftoneform = NULL;
      bb.destrect.w = 1;
      bb.destrect.h = 1;
      
    for (i = 0; i < lcount; i++)
    {
        bb.destrect.x = vertices[lines[i][0]].vpx;
        bb.destrect.y = vertices[lines[i][0]].vpy;

        p2.x = vertices[lines[i][1]].vpx;
        p2.y = vertices[lines[i][1]].vpy;

        PaintLine(&bb, &p2);
    }
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
    float angle;

    /* map FPU into our memory space */
	mapped_fpu = phys(2);
	if (mapped_fpu == NULL)
		exit(2);

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    screen = InitGraphics(FALSE);
    font = FontOpen("/fonts/PellucidaSans-Serif36.font");
   fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
      font->maps->fixed,font->maps->maxw, 
      font->maps->line, font->maps->baseline);

    ESetSignal();
    page.x = page.y = 0;
    pindex = 0;
    frametime = EGetTime() + 500;
    framenum = 0;
#ifdef PAGE_FLIP
    printf("starting alarm\n");
    ESetAlarm(frametime);
#endif

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

    /* copy page0 to page1 */
    bb.srcform = screen;
    bb.halftoneform = NULL;
    bb.destrect.h = 480;
 	bb.rule = bbS;
	BitBlt(&bb);


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

   /* hide cursor */
   CursorVisible(FALSE);

   init3d();

   /* animate it */
   angle = 0.5;
   transform3d(&ltm, angle);
   draw3d();  
   while(1)
   {
#if 0
     restoreunder(&thesprite);
/*     restoreunder(&thesprite2);  */

     movesprite(&thesprite);
/*     movesprite(&thesprite2);  */

     saveunder(&thesprite);
/*     saveunder(&thesprite2);   */

     drawsprite(&thesprite);
/*     drawsprite(&thesprite2);   */
#endif

     draw3d();  
     angle += 0.05;
     transform3d(&ltm, angle); 
     draw3d();  

#ifdef PAGE_FLIP
     /* show draw time */
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
