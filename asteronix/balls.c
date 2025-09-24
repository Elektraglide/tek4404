#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>
#include <errno.h>

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
	&BlackMask,
	&BlackMask,
};
struct BBCOM bb;
short pindex;
short scrolldir = 1;
struct POINT page;
unsigned long frametime;
unsigned int framenum;

#define MAXROCKS 30
short numbigrocks;
sprite bigrocks[MAXROCKS];
short nummediumrocks;
sprite mediumrocks[MAXROCKS];
char status[16];

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

void blitfast(splash, halftone)
struct FORM *splash;
struct FORM *halftone;
{
    register unsigned long *dst, *src;
    register unsigned short *mask;
    register short h = splash->h;

    src = splash->addr;
    mask = halftone->addr;

    /* draw on current page */
    dst = ((char*)screen->addr) + (page.y << 7);
    do
    {
        /* mask 640px scanline */
        register unsigned long amask = mask[h & 15];
        amask |= amask<<16;

        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
		dst += 12;
    } while (--h > 0);
}

void restoreunder(asprite, w,h)
sprite *asprite;
int w,h;
{

    bb.srcform = NULL;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
	bb.destrect.x = asprite->oldx[pindex];
    bb.destrect.y = asprite->oldy[pindex];
	bb.destrect.w = w;
    bb.destrect.h = h;
 	bb.rule = bbnS;
    bb.halftoneform = NULL;
	BitBlt(&bb);
}

void blitclear(sx, sy, sh)
int sx, sy, sh;
{
    register unsigned long* dst;
    register short h = sh;

    sy += page.y;
    dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
    do
    {
        dst[0] = dst[1] = 0;
        dst += 32;
    } while (--h > 0);
}

void bigblitmasked(sx, sy, shift)
int sx, sy;
struct FORM **shift;
{
    register unsigned long* dst, * src, *mask;
    register short h = bigshift[0]->h;

    src = shift[sx & 15]->addr;
    mask = big_mshift[sx & 15]->addr;

    if (sx < 0 || sx > 640 || sy < 0 || sy > 480)
    {
    	printf("error %d %d\n", sx, sy);
    	return;
    }

    sy += page.y;
    dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);

    do
    {
        dst[0] = (dst[0] & *mask++) | *src++;
        dst[1] = (dst[1] & *mask++) | *src++;
        dst += 32;
    } while (--h > 0);
}

void bigblitfast(sx, sy)
int sx, sy;
{
    register unsigned long* dst, * src;
    register short h = bigshift[0]->h;

    src = bigshift[sx & 15]->addr;

    if (sx < 0 || sx > 640 || sy < 0 || sy > 480)
    {
    	printf("error %d %d\n", sx, sy);
    	return;
    }

    sy += page.y;
    dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
    do
    {
        dst[0] = *src++;
        dst[1] = *src++;
        dst += 32;
    } while (--h > 0);

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
	int sndfd;
	char sndbuffer[8];
	
    numbigrocks = 20;
	if (argc > 1)
		numbigrocks = atoi(argv[1]);
    nummediumrocks = numbigrocks / 2;

	sndfd = open("/dev/sound", O_WRONLY);
	

    
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
    bb.destrect.h = 512;

	/* build forms from embedded data */
	makemeteorforms();
	makeshifted(&big, bigshift, bbnS);
	makeshifted(&big_m, big_mshift, bbnS);
	makeshifted(&medium, mediumshift, bbnS);
	makeshifted(&medium_m, medium_mshift, bbnS);

	/* mask sprites; why needed? */
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
#if 1
 	   bb.srcform = medium_mshift[i];		
	   bb.destform = mediumshift[i];
       bb.destrect.x = 0;
       bb.destrect.y = 0;
       bb.destrect.w = mediumshift[i]->w;
       bb.destrect.h = mediumshift[i]->h;
       bb.rule = bbSnandD;
       BitBlt(&bb);
#endif
	}

	/* splash screen fade up */
	splash = readtga("splash.tga",&w,&h);
	if (splash)
	{
	  int loop,x,y;

      for(i=0; i<7; i++)
      {
        blitfast(splash, shading[i]);
        waitframes(4);
        waitflip();
      }
      waitframes(20);

	  /* robotron */
      loop = 4096;
      while (--loop)
      {
      	short off = loop & 63;
      	short id;

		  blitclear(0,0,448);
		  blitclear(640-64,0,448);

		  /* animate border with meteors */      	
          for (x = 0; x < 640-64; x += 64)
          {
              bigblitfast(x + (off), 0);
              bigblitfast(x + (64-off), 384);
          }
          for (y = 0; y < 384; y += 64)
          {
              bigblitmasked(0, y + (64-off), bigshift);
              bigblitmasked(640-64, y + (off), bigshift);
          }

		  /*  ping 1 of 30 of them (9 + 6 + 9 + 6) */
		  id = (-loop) & 31;
		  if (id < 30)
		  {
		  if (id < 9)
		  {
		  	x = id * 64 + off;
		  	y = 0;
		  }
		  else
		  if (id < 15)
		  {
		  	x = 640 - 64;
		  	y = (id - 9) * 64 + off;
		  }
		  else
		  if (id < 24)
		  {
		  	x = (8 - (id - 15)) * 64 + (64-off);
		  	y = 384;	
		  }
		  else
		  if (id < 30)
		  {
		  	x = 0;	
		  	y = (5 - (id - 24)) * 64 + (64-off);
		  }

   	       bb.srcform = &big_m;
           bb.destform = screen;
           bb.destrect.x = x;
           bb.destrect.y = y + page.y;
           bb.destrect.w = big.w;
           bb.destrect.h = big.h;
           bb.rule = bbSorD;
           BitBlt(&bb);
          }
		  
          waitflip();
      }

    }
    waitflip();


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
printf("waiting\n");	
	waitframes(90);
printf("GO\n");

    /* splash screen cleanup */
	if (splash)
	{
	  FormDestroy(splash);
  	  splash = NULL;
      bb.srcform = NULL;  	  
    }

    ClearScreen();

	/* make a sprite to use */
    for (i=0; i<MAXROCKS; i++)
        makesprite(&mediumrocks[i]);
    for(i=0; i<MAXROCKS; i++)
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
        restoreunder(rockptr++, 64, 48);
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++)
        restoreunder(rockptr++, 48, 32);

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

   }


}
