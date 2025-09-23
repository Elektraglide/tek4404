#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>
#include <errno.h>

#define PAGE_FLIP
#define SPRITES

#ifndef unix
#info Asteronix
#info Version 1.0
#endif

#define INT2FIX(A)  ((A)<<8)
#define FIX2INT(A)  ((A)>>8)

typedef long fixed;

typedef struct
{
  fixed x,y;
  fixed dx,dy;

  short oldx[2],oldy[2];
} sprite;

typedef struct
{
  unsigned char identsize,cmaptype,imagetype;
  unsigned char cmapstart[2],cmaplen[2],cmapbits;
  unsigned char xstart[2],ystart[2],width[2],height[2];
  unsigned char bits,desc;	
} tgaheader;

struct FontHeader *font;
struct FORM *screen;
struct FORM *shading[] = {
	&WhiteMask, 
	&VeryLightGrayMask, 
	&LightGrayMask,
	&GrayMask,
	&DarkGrayMask,
	&BlackMask
};
struct BBCOM bb;
short pindex;
short scrolldir = 1;
struct POINT page;
unsigned long frametime;
unsigned int framenum;


short numbigrocks;
sprite bigrocks[30];
short nummediumrocks;
sprite mediumrocks[30];
char status[16];


struct FORM *form;
struct FORM *formshift[16];

extern makemeteorforms();
extern struct FORM big,big_m;
extern struct FORM medium,medium_m;
struct FORM *bigshift[16];
struct FORM *big_mshift[16];
struct FORM *mediumshift[16];
struct FORM *medium_mshift[16];

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

int waitflip()
{
  unsigned int currframe;
  short waiting;
  
   /* wait for frame flip */
   waiting = 0;
   currframe = framenum;
   while (currframe == framenum)
       waiting++;
   currframe = framenum;

   flip();

   /* ensure 1 vblank has happened */
   while (currframe == framenum);

  return((int)waiting);
}

int waitframes(n)
int n;
{
  unsigned int currframe;
  short waiting;
      
   /* wait for frame flip */
   waiting = 0;
   currframe = framenum + n;
   while (currframe != framenum)
       waiting++;
}


void sh_timer(sig)
int sig;
{

  frametime += 33;
  framenum++;

  signal(sig, sh_timer);
  ESetAlarm(frametime);
}

void screenfast(splash, mask)
struct FORM *splash;
struct FORM* halftone;
{
    register unsigned long *dst, *src, *mask;
    register short h = splash->h;

    src = splash->addr;
    mask = halftone->addr;

    /* draw on current page */
    dst = ((char*)screen->addr) + (page.y << 7);
    do
    {
        /* mask 1 128byte scanline */
        register amask = mask[h & 15];
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
    } while (--h > 0);
}

void restoreunder(asprite)
sprite *asprite;
{

    bb.srcform = NULL;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = asprite->oldx[pindex];
    bb.destrect.y = asprite->oldy[pindex];
	bb.destrect.w = formshift[0]->w;
    bb.destrect.h = formshift[0]->h;
 	bb.rule = bbnS;
    bb.halftoneform = NULL;
	BitBlt(&bb);
}

/* draw 64 wide sprite */
void bigdrawfast(asprite)
sprite *asprite;
{
	register unsigned long *dst,*src,*mask;
	register short h = bigshift[0]->h;
	short sx = FIX2INT(asprite->x);
	short sy = FIX2INT(asprite->y);

	src = bigshift[sx & 15]->addr;
	mask = big_mshift[sx & 15]->addr;

	if (sy < 0) 		/* clip to top */
	{
		src -= sy << 1;
		mask -= sy << 1;
		h += sy;
		sy = 0;
	}
	else
	if (sy + h > 480)	/* clip to bottom */
	{
		h = 480 - sy;
	}
	
	/* draw on current page */
	sy += page.y;

    dst = ((char *)screen->addr) + (sy<<7) + ((sx & -16) >> 3);
    do
    {
      dst[0] = (dst[0] & *mask++) | *src++;
      dst[1] = (dst[1] & *mask++) | *src++;
      dst += 32;
    } while (--h > 0);

    asprite->oldx[pindex] = sx & -16;
    asprite->oldy[pindex] = sy;
}

/* draw 48 wide sprite */
void mediumdrawfast(asprite)
sprite* asprite;
{
	register unsigned short *dst, *src, *mask;
	register short h = mediumshift[0]->h;
	short sx = FIX2INT(asprite->x);
	short sy = FIX2INT(asprite->y);

	src = mediumshift[sx & 15]->addr;
	mask = medium_mshift[sx & 15]->addr;

	if (sy < 0) 		/* clip to top */
	{
		src -= sy + (sy << 1);
		mask -= sy + (sy << 1);
		h += sy;
		sy = 0;
	}
	else
		if (sy + h > 480)	/* clip to bottom */
		{
			h = 480 - sy;
		}

	/* draw on current page */
	sy += page.y;

	dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
	do
	{
		dst[0] = (dst[0] & *mask++) | *src++;
        dst[1] = (dst[1] & *mask++) | *src++;
        dst[2] = (dst[2] & *mask++) | *src++;
        dst += 64;
	} while (--h > 0);

	asprite->oldx[pindex] = sx & -16;
	asprite->oldy[pindex] = sy;
}

void drawsprite(asprite)
sprite *asprite;
{
    short sx = FIX2INT(asprite->x);
    short sy = FIX2INT(asprite->y);
	short h = bigshift[0]->h;

	/* clip to bottom */
	if (sy + h > 480)
		h = 480 - sy;

	/* draw on current page */
	sy += page.y;
		
    /* mask out */
    bb.srcform = big_mshift[sx & 15]; 
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = sx & -16;
    bb.destrect.y = sy;
	bb.destrect.w = bigshift[0]->w;
    bb.destrect.h = h;
 	bb.rule = bbSandD;
    bb.halftoneform = NULL;
	BitBlt(&bb); 

    /* draw over */
    bb.srcform = bigshift[sx & 15]; 
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = sx & -16;
    bb.destrect.y = sy;
	bb.destrect.w = bigshift[0]->w;
    bb.destrect.h = h;
 	bb.rule = bbSorD;
    bb.halftoneform = NULL;
 	BitBlt(&bb); 

    asprite->oldx[pindex] = sx & -16;
    asprite->oldy[pindex] = sy;
}

struct FORM *readtga(filename, w,h)
char *filename;
int *w,*h;
{
  int fd,len;
  tgaheader header;
  struct FORM *form = NULL;
    
  fd = open(filename, O_RDONLY);
  if (fd > 0)
  {
	read(fd, &header, sizeof(header));

	*w = header.width[0] + header.width[1] * 256;
	*h = header.height[0] + header.height[1] * 256;
	form = FormCreate(*w,*h);

	/* this assumes width and stride match */
  	len = (*w >> 3) * (*h);
	read(fd, form->addr, len);  	
    
    close(fd);
  }
  else
  {
  	fprintf(stderr, "FAILED TO OPEN\n");
  }

  return form;
}


struct FORM *makeform()
{
  struct POINT origin,p1,p2;
  struct RECT r;
  struct FORM *form;
  short i;
     
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
     bb.halftoneform = NULL;
     bb.cliprect.x = 4;
     bb.cliprect.y = 4;
     bb.cliprect.w = form->w - 10;
     bb.cliprect.h = form->h - 8;

     origin.x = 2;
     origin.y = font->maps->line;
     StringDrawX("Tektronix\n  4404", &origin, &bb, font);

	 /* frame around it */
     bb.rule = bbSorD;
     bb.halftoneform = &GrayMask;
     bb.cliprect.x = 0;
     bb.cliprect.y = 0;
     bb.cliprect.w = form->w;
     bb.cliprect.h = form->h;
     bb.srcform = NULL;
     r.x = r.y = 4;
     r.w = 60-8;
     r.h = 60-8;
     RectBoxDrawX(&r,4, &bb);

	 origin.x = 32;
	 origin.y = 32;
	 
	 CircleDrawX(&origin, 28, 2, &bb);
   }

   return form;
}

makeshifted(src,dstarr,rule)
struct FORM *src;
struct FORM **dstarr;
int rule;
{
	int i,len;
    struct RECT r;
    	
     bb.srcform = src;
     bb.srcpoint.x = 0;
     bb.srcpoint.y = 0;
     bb.cliprect.x = 0;
     bb.cliprect.y = 0;
     bb.cliprect.w = 1024;
     bb.cliprect.h = 1024;
     bb.halftoneform = NULL;
     bb.rule = rule;

	/* cache shifted versions of it */
	r.x = 0;
	r.y = 0;
	r.w = src->w + 16;
	r.h = src->h;
	len = (r.w >> 3) * r.h;
	for (i=0; i<16; i++)
	{
	   dstarr[i] = FormCreate(src->w + 16,src->h);

	   memset(dstarr[i]->addr, 0xff, len);
		
	   bb.destform = dstarr[i];
       bb.destrect.x = i;
       bb.destrect.y = 0;
       bb.destrect.w = src->w;
       bb.destrect.h = src->h;
       BitBlt(&bb);
	}
}


void movesprite(asprite)
sprite *asprite;
{
    asprite->x += asprite->dx;
    if (asprite->x < 0) 
    {
      asprite->x += INT2FIX(640);
    }
    else
    if (asprite->x > INT2FIX(640)) 
    {
      asprite->x -= INT2FIX(640);
    }

    asprite->y += asprite->dy;
    if (asprite->y < INT2FIX(0)) 
    {
      asprite->y += INT2FIX(480);
    }
    else
    if (asprite->y > INT2FIX(480)) 
    {
      asprite->y -= INT2FIX(480);
    }
}

void makesprite(asprite)
sprite* asprite;
{
    asprite->x = INT2FIX(128 + (rand() & 255));
    asprite->y = INT2FIX(64 + (rand() & 255));
    asprite->oldx[0] = asprite->oldx[1] = -1000;

    asprite->dx = INT2FIX((rand() & 15) - 8) >> 2;
    asprite->dy = INT2FIX((rand() & 15) - 8) >> 2;
    
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
    register short i;
    fixed x,y,dx,dy;
    struct FORM *splash;
    struct POINT origin,p2;
    struct RECT r;
    unsigned int waiting;
	int w,h;
	
    nummediumrocks = 5;
    numbigrocks = 10;
	if (argc > 1)
		numbigrocks = atoi(argv[1]);

    
    /* map FPU into our memory space */
	mapped_fpu = phys(2);
	if (mapped_fpu == NULL)
	{
		fprintf(stderr, "no FPU mapped\n");
		exit(2);
	}

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    font = FontOpen("/fonts/PellucidaSans-Serif10.font");
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
    ClearScreen();

    /* hide cursor */
    CursorVisible(FALSE);

    /* ESetSignal(); */
    page.x = 0;
    page.y = 0;
    pindex = 0;
    frametime = EGetTime() + 250;
    framenum = 0;
#ifdef PAGE_FLIP
    ESetAlarm(frametime);
#endif

    /* for controlling clipping etc */
    BbcomDefault(&bb);

    bb.srcform = NULL;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 512;

	/* splash screen fade up */
	splash = readtga("splash.tga",&w,&h);
	if (splash)
	{
	  bb.srcform = splash;
	  for(i=0; i<6; i++)
	  {	
        bb.destrect.x = 0;
        bb.destrect.y = page.y;
     	bb.rule = bbS;
        bb.halftoneform = shading[i];
		BitBlt(&bb);
		waitframes(8);		
	  }
    }

	/* make shape to draw */
   	form = makeform();
	makeshifted(form, formshift, bbS);

	/* build forms from embedded data */
	makemeteorforms();
	makeshifted(&big, bigshift, bbnS);
	makeshifted(&big_m, big_mshift, bbnS);
	makeshifted(&medium, mediumshift, bbnS);
	makeshifted(&medium_m, medium_mshift, bbnS);
	/* mask sprite */
	for (i=0; i<16; i++)
	{
 	   bb.srcform = big_mshift[i];		
	   bb.destform = bigshift[i];
       bb.destrect.x = 0;
       bb.destrect.y = 0;
       bb.destrect.w = bigshift[i]->w;
       bb.destrect.h = bigshift[i]->h;
       bb.rule = bbSnandD;
       BitBlt(&bb);
	}


#if 0

bb.srcpoint.x = 0;
bb.srcpoint.y = 0;
bb.destform = screen;
bb.destrect.w = bigshift[0]->w;
bb.destrect.h = bigshift[0]->h;
for(i=0; i<16; i++)
{
      bb.srcform = bigshift[i];
      bb.destrect.x = i * 80;
      bb.destrect.y = 128;
   	  bb.rule = bbS;
	  BitBlt(&bb);

      bb.srcform = big_mshift[i];
      bb.destrect.x = i * 80;
      bb.destrect.y = 192;
   	  bb.rule = bbS;
	  BitBlt(&bb);
}
#endif
	
	waitframes(90);

    /* splash screen fade down */
	if (splash)
	{
	  bb.srcform = splash;
	  bb.destform = screen;
      bb.destrect.w = 1024;
      bb.destrect.h = 512;
	  for(i=5; i>=0; i--)
	  {	
        bb.destrect.x = 0;
        bb.destrect.y = page.y;
     	bb.rule = bbS;
        bb.halftoneform = shading[i];
		BitBlt(&bb);
		waitframes(4);
		
		waitflip();
	  }

	  FormDestroy(splash);
  	  splash = NULL;
      bb.srcform = NULL;  	  
    }

    ClearScreen();

	/* make a sprite to use */
    for (i = 0; i < 30; i++)
        makesprite(&mediumrocks[i]);
    for(i=0; i<30; i++)
	    makesprite(&bigrocks[i]);

    /* set up the bitblt command stuff */
    bb.srcform = (struct FORM *)NULL;		/* no source */
    bb.destform = screen;			/* dest is screen */
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

   /* animate it */
   while(1)
   {
   	register sprite *rockptr;
   	
    bb.cliprect.x = 0;
    bb.cliprect.y = page.y;
    bb.cliprect.w = 1024;
    bb.cliprect.h = 512;

    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++)
        restoreunder(rockptr++);
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++)
        restoreunder(rockptr++);

    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++)
        movesprite(rockptr++);
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++)
        movesprite(rockptr++);

    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++)
        bigdrawfast(rockptr++);
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++)
        mediumdrawfast(rockptr++);

	if ((framenum & 63) == 0)
	{
     sprintf(status, "%d", framenum);
	}
	
     bb.srcform = NULL;
     bb.rule = bbS;
     bb.destrect.x = 0;
     bb.destrect.y = 0;
     bb.destrect.w = 1024;
     bb.destrect.h = 1024;
     origin.x = 640-50;
     origin.y = page.y + font->maps->line;
     StringDrawX(status, &origin, &bb, font);

#ifdef PAGE_FLIP
     /* show draw time by how long we waited for VBLANK */
     waiting >>= 4;
     bb.srcform = NULL;
     bb.rule = bbS;
     r.x = page.x;
     r.y = page.y;
     r.w = waiting;
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

	 waiting = waitflip();
#endif

   }


}
