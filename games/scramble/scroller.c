#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

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

struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
int pindex;
int scrolldir = 1;
struct POINT page;
unsigned long frametime;
unsigned int framenum;

#define MAPWIDTH 1024
#define MAPHEIGHT 29
unsigned char cellmap[MAPHEIGHT][MAPWIDTH];
struct FORM* tile[256];
#define EMPTY 16
#define FILLED 60

sprite ship;
int shipx = 64+8;
int shipy = 64+8;

void sh_timer(sig)
int sig;
{

  frametime += 33;
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

            dst = form->addr;
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

    draw_bounds(aform->w, aform->h);
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

    draw_bounds(w, h);

    /* we need to be larger enough to include the cells behind */
    cw = 16 + ((w + 15) & -16);
    ch = 16 + ((h + 15) & -16);
    asprite->cached_pf = FormCreate(cw, ch);
    asprite->composed_pf = FormCreate(cw,ch);
    printf("compose_pf: %d %d\n", cw, ch);

    asprite->oldx = asprite->oldy = -1;
}

void draw_cell(x, y, cellx, celly)
int x, y;
int cellx,celly;
{
    short tileid = cellmap[celly][cellx];

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

struct FORM *maketile(name)
char *name;
{
  struct FORM *tile;
  struct POINT origin;
  struct RECT r;

    bb.rule = bbS;
    tile = FormCreate(16, 16);
    bb.destform = tile;
    bb.halftoneform = &WhiteMask;
    r.x = r.y = 0;
    r.w = tile->w;
    r.h = tile->h;
    RectDrawX(&r, &bb);

    bb.halftoneform = NULL;
    origin.x = 0;
    origin.y = font->maps->baseline;
    StringDrawX(name, &origin, &bb, font);

    draw_bounds(tile->w, tile->h);

    return tile;
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

    cellcol = (unsigned char *)cellmap + cellx;
    for (i = 0; i < MAPHEIGHT; i++)
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
    for(i=0; i<40; i++)
    {
        fillcol(pagex + (i << 4), y, cellx + i);
    }
}

/* cache playfield behind */
void compose_sprite(asprite, cellx)
sprite* asprite;
int cellx;
{
    int cx, cy;

    /* restore old playfield */
    if (asprite->oldx >= 0)
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

        bb.destrect.x = 0;
        bb.destrect.y = 0;
        BitBlt(&bb);

        asprite->oldx = -1;
        asprite->oldy = -1;
    }


    /* build background from playfield we're over */
    bb.destform = asprite->cached_pf;
    bb.rule = bbS;
    cx = cellx + (PIX2CELL(shipx));
    cy = PIX2CELL(shipy);

    draw_cell(0, 0, cx, cy);
    draw_cell(16, 0, cx + 1, cy);
    draw_cell(32, 0, cx + 2, cy);
    draw_cell(0, 16, cx, cy + 1);
    draw_cell(16, 16, cx + 1, cy + 1);
    draw_cell(32, 16, cx + 2, cy + 1);
#if 1
    draw_cell(0, 32, cx, cy + 2);
    draw_cell(16, 32, cx + 1, cy + 2);
    draw_cell(32, 32, cx + 2, cy + 2);
#endif

    /* fast copy bits */
    memcpy(asprite->composed_pf->addr, asprite->cached_pf->addr, asprite->cached_pf->w * asprite->cached_pf->h >> 3);

    /* compose ship */
    bb.rule = bbSorD;
    bb.srcform = asprite->form;
    bb.destform = asprite->composed_pf;
    bb.destrect.x = shipx & 15;
    bb.destrect.y = shipy & 15;
    bb.destrect.w = asprite->form->w;
    bb.destrect.h = asprite->form->h;
    bb.halftoneform = NULL;
    BitBlt(&bb);

    /* show bounds */
    draw_bounds(asprite->composed_pf->w, asprite->composed_pf->h);

    /* copy to active display */
    bb.rule = bbS;
    bb.srcform = asprite->composed_pf;
    asprite->oldx = page.x + (shipx & -16);
    asprite->oldy = page.y + (shipy & -16);
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
char* endptr;
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
    unsigned int currpage,waiting;
    float angle;
    char infoline[64];
    struct FORM *atlas;
    int aw,ah, oldleftcol, oldrightcol, pmem_fd;

    unsigned short nop_instr = 0x4e71;
    unsigned int shared_page1 = 0x708;
    unsigned int kernelmem = 0x0078a000;

    signal(SIGINT, sh_int);
    signal(SIGMILLI, sh_timer);

    /* map kernel variables into our space */
    pmem_fd = open("/dev/pmem", O_RDWR);

  /* overwrite BEQ with NOP so we can do phys(3) */
    lseek(pmem_fd, 0x000fe04, SEEK_SET);
    write(pmem_fd, &nop_instr, 2);

    /* phys(3) gets address to map from shared_page1 */
    lseek(pmem_fd, shared_page1, SEEK_SET);
    write(pmem_fd, &kernelmem, 4);

    screen = InitGraphics(argc == 1 ? TRUE : FALSE);

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
    printf("ESetAlarm(%d)\n", frametime);

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
        exit(-1);
    }


#if 1
    /* make ship */
    shipx = 200;
    shipy = 128;
    makeship(&ship, 32, 32);

    drawform(ship.form, 0, 0);
#endif

    /* some landscape */
    memset(cellmap, EMPTY, sizeof(cellmap));
    decode((struct groundslice *)landdat, landdat2 - 10);

    /* playfield start position */
    pfx = INT2FIX(0);
    pfdir = INT2FIX(2);

    if (argc == 1)
    {
        /* initial screen fill */
        fillscreen(FIX2INT(pfx), 0);
        fillscreen(FIX2INT(pfx), 512);
    }
#if 0
    else
    {
        pfdir = 0;
    }
#endif

    oldrightcol = 0;

    /* animate it */
    while(1)
    {
        int fbx, cellx, pixx;
 
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

        if (argc == 1) SetViewport(&page);

        /* wait for frame flip */
        waiting = 0;
        currpage = framenum;
        while (currpage == framenum)
            waiting++;
        currpage = framenum;

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

        {
            struct POINT p;

            GetMPosition(&p);
            p.x &= -2;
            p.x -= page.x;
            p.y -= page.y;
            if (GetButtons() & M_LEFT)
            {
                if (shipx != p.x || shipy != p.y)
                {
                    shipx = p.x;
                    shipy = p.y;
                }
            }
        }
#if 0
        /* animated sprite */
        compose_sprite(&ship, cellx);

        showinfo(infoline);
#endif
        /* scroll */
        pfx += pfdir;
    }
}
