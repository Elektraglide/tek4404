#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>
#include <errno.h>

#include "asteronix.h"

#ifndef unix
#info Asteronix
#info Version 1.0
#endif

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

#define MAXROCKS 40
short numbigrocks;
sprite bigrocks[MAXROCKS];
short nummediumrocks;
sprite mediumrocks[MAXROCKS];

#define MAXBULLETS 32
short numbullets;
bullet bullets[MAXBULLETS];

char status[64];
char keyinput[16];

#define ATTRACTFLASH 7

/* ship controls */
#define LEFT    1
#define RIGHT   2
#define THRUST  4
#define FIRE    8
typedef struct
{
    fixed x,y,dx,dy;
    short angle;
    short oldx[2], oldy[2];
} ship;

ship aship;
int respawndelay;

#define NUM_ANGLES 32
#define ANGLE_MASK (NUM_ANGLES-1)
struct POINT shipshape[NUM_ANGLES][4];

/************* SOUND ************/

int sndfd;
unsigned char sb[32];

noise(f,v,l)
int f,v,l;
{
	sb[0] = 1;
	sb[1] = 0x80 | 0x60 | 0x04 | (f & 3);
	sb[2] = 0;
	
    sb[3] = 1;
    sb[4] = 0x80 | 0x70 | (v & 15);
    sb[5] = l;
	write(sndfd, sb, 6);
}

voice1(f,v,l)
int f,v,l;
{
	sb[0] = 2;
	sb[1] = 0x80 | 0x20 | (f & 15);
	sb[2] = (f >> 4) & 63;
	sb[3] = 0;

	sb[4] = 1;
	sb[5] = 0x80 | 0x30 | (v & 15);
	sb[6] = l;
	write(sndfd, sb, 7);
}

voice2(f,v,l)
int f,v,l;
{
	sb[0] = 2;
	sb[1] = 0x80 | 0x40 | (f & 15);
	sb[2] = (f >> 4) & 63;
	sb[3] = 0;

	sb[4] = 1;
	sb[5] = 0x80 | 0x50 | (v & 15);
	sb[6] = l;
	write(sndfd, sb, 7);
}

testsound(f)
int f;
{
  int v;

  voice1(f,8,0);	
  voice2(f/2,8,0);

  for(v=4; v<16; v++)
  {
    voice1(f,v,2);
    voice1(f/2,v,2);
  }
}

boomsound()
{
    /* only way to cancel plating sound!! */
	close(sndfd);
	sndfd = open("/dev/sound", O_RDONLY);

 	noise(2, 15,0);
	noise(2, 0, 1);
	noise(2, 4, 2);
	noise(2, 6, 2);
	noise(2, 8, 2);
	noise(2, 10, 2);
	noise(2, 12, 4);
 	noise(2, 15,0);
}

/************** GRAPHICS **********/

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

int newrnd;
int myrand()
{
    newrnd ^= (newrnd << 13);
    newrnd ^= (newrnd >> 17);
    newrnd ^= (newrnd << 5);
    return newrnd;	
}

int rand2()
{
  int v = myrand() & 7;
  if (v > 4) v -= 5;

  return v - 2;
}

struct POINT shipmodel[4] = {
    { 0, 8}
    {-8,-8},
    { 0,-4},
    { 8,-8},
};

void initship()
{
    int i, j;

    for (i = 0; i < NUM_ANGLES; i++)
    {
        float c = cos((i / (float)NUM_ANGLES) * 2.0 * 3.1415926);
        float s = sin((i / (float)NUM_ANGLES) * 2.0 * 3.1415926);
        float s2 = -s;

        for (j = 0; j < 4; j++)
        {
            shipshape[i][j].x = (short)(shipmodel[j].x * c + shipmodel[j].y * s);
            shipshape[i][j].y = (short)(shipmodel[j].x * s2 + shipmodel[j].y * c);
        }
    }
}

void newship()
{
    aship.x = INT2FIX(320);
    aship.y = INT2FIX(240);
    aship.dx = 0;
    aship.dy = 0;
    aship.angle = NUM_ANGLES / 2;   /* display origin top left, so angle 0 pointing down.. */
}

void doship()
{
    register struct POINT* shapeptr;
    register short sx, sy;
    struct POINT p2;

    /* restore previous */
    bb.srcform = NULL;
    bb.srcpoint.x = 0;
    bb.srcpoint.y = 0;
    bb.destform = screen;
    bb.destrect.x = aship.oldx[pindex] - 12;
    bb.destrect.y = aship.oldy[pindex] - 12;
    bb.destrect.w = 24;
    bb.destrect.h = 24;
    bb.rule = bbnS;
    bb.halftoneform = NULL;
    BitBlt(&bb);

    /* attenuate velocity by 250.0/256.0 (0.976) */
    aship.dx = (aship.dx * 250) >> 8;
    aship.dy = (aship.dy * 250) >> 8;

    /* move */
    aship.x += aship.dx;
    if (aship.x < 0)
    {
        aship.x += INT2FIX(640);
    }
    else
    if (aship.x > INT2FIX(640))
    {
        aship.x -= INT2FIX(640);
    }
    aship.y += aship.dy;
    if (aship.y < 0)
    {
        aship.y += INT2FIX(480);
    }
    else
    if (aship.y > INT2FIX(480))
    {
        aship.y -= INT2FIX(480);
    }

    /* origin */
    sx = FIX2INT(aship.x);
    sy = FIX2INT(aship.y);
    sy += page.y;

    shapeptr = shipshape[aship.angle & ANGLE_MASK];
    bb.destrect.x = sx + shapeptr->x;
	bb.destrect.y = sy + shapeptr->y;
    shapeptr++;
    p2.x = sx + shapeptr->x;
    p2.y = sy + shapeptr->y;
    LineDraw(&bb.destrect, &p2, 1, 0, &bb);

    shapeptr++;
    bb.destrect.x = sx + shapeptr->x;
    bb.destrect.y = sy + shapeptr->y;
    LineDraw(&bb.destrect, &p2, 1, 0, &bb);

    shapeptr++;
    p2.x = sx + shapeptr->x;
    p2.y = sy + shapeptr->y;
    LineDraw(&bb.destrect, &p2, 1, 0, &bb);

    shapeptr = shipshape[aship.angle & ANGLE_MASK];
    bb.destrect.x = sx + shapeptr->x;
    bb.destrect.y = sy + shapeptr->y;
    LineDraw(&bb.destrect, &p2, 1, 0, &bb);

    aship.oldx[pindex] = sx;
    aship.oldy[pindex] = sy;
}

void makesprite(asprite, x,y)
sprite* asprite;
int x,y;
{
    int v;

    asprite->x = INT2FIX(x);
    asprite->y = INT2FIX(y);
    asprite->oldx[0] = asprite->oldx[1] = -1000;

    /* ensure never zero */
    asprite->dx = INT2FIX(1 + (myrand() & 31)) >> 3;
    asprite->dy = INT2FIX(1 + (myrand() & 31)) >> 3;
    v = myrand();
    if (v & 1)
        asprite->dx = -asprite->dx;
    if (v & 2)
        asprite->dy = -asprite->dy;

    /* immortal for 20 frames */
    asprite->state = ALIVE + 20;
}

void makefastsprite(asprite, parent, angle)
sprite* asprite;
sprite* parent;
int angle;
{
    asprite->x = parent->x;
    asprite->y = parent->y;
    asprite->oldx[0] = asprite->oldx[1] = -1000;

    /* ricochet angles */
    if (angle == 0)
    {
        asprite->dx = (parent->dx + parent->dy);
        asprite->dy = (parent->dy + parent->dx);
    }
    else
    {
        asprite->dx = (parent->dx - parent->dy);
        asprite->dy = (parent->dy - parent->dx);
    }

    /* immortal for 20 frames */
    asprite->state = ALIVE + 20;
}

int hit(rockptr,w,h)
register sprite *rockptr;
int w,h;
{
register bullet *bulletptr; 
register short i;

  /* dying cannot be hit again */
  if (rockptr->state != ALIVE)
    return 0;

  bulletptr = bullets;  
  for(i=0; i<numbullets; i++,bulletptr++)
  {
    register short dx;

    if (bulletptr->spr.state >= ALIVE)
    {      
      dx = FIX2INT(bulletptr->spr.x - rockptr->x);
      if ( (dx > (0)) && (dx < (w)) )
      {
        dx = FIX2INT(bulletptr->spr.y - rockptr->y);
        if ( (dx > (0)) && (dx < (h)) )
        {
          /* start dying */
          bulletptr->spr.state = DYING2;
          return 1;
        }
      }
    }
  }
  return 0;
}

fixed d360[] = {
   INT2FIX(-1),INT2FIX(0),	
   INT2FIX(1),INT2FIX(0),	
   INT2FIX(0),INT2FIX(-1),	
   INT2FIX(0),INT2FIX(1),	
   INT2FIX(-2),INT2FIX(-1),	
   INT2FIX(-1),INT2FIX(1),	
};

void makeboom(sx,sy)
int sx,sy;
{
int i, *dir;

    boomsound();

  dir = d360;
  for(i=0; i<6; i++)
  {
    if (numbullets < MAXBULLETS)
    {
		bullets[numbullets].spr.x = INT2FIX(sx);
		bullets[numbullets].spr.y = INT2FIX(sy);
        bullets[numbullets].spr.dx = *dir++;
        bullets[numbullets].spr.dy = *dir++;
	    bullets[numbullets].spr.oldx[0] =
	    bullets[numbullets].spr.oldx[1] = -1;  /* mark as new */
		bullets[numbullets].spr.state = ALIVE + 25;
 	    numbullets++;
 	}
  }
}

void dobullets()
{
  register bullet *bulletptr;
  int i;
  
  bulletptr = bullets;
  for(i=0; i<numbullets; i++, bulletptr++)
  {
    register unsigned short* dst;
    short sx;
    short sy;

	/* undraw */
	if (bulletptr->spr.oldx[pindex] != -1)
    {
      sx = bulletptr->spr.oldx[pindex];
      sy = bulletptr->spr.oldy[pindex];
      
      dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
      dst[0] &= ~(0x8000 >> (sx & 15));
    }
    
	/* move */
    bulletptr->spr.x += bulletptr->spr.dx;
    if (bulletptr->spr.x < INT2FIX(0)) bulletptr->spr.x += INT2FIX(640);
    if (bulletptr->spr.x >INT2FIX(640)) bulletptr->spr.x -= INT2FIX(640);
    bulletptr->spr.y += bulletptr->spr.dy;
    if (bulletptr->spr.y < INT2FIX(0)) bulletptr->spr.y += INT2FIX(480);
    if (bulletptr->spr.y >INT2FIX(480)) bulletptr->spr.y -= INT2FIX(480);

    bulletptr->spr.state--;

    /* draw */
	if (bulletptr->spr.state >= ALIVE)
	{
      sx = FIX2INT(bulletptr->spr.x);
      sy = FIX2INT(bulletptr->spr.y);

      sy += page.y;
      dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
      dst[0] |= 0x8000 >> (sx & 15);
      
      bulletptr->spr.oldx[pindex] = sx;
      bulletptr->spr.oldy[pindex] = sy;
    }
    else
    if (bulletptr->spr.state == DEAD)
    {
      numbullets--;
      *bulletptr-- = bullets[numbullets];
      i--;
    }
  }	
}

dorocks()
{
register sprite *rockptr;
short i;
	
    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++,rockptr++)
	{
      restoreunder(rockptr, 64, 41);

      /* has it hit a bullet? */
      if (hit(rockptr, 48,41))
      {
        rockptr->state = DYING1;

  /* STOP */        

        /* 2 smaller fragments */
        if (nummediumrocks < MAXROCKS-2)
        {
          makefastsprite(&mediumrocks[nummediumrocks++], rockptr, 0);
          makefastsprite(&mediumrocks[nummediumrocks++], rockptr, 1);
        }
      }
    }
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++,rockptr++)
    {
      restoreunder(rockptr, 48, 22);

      /* has it hit a bullet? */
      if (hit(rockptr, 32,22))
      {
        rockptr->state = DYING2;

        makeboom(rockptr->oldx[pindex], rockptr->oldy[pindex]);
      }
    }

    /* move */
    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++)
        movesprite(rockptr++);
    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++)
        movesprite(rockptr++);

    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;

    /* draw */
    rockptr = bigrocks;
	for(i=0; i<numbigrocks; i++,rockptr++)
	{
      if (rockptr->state >= ALIVE)
      {
        /* count down immortal state */
        if (rockptr->state > ALIVE) rockptr->state--;
        bigdrawfast(rockptr);
      }
      else
      {
        rockptr->state--;
        if (rockptr->state == DEAD)
        {
            numbigrocks--;
            *rockptr-- = bigrocks[numbigrocks];
            i--;
        }
      }
    }

    rockptr = mediumrocks;
    for (i = 0; i < nummediumrocks; i++,rockptr++)
    {
      if (rockptr->state >= ALIVE)
      {
          /* count down immortal state */
          if (rockptr->state > ALIVE) rockptr->state--;
          mediumdrawfast(rockptr);
      }
      else
      {
          rockptr->state--;
          if (rockptr->state == DEAD)
          {
              nummediumrocks--;
              *rockptr-- = mediumrocks[nummediumrocks];
              i--;
          }
      }
	}
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
    if (form)
    {
        /* this assumes width and stride match */
        len = (*w >> 3) * (*h);
        read(fd, form->addr, len);
    }
    else
    {
        fprintf(stderr, "FormCreate: %s\n", strerror(errno));
    }

    close(fd);
  }
  else
  {
  	fprintf(stderr, "FAILED TO OPEN\n");
  }

  return form;
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

void sh_int(sig)
int sig;
{
  struct POINT p;
  p.x = p.y = 0;
  SetViewport(&p);

  EventDisable();
  SetKBCode(1);

  ExitGraphics();
  FontClose(font);

  exit(2);
}

unsigned char* keyboardmap;
unsigned int oldwaiting[2];
main(argc,argv)
int argc;
char *argv[];
{
    register short i,j;
    fixed x,y,dx,dy;
    struct FORM *splash;
    struct POINT origin,p2;
    struct RECT r;
    unsigned int waiting;
	int w,h;

	newrnd = time(NULL);
	
	numbullets = 0;	
    nummediumrocks = 0;
    numbigrocks = 20;
	if (argc > 1)
		numbigrocks = atoi(argv[1]);


	sndfd = open("/dev/sound", O_WRONLY);

	/* tempo 0.1Hz */
	sb[0] = 0;
	sb[1] = 0;
	sb[2] = 6;
    write(sndfd, sb, 3);

/*	testsound(600);  */
		
    signal(SIGINT, sh_int);
    signal(SIGQUIT, sh_int);
    signal(SIGTERM, sh_int);
    signal(SIGMILLI, sh_timer);

    font = FontOpen("/fonts/MagnoliaFixed7.font");
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

    EventEnable();
    SetKBCode(0);

    /* hide cursor */
    CursorVisible(FALSE);

    /* get reference to keymap */
    keyboardmap = getkeymap();

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

 	   bb.srcform = medium_mshift[i];
	   bb.destform = mediumshift[i];
       bb.destrect.x = 0;
       bb.destrect.y = 0;
       bb.destrect.w = mediumshift[i]->w;
       bb.destrect.h = mediumshift[i]->h;
       bb.rule = bbSnandD;
       BitBlt(&bb);
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
        short off = (loop<<1) & 63;
        short id;

          /* wait for a key */
          if (readkeyboardevents(keyinput, 8))
            break;

          blitclear64(0,0,440);
          blitclear64(640-64,0,440);

          /* animate border with meteors */
          id = (loop>>1);
          for (x = 0; x < 640 - 64; x += 64)
          {
          	bigblitfast(x + (off), 0);
              if ((++id & ATTRACTFLASH) == 0) 
                bigblitflash(x + (off), 0);
          }

          /* doing this out of order to avoid masking problems! */
          id += 8;
          for (x = 512; x >= 0; x -= 64)
          {
            bigblitfast(x + (64 - off), 384);
              if ((++id & ATTRACTFLASH) == 0) 
                bigblitflash(x + (64 - off), 384);
          }
          id -= 8;
          
          for (y = 0; y < 384; y += 64)
          {
          	 bigblitmasked(640 - 64, y + (off), bigshift);
              if ((++id & ATTRACTFLASH) == 0)
                bigblitflash(640 - 64, y + (off));
          }
          for (y = 320; y >= 0; y -= 64)
          {
          	bigblitmasked(0, y + (64 - off), bigshift);
              if ((++id & ATTRACTFLASH) == 0) 
                bigblitflash(0, y + (64 - off));
          }

		  
          waitflip();
      }

    }
    else
    {
        sh_int(1);
    }
    waitflip();


#if 0
ClearScreen();

bb.srcpoint.x = 0;
bb.srcpoint.y = 0;
bb.destform = screen;
bb.destrect.w = bigshift[0]->w;
bb.destrect.h = bigshift[0]->h;
for(i=0; i<8; i++)
{
      bb.srcform = bigshift[i];
      bb.destrect.x = i * 64;
      bb.destrect.y = page.y + 128;
   	  bb.rule = bbS;
	  BitBlt(&bb);
      bb.srcform = bigshift[8+i];
      bb.destrect.x = i * 64;
      bb.destrect.y = page.y + 128 + 64;
   	  bb.rule = bbS;
	  BitBlt(&bb);

     bb.srcform = &BlackMask;
   	 bb.rule = bbS;
     r.x = bb.destrect.x;
     r.y = page.y + 128;
     r.w = 64;
     r.h = 128;
     RectBoxDrawX(&r,1, &bb);

      bb.srcform = big_mshift[i];
      bb.destrect.x = i * 64;
      bb.destrect.y = page.y + 256;
   	  bb.rule = bbS;
	  BitBlt(&bb);
      bb.srcform = big_mshift[8+i];
      bb.destrect.x = i * 64;
      bb.destrect.y = page.y + 256 + 64;
   	  bb.rule = bbS;
	  BitBlt(&bb);

     bb.srcform = &WhiteMask;
   	 bb.rule = bbS;
     r.x = bb.destrect.x;
     r.y = page.y + 256;
     r.w = 64;
     r.h = 128;
     RectBoxDrawX(&r,1, &bb);

}
waitflip();
	waitframes(900);
#endif

	waitframes(90);


    /* splash screen cleanup */
	if (splash)
	{
	  FormDestroy(splash);
  	  splash = NULL;
      bb.srcform = NULL;
    }

    ClearScreen();

	/* make sprites to use */
    for(i=0; i<MAXROCKS; i++)
	    makesprite(&bigrocks[i], myrand() % 640, myrand() % 480);

    initship();
    respawndelay = framenum + 60;

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

   /* keep going while things exist */
   while(numbigrocks + nummediumrocks > 0)
   {
   	register sprite *rockptr;
    short keys;

    readkeyboardevents(keyinput, 8);
    if (keyinput[0] == 0x03)     /* Ctrl-C */
    {
        sh_int(2);
    }

    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;
    bb.cliprect.x = 0;
    bb.cliprect.y = page.y;
    bb.cliprect.w = 1024;
    bb.cliprect.h = 512;

    if (respawndelay)
    {
        if (respawndelay < framenum)
        {
            newship();
            respawndelay = 0;
        }
    }
    else
    {
        doship();

        keys = 0;
        if (keyboardmap[0x61]) keys |= LEFT;    /* A */
        if (keyboardmap[0x64]) keys |= RIGHT;   /* D */
        if (keyboardmap[0x77]) keys |= THRUST;  /* W */
        if (keyboardmap[0x73]) keys |= FIRE;    /* S */

        if (keys & LEFT)
            aship.angle += 1;
        if (keys & RIGHT)
            aship.angle -= 1;

        if (keys & THRUST)
        {
            aship.dx += INT2FIX(shipshape[aship.angle & ANGLE_MASK][0].x) >> 4;
            aship.dy += INT2FIX(shipshape[aship.angle & ANGLE_MASK][0].y) >> 4;
        }

        if (numbullets < MAXBULLETS && (keys & FIRE) && ((framenum & 7) == 0))
        {
            bullets[numbullets].spr.x = aship.x + shipshape[aship.angle & ANGLE_MASK][0].x;
            bullets[numbullets].spr.y = aship.y + shipshape[aship.angle & ANGLE_MASK][0].y;

            bullets[numbullets].spr.dx = INT2FIX(shipshape[aship.angle & ANGLE_MASK][0].x);
            bullets[numbullets].spr.dy = INT2FIX(shipshape[aship.angle & ANGLE_MASK][0].y);

            bullets[numbullets].spr.oldx[0] =
                bullets[numbullets].spr.oldx[1] = -1;  /* mark as new */

            bullets[numbullets].spr.state = ALIVE + 60;

            numbullets++;
        }
    }

    dorocks();
    
    dobullets();

	if ((framenum & 15) == 0)
	{
      sprintf(status, "bullets:%d big:%d med:%d   ", numbullets, numbigrocks,nummediumrocks);
	}

     bb.srcform = NULL;
     bb.rule = bbS;
     bb.destrect.x = 0;
     bb.destrect.y = 0;
     bb.destrect.w = 1024;
     bb.destrect.h = 1024;
     origin.x = 192;
     origin.y = page.y + font->maps->line;
     StringDrawX(status, &origin, &bb, font);

#if 0
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
#endif

	 waiting = waitflip();

   }

   sh_int(0);

}
