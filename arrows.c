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
  char buffer[64];

     origin.x = 250;
     origin.y = 16;
     sprintf(buffer, "wait %s", msg);
     StringDraw(buffer, &origin);

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

#define BSIZE 42
#define BX (640 - BSIZE*8)/2
#define BY (480 - BSIZE*8)/2

/* more external arrows => easier.. */
#define NUMEDGERS 20

#define FIRST 0x80
#define LAST 0x40
#define CORNER 0x20

#define CL  255
#define N   0
#define E   1
#define S   2
#define W   3

#define NW  (N + (W<<2))
#define NE  (N + (E<<2))
#define SE  (S + (E<<2))
#define SW  (S + (W<<2))
#define EN  (E + (N<<2))
#define ES  (E + (S<<2))
#define WS  (W + (S<<2))
#define WN  (W + (N<<2))

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

unsigned char board[BSIZE][BSIZE];
int deltamove[4][2] = { {0,-1},{1,0},{0,1},{-1,0} };

void drawcell(x, y)
short x, y;
{
    if (board[y][x] != CL)
    {
        if (board[y][x] & CORNER)
        {
            bb.srcform = corners[board[y][x] & 15];
        }
        else
        if (board[y][x] & FIRST)
        {
            bb.srcform = tips[board[y][x] & 3];
        }
        else
        {
            bb.srcform = straights[board[y][x] & 3];
        }

#if 0
        /* mark last */
        bb.halftoneform = (board[y][x] & LAST) ? &GrayMask : &BlackMask;
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

    for (r = 0; r <= l; r++)
    {
        drawcell(x+r, y);
    }
}

void drawvline(x, y, l)
short x, y, l;
{
    short r;

    for (r = 0; r <= l; r++)
    {
        drawcell(x, y+r);
    }
}

void drawlevel()
{
    short x, y, r;

    x = (BSIZE / 2) - 1;
    y = (BSIZE / 2);
    for (r = 1; r < BSIZE / 2; r++)
    {
        drawhline(x - r, y - r, r + r);
        drawhline(x - r, y + r, r + r);

        drawvline(x - r, y - r, r + r);
        drawvline(x + r, y - r, r + r);
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
    while ((board[y][x] == CL))
    {
        /*
        printf("%d,%d dir=%d dx=%d dy=%d len=%d  \015", x, y, tile, deltamove[tile&15][0], deltamove[tile&15][1], len);
       */
        board[y][x] = tile;
        /* mark first cell */
        if (len == 0)
            board[y][x] |= FIRST;

        /* next move */
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];

        /* is it blocked? or too long? */
        if (isoob(nx,ny) || (board[ny][nx] != CL) || (len > 4 && ((rand() & 31) < 8)))
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

            if (isoob(nx, ny) || (board[ny][nx] != CL))
            {
                break;
            }

            /* make a corner and encode next direction */
            board[y][x] = CORNER | tile + (newtile << 2);

            if (len == 0)
                board[y][x] = FIRST | newtile;

            tile = newtile;
        }

        /* chance early out */
        if (len > 4 && (len > (rand() & 255)))
            break;

        /* move to next */
        x = nx;
        y = ny;

        len++;
    }

    /* mark last tile */
    board[y][x] |= LAST;

    /* never create runts */
    if (len == 0)
        board[y][x] = CL;
}

void
buildlevel()
{
    short x, y, r, tile;
    short len;
    short nx, ny;
    short edgers;

    /* clear it */
    for (y = 0; y < BSIZE; y++)
    {
        for (x = 0; x < BSIZE; x++)
        {
            board[y][x] = CL;
        }
    }

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

        } while (board[y][x] != CL);

        /* grow it */
        growarrow(x, y, tile);
    }

    for (y = 0; y < BSIZE; y++)
    {
        for (x = 0; x < BSIZE; x++)
        {
            if (board[y][x] == CL)
            {
                /* ensure we have at least 1 move */
                tile = rand() & 3;
                for (r=0; r<4; r++)
                {
                    nx = x + deltamove[tile][0];
                    ny = y + deltamove[tile][1];
                    if (board[ny][nx] == CL)
                        break;
                    tile++;
                    tile &= 3;
                }
                if (r<4)
                    growarrow(x, y, tile);
            }
        }
    }

}

int istip(x, y)
short x, y;
{

    return (board[y][x] & FIRST);
}

int canmove(x, y)
short x, y;
{
    short tile = board[y][x] & 3;
    short i;

    /* keep moving in same direction until board edge */
    while(1)
    {
        /* NB reverse direction */
        x = x - deltamove[tile][0];
        y = y - deltamove[tile][1];

        if (isoob(x, y))
            break;

        if (board[y][x] != CL)
            return 0;
    }

    /* has clear path */
    return 1;
}

int findtail(x, y)
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
        tile = board[y][x];
        if ((tile & LAST) || (tile == CL))
            return y * BSIZE + x;
 
         if (tile & CORNER)
             tile >>= 2;
         tile &= 3;

        /* get next tile */
        nx = x + deltamove[tile][0];
        ny = y + deltamove[tile][1];

        /* last known good */
        if (board[ny][nx] == CL)
            return y * BSIZE + x;

        x = nx;
        y = ny;
    }
    printf("find: FAIL\015");
    return -1;
}

void retire(x, y)
short x, y;
{
    short tile = board[y][x] & 3;
    short nx, ny, xy;
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
            lx = xy % BSIZE;
            ly = xy / BSIZE;
            bb.srcform = &BlackMask;
            bb.srcpoint.x = 0;
            bb.srcpoint.y = 0;
            bb.destrect.x = BX + lx * 8;
            bb.destrect.y = BY + ly * 8;
            bb.destrect.w = 8;
            bb.destrect.h = 8;
            bb.rule = bbnS;
            BitBlt(&bb);

            board[ly][lx] = CL;
        }

        waitframes(1);
    }

    /* cleaning up */
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

        waitframes(1);
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

    printf("\033[30;33r\033[1H");

    SaveDisplayState(&ds);

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    screen = InitGraphics(FALSE);
    font = FontOpen("/fonts/PellucidaSans-Serif36.font");
   fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
      font->maps->fixed,font->maps->maxw, 
      font->maps->line, font->maps->baseline);

    printf("starting alarm\n");
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
       short x, y;

       GetMPosition(&origin);
       x = (origin.x - BX) / 8;
       y = (origin.y - BY) / 8;

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
           char msg[32];

           if (istip(x, y))
           {
               if (canmove(x, y))
               {
                   retire(x, y);
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

           sprintf(msg, "cell %d,%d 0x%2.2x  ", x, y, board[y][x]);
           waitbutton(msg);
       }





   }

}
