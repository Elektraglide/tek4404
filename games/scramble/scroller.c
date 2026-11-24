#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>
#include <sys/sgtty.h>

#ifndef unix
#info scrolldemo
#info Version 1.0
#tstmp
#endif

/*
Skip setting srcpoint.xy since its rarely is not 0,0


*/

#define PIX2CELL(A) ((A)>>4)

#define INT2FIX(A)  ((A)<<8)
#define FIX2INT(A)  ((A)>>8)

typedef long fixed;

typedef struct
{
  struct FORM *form;
  fixed x,y;
  fixed dx,dy;

  short cx, cy, len;
  struct FORM* cached_pf;
  struct FORM* composed_pf;
  int oldx,oldy;
} sprite;

typedef struct
{
    unsigned char identsize, cmaptype, imagetype;
    unsigned char cmapstart[2], cmaplen[2], cmapbits;
    unsigned char xstart[2], ystart[2], width[2], height[2];
    unsigned char bits, desc;
} tgaheader;

extern unsigned char landdat[];
extern unsigned char landdat2[];

int fdtty;
struct sgttyb orig_term_settings;
struct sgttyb new_term_settings;

struct DISPSTATE ds;
struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
int pindex;
int scrolldir = 1;
struct POINT page;

/* frame timing using SIGMILLI, so we use a fixedpoint to be able to advance by 33.3ms each frame */
#define ALARMFRAC 4
unsigned long alarmtime;
unsigned int framenum;

unsigned char* kernelptr;
unsigned char* displaystateptr;
unsigned char* tenthptr;

#define MAPWIDTH 1024
#define MAPHEIGHT 29
#define DRAWHEIGHT 29
unsigned char cellmap[MAPHEIGHT][MAPWIDTH];
unsigned char* drawmap;
struct FORM* tile[256];
#define EMPTY 16
#define FILLED 60

/* screen-relative ship */
sprite ship;
int shipx;
int shipy;

unsigned int alarmtimes[32];
unsigned int rendertimes[32];
unsigned int pretimes[32];
unsigned int times[32];
unsigned int tindex = 0;

void wait_vblank()
{
    register unsigned char* ptr = tenthptr;
    unsigned char old;
    unsigned int currpage, waiting;

    /* record absolute SetAlarm time */
    alarmtimes[tindex] = alarmtime >> ALARMFRAC;

    /* start of busy wait */
    pretimes[tindex] = EGetTime();

    /* only if we're early as expected */
    if (pretimes[tindex] < alarmtimes[tindex])
    {
        /* COARSE: wait for SIGMILLI at 33ms (early for a 33.3ms framerate) */
        waiting = 0;
        currpage = framenum;
        while (currpage == framenum)
            waiting++;

        /* FINE: wait for a vblank to tick over tenthsec counter */
        old = *ptr;
        while (old == *ptr);
    }

    /* end of busy wait / end of frame */
    times[tindex] = EGetTime();

    tindex++;
    tindex &= 31;
}

void sh_timer(sig)
int sig;
{
  alarmtime += 533; /* 33.3125 ms */
  framenum++;

  signal(sig, sh_timer);
  ESetAlarm((alarmtime >> ALARMFRAC) & 0x7fffffff);
}

void sh_int(sig)
int sig;
{
    struct POINT p;
    int i;

    if (sig == SIGALRM)
        printf("*** Watchdog ***\n");

    p.x = p.y = 0;
    SetViewport(&p);
    ExitGraphics();
    FontClose(font);
    RestoreDisplayState(&ds);

    stty(fdtty, &orig_term_settings);

    for (i = 1; i < 16; i++)
        printf("frame[%2d]: render(%d) busywait(%d) frametime(%d) alarmtime(%d)\n", i, pretimes[i] - rendertimes[i], times[i] - pretimes[i], times[i] - times[i-1], alarmtimes[i]);

    exit(2);
}

struct FORM* readtga(filename, w, h)
char* filename;
int* w, * h;
{
    int fd, len;
    tgaheader header;
    struct FORM* form = NULL;

    fd = open(filename, O_RDONLY);
    if (fd > 0)
    {
        read(fd, &header, sizeof(header));

        *w = header.width[0] + header.width[1] * 256;
        *h = header.height[0] + header.height[1] * 256;

        /* uncompressed b&w */
        if (header.imagetype != 3)
        {
            fprintf(stderr, "FormCreate: not supported: imagetype: %d\n", header.imagetype);
        }

        if ((header.desc & 32) == 0)
        {
            fprintf(stderr, "FormCreate: origin: lower left\n");
        }

        form = FormCreate(*w, *h);
        if (form)
        {
            char* dst;
            int i, stride;

            dst = (char *)form->addr;
            stride = ((*w + 15) & -16) >> 3;
            printf("atlas: %d %d  stride: %d\n", *w, *h, stride);

            /* needs flipping */
            if ((header.desc & 32) == 0)
            {
                dst += stride * (*h - 1);
                stride = -stride;
            }

            for (i = 0; i < *h; i++)
            {
                read(fd, dst, ((*w + 7) >> 3));
                dst += stride;
            }
        }
        else
        {
            fprintf(stderr, "FormCreate: %s\n", strerror(errno));
        }

        close(fd);
    }
    else
    {
        fprintf(stderr, "FAILED TO OPEN: %s\n", filename);
    }

    return form;
}

void draw_bounds(w, h)
int w, h;
{
    struct RECT r;

    bb.rule = bbS;
    bb.srcform = NULL;
    bb.halftoneform = &BlackMask;
    r.x = 1;
    r.y = 1;
    r.w = w - 2;
    r.h = h - 2;
    RectBoxDrawX(&r, 1, &bb);
}

void drawform(aform, x, y)
struct FORM* aform;
int x, y;
{
    struct RECT r;

    /* show it */
    bb.rule = bbS;
    bb.srcform = aform;
    bb.destform = screen;
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = aform->w;
    bb.destrect.h = aform->h;
    bb.halftoneform = NULL;
    BitBlt(&bb);

    bb.rule = bbS;
    bb.srcform = NULL;
    bb.halftoneform = &BlackMask;
    r.x = x + 1;
    r.y = y + 1;
    r.w = aform->w - 2;
    r.h = aform->h - 2;
    RectBoxDrawX(&r, 1, &bb);
}

void draw_tile(x, y, tileid)
int x, y;
int tileid;
{

    bb.srcform = tile[tileid];
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.halftoneform = NULL;
    BitBlt(&bb);
}

struct FORM* cache_tile(atlas, x, y)
struct FORM* atlas;
int x, y;
{
    struct FORM* tile;

    tile = FormCreate(16, 16);
    if (tile)
    {
        bb.rule = bbS;
        bb.srcform = atlas;
        bb.srcpoint.x = x;
        bb.srcpoint.y = y;
        bb.destform = tile;
        bb.destrect.x = 0;
        bb.destrect.y = 0;
        bb.destrect.w = 16;
        bb.destrect.h = 16;
        bb.halftoneform = NULL;
        BitBlt(&bb);
    }

    return tile;
}


void drawsprite(asprite)
sprite *asprite;
{
    /* draw over */
    bb.rule = bbSorD;
    bb.srcform = asprite->composed_pf;
    bb.destform = screen;
	bb.destrect.x = FIX2INT(asprite->x);
    bb.destrect.y = FIX2INT(asprite->y) + (page.y);
	bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
	BitBlt(&bb);
}

void makeship(asprite,w,h)
sprite* asprite;
int w,h;
{
    struct POINT origin;
    struct RECT r;
    int cw,ch;

    asprite->form = FormCreate(w,h);

    /* make some shape to show */
    bb.rule = bbS;
    bb.srcform = NULL;
    bb.destform = asprite->form;
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = w;
    bb.destrect.h = h;
    bb.halftoneform = &WhiteMask;
    r.x = r.y = 0;
    r.w = w;
    r.h = h;
    RectDrawX(&r, &bb);

    draw_tile(0, 0, 10);
    draw_tile(16, 0, 11);
#ifdef DEBUG
    draw_bounds(w, h);
#endif

    /* we need to be larger enough to include the cells behind */
    cw = 16 + ((w + 15) & -16);
    ch = 16 + ((h + 15) & -16);
    asprite->cached_pf = FormCreate(cw, ch);
    asprite->composed_pf = FormCreate(cw,ch);
    /* cache number of bytes for bitmap */
    asprite->len = asprite->cached_pf->w * asprite->cached_pf->h >> 3;
    printf("compose_pf: %d %d\n", cw, ch);

    asprite->oldx = asprite->oldy = -1;
}

void draw_cell(x, y, cellx, celly)
int x, y;
int cellx,celly;
{
    short tileid = drawmap[(celly * MAPWIDTH) + cellx];

    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.halftoneform = NULL;

    /* fastpath for blank */
    if (tileid == EMPTY)
    {
        /* FIXME: have a ready-to-use bb */
        bb.rule = bbZero;
        bb.srcform = NULL;
        BitBlt(&bb);
        bb.rule = bbS;
    }
    else
    {
        bb.srcform = tile[tileid];
        BitBlt(&bb);
    }
}

void fillcol(x,y,cellx)
int x,y;
int cellx;
{
    register unsigned char *cellcol;
    short i;

    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.halftoneform = NULL;

    cellcol = (unsigned char *)drawmap + cellx;
    for (i = 0; i < DRAWHEIGHT; i++)
    {
        /* fastpath for blank */
        if (*cellcol == EMPTY)
        {
            /* FIXME: have a ready-to-use bb */
            bb.rule = bbZero;
            bb.srcform = NULL;
            BitBlt(&bb);
            bb.rule = bbS;
        }
        else
        {
            bb.srcform = tile[(*cellcol)];
            BitBlt(&bb);
        }
        bb.destrect.y += 16;

        /* stride to next row */
        cellcol += MAPWIDTH;
    }
}

void animated_pf(x, y, cell)
int x, y;
int cell;
{

    bb.srcform = tile[cell];
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.halftoneform = NULL;
    BitBlt(&bb);
    bb.destrect.x += 16;
    BitBlt(&bb);
    bb.destrect.y += 16;
    BitBlt(&bb);
    bb.destrect.x -= 16;
    BitBlt(&bb);
}

void fillscreen(pixx,y)
int pixx,y;
{
    int i, pagex, fbx, cellx;

    /* which cell are we starting on */
    cellx = PIX2CELL(pixx);

    /* which framebuffer window are we in? */
    fbx = pixx / 384;

    /* find scroll window offset */
    pagex = pixx - (fbx * 384);

    bb.rule = bbS;
    bb.destform = screen;
    for(i=0; i<40; i++)
    {
        fillcol(pagex + (i << 4), y, cellx + i);
    }
}

/* cache playfield behind */
void compose_sprite(asprite, pixx)
sprite* asprite;
int pixx;
{
    int cx, cy, rule;

    /* display relative coordinates */
    cx = (page.x + shipx) & -16;
    cy = (page.y + shipy) & -16;

    /* restore old playfield if we've moved */
    if (asprite->oldx > 0)
    {
        if (asprite->oldx != cx || asprite->oldy != cy)
        {
            bb.rule = bbS;
            bb.srcform = asprite->cached_pf;
            bb.destform = screen;
            bb.destrect.x = asprite->oldx;
            bb.destrect.y = asprite->oldy;
            bb.destrect.w = asprite->cached_pf->w;
            bb.destrect.h = asprite->cached_pf->h;
            bb.halftoneform = NULL;
            BitBlt(&bb);

            asprite->oldx = -1;
            asprite->oldy = -1;
        }
    }

    /* build background from playfield we're over if we dont have it */
    rule = bbSorD;
    cx = PIX2CELL(pixx + shipx);
    cy = PIX2CELL(shipy);
    if (asprite->cx != cx || asprite->cy != cy)
    {
        bb.rule = bbS;
        bb.destform = asprite->cached_pf;

        draw_cell(0, 0, cx, cy);
        draw_cell(16, 0, cx + 1, cy);
        draw_cell(32, 0, cx + 2, cy);
        draw_cell(0, 16, cx, cy + 1);
        draw_cell(16, 16, cx + 1, cy + 1);
        draw_cell(32, 16, cx + 2, cy + 1);
        draw_cell(0, 32, cx, cy + 2);
        draw_cell(16, 32, cx + 1, cy + 2);
        draw_cell(32, 32, cx + 2, cy + 2);

        asprite->cx = cx;
        asprite->cy = cy;
#ifdef DEBUG
        rule = bbSnorD;
#endif
    }

    /* fast copy bits */
    memcpy(asprite->composed_pf->addr, asprite->cached_pf->addr, asprite->len);

    /* compose ship */
    bb.rule = rule;
    bb.srcform = asprite->form;
    bb.destform = asprite->composed_pf;
    bb.destrect.x = (pixx + shipx) & 15;
    bb.destrect.y = (       shipy) & 15;
    bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
    bb.halftoneform = NULL;
    BitBlt(&bb);

#ifdef DEBUG
    /* show bounds */
    draw_bounds(asprite->composed_pf->w, asprite->composed_pf->h);
#endif

    /* copy to active display */
    asprite->oldx = (page.x + shipx) & -16;
    asprite->oldy = (page.y + shipy) & -16;
    bb.rule = bbS;
    bb.srcform = asprite->composed_pf;
    bb.destform = screen;
    bb.destrect.x = asprite->oldx;
    bb.destrect.y = asprite->oldy;
    bb.destrect.w = asprite->composed_pf->w;
    bb.destrect.h = asprite->composed_pf->h;
    bb.halftoneform = NULL;
    BitBlt(&bb);
}



void showinfo(buffer)
char *buffer;
{
    struct POINT origin;

    return;

    /* avoid wrapping text */
    bb.destrect.x = 0;
    bb.destrect.y = 0;
    bb.destrect.w = 1024;
    bb.destrect.h = 1024;

    origin.x = page.x;
    origin.y = page.y + 480 - 4;
    StringDrawX(buffer, &origin, &bb, font);
}

struct groundslice
{
    unsigned char dist0, tile0;
    unsigned char dist1, tile1;
    unsigned char hasceiling;
    unsigned char objectid;
};

/* struct padding means its simpler to just cast to a different struct */
struct ceilingslice
{
    unsigned char dist0, tile0;
    unsigned char dist1, tile1;
    unsigned char dist2, tile2;
    unsigned char dist3, tile3;
};

/* distances include topline scoreline etc */
#define DIST2CELL(A)  ((A>>3) - 2)

decode(ptr, endptr)
struct groundslice* ptr;
struct groundslice* endptr;
{
    int i,h,y0,y1,objid;

    /* lowest */
    for (i = 0; i < 30; i++)
    {
        cellmap[MAPHEIGHT - 7][i] = 55;
        cellmap[MAPHEIGHT - 6][i] = FILLED;
        cellmap[MAPHEIGHT - 5][i] = FILLED;
        cellmap[MAPHEIGHT - 4][i] = FILLED;
        cellmap[MAPHEIGHT - 3][i] = FILLED;
        cellmap[MAPHEIGHT - 2][i] = FILLED;
        cellmap[MAPHEIGHT - 1][i] = FILLED;
    }

    while (ptr < endptr)
    {
        /* ground level */
        h = DIST2CELL(ptr->dist0);

        if (h > MAPHEIGHT)
        {
            printf("too high: %d\n", h);
        }

        /* surface objects */
        objid = ptr->objectid;
        if (objid && !ptr->hasceiling)
        {
            switch (objid)
            {
            case 1:
                cellmap[h - 1][i + 1] = 113;
                cellmap[h - 2][i + 1] = 112;
                cellmap[h - 1][i] = 115;
                cellmap[h - 2][i] = 114;
                break;
            case 2:
                cellmap[h - 1][i] = 67;
                cellmap[h - 2][i] = 66;
                cellmap[h - 1][i + 1] = 65;
                cellmap[h - 2][i + 1] = 64;
                break;
            case 4:
                cellmap[h - 1][i + 1] = 125;
                cellmap[h - 2][i + 1] = 124;
                cellmap[h - 1][i] = 127;
                cellmap[h - 2][i] = 126;
                break;
            case 8:
            default:
                cellmap[h - 1][i + 1] = 206;
                cellmap[h - 2][i + 1] = 207;
                cellmap[h - 1][i] = 204;
                cellmap[h - 2][i] = 205;
                break;
            }
        }

        cellmap[h][i] = ptr->tile0;
        while (++h < MAPHEIGHT) {
            cellmap[h][i] = FILLED;
        };

        h = DIST2CELL(ptr->dist1);
        cellmap[h][i + 1] = ptr->tile1;
        while (++h < MAPHEIGHT) {
            cellmap[h][i + 1] = FILLED;
        };

        if (ptr->hasceiling)
        {
            struct ceilingslice *ceiling = (struct ceilingslice *)ptr;

            h = DIST2CELL(ceiling->dist2);
            cellmap[h][i] = ceiling->tile2;
            while (--h > 0) {
                cellmap[h][i] = FILLED;
            };
            h = DIST2CELL(ceiling->dist3);
            cellmap[h][i + 1] = ceiling->tile3;
            while (--h > 0) {
                cellmap[h][i + 1] = FILLED;
            };

            /* struct padding makes this 10 bytes.. */
            ptr = (struct groundslice*)((char *)ceiling + 9);
        }
        else
        {
            ptr++;
        }

        /* 2 cells per slice */
        i += 2;
    }
}

int *mapped_fpu;
unsigned int oldwaiting[2];
main(argc,argv)
int argc;
char *argv[];
{
    register int ret, i, col;
    fixed pfx,pfdir;
    struct POINT origin,p2;
    struct RECT r;
    float angle;
    char infoline[64];
    struct FORM *atlas;
    int aw,ah, oldleftcol, oldrightcol, pmem_fd;

    unsigned short nop_instr = 0x4e71;
    unsigned int shared_page1 = 0x708;
    unsigned int kerneladdr = 0x001000;

    /* read keyboard */
    fdtty = open(ttyname(fileno(stdin)), O_RDWR);
    ret = gtty(fdtty, &orig_term_settings);
    new_term_settings = orig_term_settings;
    new_term_settings.sg_flag |= CBREAK | RAW;
    stty(fdtty, &new_term_settings);

    signal(SIGINT, sh_int);
    signal(SIGTERM, sh_int);
    signal(SIGMILLI, sh_timer);

    /* 180 second watchdog armed */
    signal(SIGALRM, sh_int);
    alarm(180);

    /* map kernel variables into our space */
    pmem_fd = open("/dev/pmem", O_RDWR);

  /* overwrite BEQ with NOP so we can do phys(3) */
    lseek(pmem_fd, 0x000fe04, SEEK_SET);
    write(pmem_fd, &nop_instr, 2);

    /* phys(3) gets address to map from shared_page1 */
    lseek(pmem_fd, shared_page1, SEEK_SET);
    write(pmem_fd, &kerneladdr, 4);

    /* Map this so we can inspect DisplayState+0x18 which is the kernel-managed 0.1s counter (6 vblanks) used for event delivery */
    /* Checking whether it has changed indicates whether a VBL has occurred */
    kernelptr = phys(3);
    displaystateptr = kernelptr + 0xbe4;
    tenthptr = displaystateptr + 0x018;

    /* want lots of attention */
    nice(-20);

    SaveDisplayState(&ds);

    screen = InitGraphics(argc == 1 ? TRUE : FALSE);
    VideoNormal(0);

    font = (struct FontHeader *)FontOpen("/fonts/PellucidaSans-Serif10.font");
    fprintf(stderr, "fixed: %d width:%d height:%d baseline:%d\n", 
      font->maps->fixed,font->maps->maxw, 
      font->maps->line, font->maps->baseline);

    page.x = page.y = 0;
    pindex = 0;
    framenum = 0;

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

    /* read sprite atlas */
    atlas = readtga("scramble_tiles.tga", &aw, &ah);
    if (atlas)
    {
        /* extract into sprites */
        for(i=0; i<9; i++)
        {
            for(col=0;col<28; col++)
            {
                tile[i*28+col] = cache_tile(atlas, col * 16, i * 16);
            }
        }
        /* reset to 0,0 so we dont need to set it in general use */
        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;

        /* delete atlas */
        FormDestroy(atlas);
        atlas = NULL;
    }
    else
    {
        fprintf(stderr, "cannot create atlas\n");
        sh_int(-1);
    }


    /* make ship with screen-relative coordinates */
    shipx = 200;
    shipy = 144;
    makeship(&ship, 32, 32);

    /* some landscape */
    memset(cellmap, EMPTY, sizeof(cellmap));
    decode((struct groundslice *)landdat, (struct groundslice*)landdat2 - 2);

    /* calc an offset from which we will draw */
    drawmap = (char *)cellmap + (MAPHEIGHT - DRAWHEIGHT) * MAPWIDTH;

    /* playfield start position */
    pfx = INT2FIX(0);
    /* haltoning on 2x2 grid means minimum scroll disatnce is 2 */
    pfdir = INT2FIX(2);

    if (argc == 1)
    {
        /* initial screen fill */
        fillscreen(FIX2INT(pfx), 0);
        fillscreen(FIX2INT(pfx), 512);
    }

    oldrightcol = 0;

    ESetSignal();
    alarmtime = (EGetTime() + 250) << ALARMFRAC;
    ESetAlarm((alarmtime >> ALARMFRAC) & 0x7fffffff);

    /* animate it */
    while(1)
    {
        int fbx, cellx, pixx;
 
        gtty(fdtty, &new_term_settings);
        if (new_term_settings.sg_speed & INCHR)
        {
            char inputbuffer[8];
            ret = read(fdtty, inputbuffer, sizeof(inputbuffer));
            if (ret > 0)
            {
                if (inputbuffer[0] <= 0x1b)
                    sh_int(-5);

                if (inputbuffer[0] == 'w')
                    shipy -= 4;
                if (inputbuffer[0] == 'a')
                    shipx -= 4;
                if (inputbuffer[0] == 's')
                    shipy += 4;
                if (inputbuffer[0] == 'd')
                    shipx += 4;
            }
        }

        /* mark start of rendering */
        rendertimes[tindex] = EGetTime();

        /* top left coordinate */
        pixx = FIX2INT(pfx);

        /* which cell are we starting on */
        cellx = PIX2CELL(pixx);

        /* which framebuffer window are we in? */
        fbx = pixx / 384;

        /* find scroll window offset */
        page.x = pixx - (fbx * 384);

        /* decide which page based on even/odd framebuffer window */
        page.y = fbx & 1 ? 512 : 0;

#ifdef DEBUG
        /* infoline */
        sprintf(infoline, "framenum(%4.4x) %c pfx(%d) cellx(%d) fbx(%d)", framenum, ((framenum & 15) == 0) ? '*' : ' ', pixx, cellx, fbx);
#endif

        if (pfdir >= 0)
        {
            /* has leading edge column changed? render a new column */
            col = PIX2CELL(page.x + 640);
            if (col != oldrightcol)
            {
                oldrightcol = col;

                /* fill new right column just out of view */
                bb.rule = bbS;
                fillcol(col << 4, page.y, cellx + 40);

                /* fill on next page too */
                col = PIX2CELL(page.x);
                if (col >= 0)
                {
                    fillcol(((16+col) << 4), page.y ^ 512, cellx + 40 );

                    /* the region that overlaps far left and far right scroll window position also needs filling */
                    if (col < 16)
                    {
                        fillcol((col << 4), page.y ^ 512, cellx + 24 );
                    }
                }
            }
        }
        else /* reverse direction */
        {
            /* has leading edge column changed? render a new column */
            col = PIX2CELL(page.x);
            if (col != oldrightcol)
            {
                oldrightcol = col;

                /* fill new right column just out of view */
                bb.rule = bbS;
                fillcol(col << 4, page.y, cellx - 1);

                /* fill on next page too */
                col = PIX2CELL(page.x + 40);
                if (col >= 0)
                {
                    /* cellx is for x==0, we are 1 cell before */
                    fillcol(((16 + col) << 4), page.y ^ 512, cellx + 40 - 1);

                    /* the region that overlaps far left and far right scroll window position also needs filling */
                    if (col < 16)
                    {
                        fillcol((col << 4), page.y ^ 512, cellx + 24 - 1);
                    }
                }
            }
        }

#if 0
        /* animated playfield at 3.25Hz */
        if ((framenum & 15) == 0)
        {
            /* TODO: need a way of culling animated sprites based on current view window */
            if (pixx < 100)
            {
                animated_pf(64, page.y + 64, rand() & 7);
                animated_pf(128, page.y + 64, rand() & 7);
            }
            else
            if (pixx < 300)
            {
                animated_pf(192, page.y + 64, rand() & 7);
                animated_pf(224, page.y + 128, rand() & 7);
            }
        }
#endif

        if (0)
        {
            struct POINT p;

            GetMPosition(&p);
            p.x &= -2;
            if (GetButtons() & M_LEFT)
            {
                if (shipx != p.x || shipy != p.y)
                {
                    shipx = p.x;
                    shipy = p.y;
                }
            }
        }

        /* animated sprite */
        compose_sprite(&ship, pixx);

#ifdef DEBUG
        showinfo(infoline);
#endif

        if (argc == 1)
        {
            /* move display viewport */
            SetViewport(&page);
        }
        else
        {
            /* show where viewport *would* be */
            bb.rule = bbS;
            bb.srcform = NULL;
            bb.halftoneform = &GrayMask;
            r.x = page.x + 4;
            r.y = page.y + 4;
            r.w = 640 - 8;
            r.h = 480 - 8;
            RectBoxDrawX(&r, 4, &bb);
        }

        /* wait for vblank so we know viewport has been applied */
        wait_vblank();

        /* scroll */
        pfx += pfdir;
         
#if 0
        drawform(ship.cached_pf, page.x+64, page.y);
        drawform(ship.composed_pf, page.x+64+48, page.y);

        while ((GetButtons() & M_LEFT) == 0);
#endif

    }
}
