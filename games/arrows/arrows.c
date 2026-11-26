#include <stdio.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

#info arrows
#info Version 1.0
#tstmp

struct DISPSTATE ds;
struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
unsigned long frametime;
unsigned int framenum;

waitbutton(msg)
char *msg;
{
  struct POINT origin;

     origin.x = 250;
     origin.y = 16;
     StringDraw(msg, &origin);

     while (GetButtons());
}


void sh_timer(sig)
int sig;
{

  frametime += 17;
  framenum++;

  signal(sig, sh_timer);
  ESetAlarm(frametime);
}


void sh_int(sig)
int sig;
{
  struct POINT p;
  p.x = p.y = 0;
  SetViewport(&p);
  ExitGraphics();
  FontClose(font);
  RestoreDisplayState(&ds);

  exit(2);
}

int waitframes(n)
int n;
{
    unsigned int currframe;
    short waiting;

    /* wait for frame flip */
    waiting = 0;
    currframe = framenum + n;
    while (currframe > framenum)
        waiting++;
}

/* set by args */
int BSIZE;

#define BX ((640 - BSIZE*8)/2)
#define BY ((450 - BSIZE*8)/2)

/* more external arrows => easier.. */
#define NUMEDGERS 20

typedef int packedxy;
#define PACKXY(X,Y) (Y*BSIZE+X)
#define UNPACKX(XY) (XY % BSIZE)
#define UNPACKY(XY) (XY / BSIZE)

struct FORM* corners[16];
struct FORM* straights[4];
struct FORM* tips[4];

unsigned short tipimg[][8] =
{
    {0xfe00,0xfe00,0x7c00,0x7c00,0x3800,0x3800,0x1000,0x0000},
    {0x0300,0x0f00,0x3f00,0x7f00,0x3f00,0x0f00,0x0300,0x0000},
    {0x0000,0x1000,0x3800,0x3800,0x7c00,0x7c00,0xfe00,0xfe00},
    {0xc000,0xf000,0xfc00,0xfe00,0xfc00,0xf000,0xc000,0x0000},
};

unsigned short straightimg[][8] =
{
    {0x3800,0x3800,0x3800,0x3800,0x3800,0x3800,0x3800,0x3800},
    {0x0000,0x0000,0xff00,0xff00,0xff00,0x0000,0x0000,0x0000},
    {0x3800,0x3800,0x3800,0x3800,0x3800,0x3800,0x3800,0x3800},
    {0x0000,0x0000,0xff00,0xff00,0xff00,0x0000,0x0000,0x0000},
};

unsigned short cornerimg[16][8] =
{
    {0,0,0,0,0,0,0,0},
    {0x3800,0x3800,0xf800,0xf800,0xf000,0x0000,0x0000,0x0000}, /* E -> N */
    {0,0,0,0,0,0,0,0},
    {0x3800,0x3800,0x3f00,0x3f00,0x1f00,0x0000,0x0000,0x0000}, /* W -> N */

    {0x0000,0x0000,0x1f00,0x3f00,0x3f00,0x3800,0x3800,0x3800}, /* N -> E */
    {0,0,0,0,0,0,0,0},
    {0x3800,0x3800,0x3f00,0x3f00,0x1f00,0x0000,0x0000,0x0000}, /* S -> E */
    {0,0,0,0,0,0,0,0},

    {0,0,0,0,0,0,0,0},
    {0x0000,0x0000,0xf000,0xf800,0xf800,0x3800,0x3800,0x3800}, /* E -> S */
    {0,0,0,0,0,0,0,0},
    {0x0000,0x0000,0x1f00,0x3f00,0x3f00,0x3800,0x3800,0x3800}, /* W -> S */

    {0x0000,0x0000,0xf000,0xf800,0xf800,0x3800,0x3800,0x3800}, /* N -> W */
    {0,0,0,0,0,0,0,0},
    {0x3800,0x3800,0xf800,0xf800,0xf000,0x0000,0x0000,0x0000}, /* S -> W */
    {0,0,0,0,0,0,0,0},
};

void
buildblocks()
{
    int i;

    for (i = 0; i < 4; i++)
    {
        straights[i] = FormCreate(8, 8);
        memcpy(straights[i]->addr, straightimg[i], 8 * 2);
    }

    for (i = 0; i < 4; i++)
    {
        tips[i] = FormCreate(8, 8);
        memcpy(tips[i]->addr, tipimg[i], 8 * 2);
    }

    for (i = 0; i < 16; i++)
    {
        corners[i] = FormCreate(8,8);
        memcpy(corners[i]->addr, cornerimg[i], 8*2);
    }
}

#define MAXBOARD 64

unsigned char board[MAXBOARD * MAXBOARD];
int deltamove[4][2] = { {0,-1},{1,0},{0,1},{-1,0} };

#define FIRST 0x80
#define LAST 0x40
#define CORNER 0x20

#define CL  255
#define N   0
#define E   1
#define S   2
#define W   3

#define ISCLEAR(X,Y) (board[Y*64+X] == CL)
#define ISUSED(X,Y) ((board[Y*64+X] & 0x10) != 0x10)

void drawcell(x, y)
short x, y;
{
    if (ISUSED(x,y))
    {
        if (board[y*64+x] & CORNER)
        {
            bb.srcform = corners[board[y * 64 + x] & 15];
        }
        else
        if (board[y * 64 + x] & FIRST)
        {
            bb.srcform = tips[board[y * 64 + x] & 3];
        }
        else
        {
            bb.srcform = straights[board[y * 64 + x] & 3];
        }

#if 0
        /* mark last */
        bb.halftoneform = (board[y * 64 + x] & LAST) ? &GrayMask : &BlackMask;
#endif

        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;
        bb.destrect.x = BX + x * 8;
        bb.destrect.y = BY + y * 8;
        bb.destrect.w = 8;
        bb.destrect.h = 8;
        BitBlt(&bb);
    }
}

void drawhline(x, y, l)
short x, y, l;
{
    short r;

    for (r = 0; r < l; r++)
    {
        drawcell(x+r, y);
    }
}

void drawvline(x, y, l)
short x, y, l;
{
    short r;

    for (r = 0; r < l; r++)
    {
        drawcell(x, y+r);
    }
}

void drawlevel()
{
    short x, y, r;

    x = (BSIZE / 2);
    y = (BSIZE / 2);
    for (r = 1; r < BSIZE / 2; r++)
    {
        drawhline(x - 1 - r, y - r, r + r + 1);
        drawhline(x - 1 - r, y + r, r + r + 2);

        drawvline(x - 1 - r, y - 1 - r, r + r + 1);
        drawvline(x + r, y - 1 - r, r + r + 2);
    }

    for (y = 0; y < BSIZE; y++)
    {
        for (x = 0; x < BSIZE; x++)
        {
            drawcell(x, y);
        }
    }
}

int isoob(nx, ny)
short nx, ny;
{
    return (nx < 0 || (nx > (BSIZE - 1)) || ny < 0 || (ny > (BSIZE - 1)));
}

void
growarrow(x, y, tile)
short x, y, tile;
{
    short len;
    short nx, ny, newtile;

    len = 0;
    while (ISCLEAR(x,y))
    {
        /*
        printf("%d,%d dir=%d dx=%d dy=%d len=%d  \015", x, y, tile, deltamove[tile&15][0], deltamove[tile&15][1], len);
       */
        board[y * 64 + x] = tile;
        /* mark first cell */
        if (len == 0)
            board[y * 64 + x] |= FIRST;

        /* next move */
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];

        /* is it blocked? or too long? */
        if (isoob(nx,ny) || (ISUSED(nx,ny)) || (len > 4 && ((rand() & 31) < 8)))
        {
            short newtile;

            /* try turn left or right */
            newtile = tile + ((rand() & 1) ? 1 : -1);
            if (newtile < N) newtile = W;
            if (newtile > W) newtile = N;
      /*
               printf("newdir=%d (%d)  \015", newtile, tile + (newtile << 2));
               */

            nx = x + deltamove[newtile][0];
            ny = y + deltamove[newtile][1];

            if (isoob(nx, ny) || (ISUSED(nx,ny)))
            {
                break;
            }

            /* make a corner and encode next direction */
            board[y * 64 + x] = CORNER | tile + (newtile << 2);

            if (len == 0)
                board[y * 64 + x] = FIRST | newtile;

            tile = newtile;
        }

        /* chance early out */
        if (len > 8 && (len > (rand() & 255)))
            break;

        /* move to next */
        x = nx;
        y = ny;

        len++;
    }

    /* mark last tile */
    board[y * 64 + x] |= LAST;

    /* never create runts */
    if (len == 0)
        board[y * 64 + x] = CL;

#ifdef DEBUG
    drawlevel();
    waitbutton("xxx");
#endif
}

packedxy chooseneighbour(x, y)
short x, y;
{
    short tile, nx, ny, r;

    /* ensure we have at least 1 move */
    tile = rand() & 3;
    for (r = 0; r < 4; r++)
    {
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];
        if (ISCLEAR(nx,ny))
            break;
        tile++;
        tile &= 3;
    }

    if (r < 4)
        return PACKXY(nx,ny);
    else
        return -1;
}

int choosedirection(x, y, tx, ty)
short x, y, tx, ty;
{
    short tile, nx, ny, r;

#if 1
    /* choose direction pointing at tx,ty*/
    nx = tx - x;
    ny = ty - y;
    if (nx == 0)
        tile = (ny > 0) ? S : N;
    if (ny == 0)
        tile = (nx > 0) ? E : W;
#else
    /* ensure we have at least 1 move */
    tile = rand() & 3;
    for (r = 0; r < 4; r++)
    {
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];
        if (ISCLEAR(nx,ny))
            break;
        tile++;
        tile &= 3;
    }
#endif

    return tile;
}

void
buildlevel()
{
    packedxy xy;
    short x, y, r, tile;
    short len;
    short nx, ny;
    short edgers;

    /* clear it */
    for (y = 0; y < BSIZE; y++)
    {
        for (x = 0; x < BSIZE; x++)
        {
            board[y * 64 + x] = CL;
        }
    }
#if 0
    /* mask out playfield */
    for (x = 0; x < BSIZE; x++)
    {
        /* fail ISCLEAR pass ISUSED */
        board[BSIZE/2 * 64 + x] = 0x10;
        board[x * 64 + BSIZE/2] = 0x10;
    }
#endif

    edgers = NUMEDGERS;
    while (edgers--)
    {
        /* find a start on one of the edges that is unused */
        do
        {
            r = rand() % BSIZE;
            switch (rand() & 3)
            {
            case 0:
                tile = S;
                x = r;
                y = 0;
                break;
            case 1:
                tile = W;
                x = BSIZE-1;
                y = r;
                break;
            case 2:
                tile = N;
                x = r;
                y = BSIZE - 1;
                break;
            case 3:
                tile = E;
                x = 0;
                y = r;
                break;
            }

        } while (ISUSED(x,y));

        /* grow it */
        growarrow(x, y, tile);

        /* try grafting another onto tail */
        xy = findtail(x, y);
        if (xy > 0)
        {
            nx = UNPACKX(xy);
            ny = UNPACKY(xy);
            xy = chooseneighbour(nx, ny);
            if (xy > 0)
            {
                x = UNPACKX(xy);
                y = UNPACKY(xy);
                tile = choosedirection(x, y, nx,ny);
                growarrow(x, y, tile);

                /* try grafting another onto tail */
                xy = findtail(x, y);
                if (xy > 0)
                {
                    nx = UNPACKX(xy);
                    ny = UNPACKY(xy);
                    xy = chooseneighbour(nx, ny);
                    if (xy > 0)
                    {
                        x = UNPACKX(xy);
                        y = UNPACKY(xy);
                        tile = choosedirection(x, y, nx, ny);
                        growarrow(x, y, tile);
                    }
                }

            }
        }
    }

    for (y = 0; y < BSIZE; y++)
    {
        for (x = 0; x < BSIZE; x++)
        {
            if (ISCLEAR(x,y))
            {

                /* ensure we have at least 1 move */
                tile = rand() & 3;
                for (r = 0; r < 4; r++)
                {
                    nx = x + deltamove[tile][0];
                    ny = y + deltamove[tile][1];
                    if (ISCLEAR(nx, ny))
                        break;
                    tile++;
                    tile &= 3;
                }

                growarrow(x, y, tile);
            }
        }
    }

}

int istip(x, y)
short x, y;
{

    return (!isoob(x,y) && (board[y * 64 + x] & FIRST));
}

int canmove(x, y)
short x, y;
{
    short tile = board[y * 64 + x] & 3;
    short i;

    /* keep moving in same direction until board edge */
    while(1)
    {
        /* NB reverse direction */
        x = x - deltamove[tile][0];
        y = y - deltamove[tile][1];

        if (isoob(x, y))
            break;

        if (ISUSED(x,y))
            return 0;
    }

    /* has clear path */
    return 1;
}

packedxy findhead(x, y)
short x, y;
{
    short len;

    len = 256;
    while (--len > 0)
    {
        short nx, ny;
        short tile;
        short newtile;

        /* follow direction and handle corners */
        tile = board[y * 64 + x];
        if (tile & FIRST)
            return PACKXY(x, y);

        tile &= 3;

        /* get to previous tile */
        nx = x - deltamove[tile][0];
        ny = y - deltamove[tile][1];

        x = nx;
        y = ny;
    }
    printf("find: FAIL\015");
    return -1;
}

packedxy findtail(x, y)
short x, y;
{
    short len;

    len = 256;
    while (--len > 0)
    {
        short nx, ny;
        short tile;
        short newtile;

        /* follow direction and handle corners */
        tile = board[y * 64 + x];
        if ((tile & LAST) || ISCLEAR(x,y))
            return PACKXY(x,y);
 
         if (tile & CORNER)
             tile >>= 2;
         tile &= 3;

        /* get next tile */
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];

        /* last known good */
        if (ISCLEAR(nx,ny))
            return PACKXY(x, y);

        x = nx;
        y = ny;
    }
    printf("find: FAIL\015");
    return -1;
}

void retire(x, y)
short x, y;
{
    packedxy xy;
    short tile = board[y * 64 + x] & 3;
    short nx, ny;
    short lx, ly;

    lx = ly = -1;

    /* extracting from board */
    nx = x;
    ny = y;
    while ((lx != x) || (ly != y))
    {
        bb.srcform = straights[tile];
        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;
        bb.destrect.x = BX + nx * 8;
        bb.destrect.y = BY + ny * 8;
        bb.destrect.w = 8;
        bb.destrect.h = 8;
        bb.rule = bbS;
        BitBlt(&bb);

        /* NB reverse direction */
        nx = nx - deltamove[tile][0];
        ny = ny - deltamove[tile][1];

        bb.srcform = tips[tile];
        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;
        bb.destrect.x = BX + nx * 8;
        bb.destrect.y = BY + ny * 8;
        bb.destrect.w = 8;
        bb.destrect.h = 8;
        bb.rule = bbS;
        BitBlt(&bb);

        /* erase tail */
        xy = findtail(x, y);
        if (xy > 0)
        {
            lx = UNPACKX(xy);
            ly = UNPACKY(xy);
            bb.srcform = &BlackMask;
            bb.srcpoint.x = 0;
            bb.srcpoint.y = 0;
            bb.destrect.x = BX + lx * 8;
            bb.destrect.y = BY + ly * 8;
            bb.destrect.w = 8;
            bb.destrect.h = 8;
            bb.rule = bbnS;
            BitBlt(&bb);

            board[ly * 64 + lx] = CL;
        }

        waitframes(2);
    }

    /* cleaning up until offscreen */
    do
    {
        /* NB reverse direction */
        lx = lx - deltamove[tile][0];
        ly = ly - deltamove[tile][1];

        bb.srcform = &BlackMask;
        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;
        bb.destrect.x = BX + lx * 8;
        bb.destrect.y = BY + ly * 8;
        bb.destrect.w = 8;
        bb.destrect.h = 8;
        bb.rule = bbnS;
        BitBlt(&bb);

        waitframes(2);

    } while (bb.destrect.x > 0 && bb.destrect.y > 0 && bb.destrect.x < 640 && bb.destrect.y < 480);

}

main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i;
    struct POINT origin,p2;
    struct RECT r;
    short oldx, oldy;

    BSIZE = 32;
    for (i = 1; i < argc; i++)
    {
        BSIZE = atol(argv[i]);
    }
    if (BSIZE > MAXBOARD)
        exit(-1);
    
    printf("\033[30;33r\033[1H");

    SaveDisplayState(&ds);

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    screen = InitGraphics(FALSE);
    font = FontOpen("/fonts/PellucidaSans-Serif36.font");

    ESetSignal();
    frametime = EGetTime() + 500;
    framenum = 0;
    ESetAlarm(frametime);

    /* for controlling clipping etc */
    BbcomDefault(&bb);

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
    bb.cliprect.w = 640;
    bb.cliprect.h = 480;

    origin.x = origin.y = 200;
    SetMPosition(&origin);
    CursorVisible(0);
    CursorTrack(1);

   buildblocks();

   buildlevel();

   drawlevel();

   /* animate it */
   while(1)
   {
       char msg[32];
       short x, y;

       GetMPosition(&origin);
       x = (origin.x - BX) / 8;
       y = (origin.y - BY) / 8;

       /* redraw cursor */
       if (x != oldx || y != oldy)
       {
           /* remove old */
           r.x = BX + oldx * 8 + 1;
           r.y = BY + oldy * 8 + 1;
           r.w = 6;
           r.h = 6;
           bb.rule = bbZero;
           bb.srcform = NULL;
           RectBoxDrawX(&r, 1, &bb);

           if (!isoob(oldx, oldy))
           {
               bb.rule = bbS;
               drawcell(oldx, oldy);
           }

           /* draw new */
           r.x = BX + x * 8 + 1;
           r.y = BY + y * 8 + 1;
           r.w = 6;
           r.h = 6;
           bb.rule = bbS;
           bb.srcform = NULL;
           RectBoxDrawX(&r, 1, &bb);

           oldx = x;
           oldy = y;
       }

       if (GetButtons() & M_LEFT)
       {
           packedxy xy;

           xy = findhead(x, y);
           x = UNPACKX(xy);
           y = UNPACKY(xy);

           if (istip(x, y))
           {
               if (canmove(x, y))
               {
                   retire(x, y);
                   oldx = oldy = -1;
               }
               else
               {
                   printf("*** BLOCKED\015");
               }
           }
           else
           {
               printf("*** NOT TIP\015");
           }

           waitbutton(msg);
       }

       waitframes(1);
       sprintf(msg, "cell[%d,%d 0x%2.2x]    ", x, y, board[y * BSIZE + x] );
       origin.x = 250;
       origin.y = 16;
       StringDraw(msg, &origin);
   }

}
