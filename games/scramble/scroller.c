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

#define INT2FIX(A)  ((A)<<8)
#define FIX2INT(A)  ((A)>>8)

typedef long fixed;

typedef struct
{
  struct FORM *form;
  fixed x,y;
  fixed dx,dy;

  struct FORM *orig_pf;
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

struct FontHeader *font;
struct FORM *screen;
struct BBCOM bb;
int pindex;
int scrolldir = 1;
struct POINT page;
unsigned long frametime;
unsigned int framenum;

void sh_timer(sig)
int sig;
{

  frametime += 32;
  framenum++;

  signal(sig, sh_timer);
  ESetAlarm(frametime);
}

void drawsprite(asprite)
sprite *asprite;
{
    /* draw over */
    bb.srcform = asprite->composed_pf;
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
            char *dst;
            int i,stride;

            dst = form->addr;
            stride = ((*w + 15) & -16) >> 3;
            printf("atlas: %d %d  stride: %d\n", *w, *h, stride);

            /* needs flipping */
            if ((header.desc & 32) == 0)
            {
                dst += stride * (*h - 1);
                stride = -stride;
            }

            for(i=0; i<*h; i++)
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
        fprintf(stderr, "FAILED TO OPEN\n");
    }

    return form;
}

struct FORM* copytile(atlas, x, y)
struct FORM *atlas;
int x,y;
{
    struct FORM* tile;

    tile = FormCreate(16, 16);
    if (tile)
    {
        bb.destform = tile;
        bb.destrect.x = 0;
        bb.destrect.y = 0;
        bb.destrect.w = 16;
        bb.destrect.h = 16;
        bb.rule = bbS;
        bb.halftoneform = NULL;
        bb.srcform = atlas;
        bb.srcpoint.x = x;
        bb.srcpoint.y = y;
        BitBlt(&bb);
    }

    return tile;
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
    r.x = r.y = 1;
    r.w = tile->w - 2;
    r.h = tile->h - 2;
    RectBoxDrawX(&r, 1, &bb);

    return tile;
}

#define MAPWIDTH 1024
#define MAPHEIGHT 28
unsigned char cellmap[MAPHEIGHT][MAPWIDTH];
struct FORM* tile[96];

void fillcol(x,y,cellx)
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

    cellcol = (unsigned char *)cellmap + cellx;
    for (i = 0; i < MAPHEIGHT; i++)
    {
        /* fastpath for blank */
        if (*cellcol == 0)
        {
            /* FIXME: have a ready-to-use bb */
            bb.rule = bbZero;
            bb.srcform = NULL;
            BitBlt(&bb);
            bb.rule = bbS;
        }
        else
        {
            bb.srcform = tile[(*cellcol) & 127];
            BitBlt(&bb);
        }
        bb.destrect.y += 16;
        cellcol += MAPWIDTH;
    }
}

void animated_pf(x, y, cell)
int x, y;
int cell;
{

    bb.halftoneform = NULL;
    bb.destrect.x = x;
    bb.destrect.y = y;
    bb.destrect.w = 16;
    bb.destrect.h = 16;
    bb.srcform = tile[cell];
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
    cellx = pixx >> 4;

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
    origin.y = page.y + 480 - 4;
    StringDrawX(buffer, &origin, &bb, font);
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



    screen = InitGraphics(FALSE);

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

    tile[0] = maketile("00");
    tile[1] = maketile("1 ");
    tile[2] = maketile("2 ");
    tile[3] = maketile("3 ");
    tile[4] = maketile("4 ");
    tile[5] = maketile("5 ");
    tile[6] = maketile("6 ");
    tile[7] = maketile("7 ");

    /* read sprite atlas */
    atlas = readtga("scramble_tiles.tga", &aw, &ah);
    if (atlas)
    {
        /* show it */
        bb.destform = screen;
        bb.destrect.x = 64;
        bb.destrect.y = 64;
        bb.destrect.w = 448;
        bb.destrect.h = 160;
        bb.rule = bbS;
        bb.halftoneform = NULL;
        bb.srcform = atlas;
        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;
        BitBlt(&bb);

        /* extract into sprites */
        for(i=0; i<3; i++)
        {
            for(col=0;col<28; col++)
            {
                tile[i*28+col] = copytile(atlas, col * 16, i * 16);
            }
        }

        bb.srcpoint.x = 0;
        bb.srcpoint.y = 0;

        /* delete atlas */
        FormDestroy(atlas);
        atlas = NULL;
    }

    /* some tiles */
    bb.destform = screen;

    /* random playfield */
    for (col = 0; col < MAPWIDTH; col += 8)
    {
        i = rand() % 25;
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
    for(i=0; i<64; i++)
    {
        cellmap[0][i] = i;
    }

    /* playfield start position */
    pfx = INT2FIX(0);
    pfdir = 256;

    if (argc == 1)
    {
        /* initial screen fill */
        fillscreen(FIX2INT(pfx), 0);
        fillscreen(FIX2INT(pfx), 512);
    }

    oldrightcol = 0;

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

        /* decide which page based on even/odd framebuffer window */
        page.y = fbx & 1 ? 512 : 0;

        if (argc == 1) SetViewport(&page);

        /* wait for frame flip */
        waiting = 0;
        currpage = framenum;
        while (currpage == framenum)
            waiting++;
        currpage = framenum;

        /* infoline */
        sprintf(infoline, "framenum(%4.4x) %c pfx(%d) cellx(%d) fbx(%d)", framenum, ((framenum & 15) == 0) ? '*' : ' ', pixx, cellx, fbx);

        if (pfdir >= 0)
        {
            /* has leading edge column changed? render a new column */
            col = (page.x + 640) >> 4;
            if (col != oldrightcol)
            {
                oldrightcol = col;

                /* fill new right column just out of view */
                bb.rule = bbS;
                fillcol(col << 4, page.y, cellx + 40);

                /* fill on next page too */
                col = (page.x - 1) >> 4;
                if (col >= 0)
                {
                    /* cellx is for x==0, we are 1 cell before */
                    fillcol(((16+col) << 4), page.y ^ 512, cellx + 40 - 1);

                    /* the region that overlaps far left and far right scroll window position also needs filling */
                    if (col < 16)
                    {
                        fillcol((col << 4), page.y ^ 512, cellx + 24 - 1);
                    }
                }
            }
        }
        else /* reverse direction */
        {
            /* has leading edge column changed? render a new column */
            col = (page.x - 1) >> 4;
            if (col != oldrightcol)
            {
                oldrightcol = col;

                /* fill new right column just out of view */
                bb.rule = bbS;
                fillcol(col << 4, page.y, cellx - 1);

                /* fill on next page too */
                col = (page.x + 40) >> 4;
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

        /* animated playfield at 3.25Hz */
        if ((framenum & 15) == 0)
        {
            /* TODO: need a way of culling animated sprites based on current view window */
            if (pixx < 100)
            {
                animated_pf(64, page.y + 64, rand() & 7);
                animated_pf(128, page.y + 64, rand() & 7);
            }
            if (pixx < 300)
            {
                animated_pf(192, page.y + 64, rand() & 7);
                animated_pf(224, page.y + 128, rand() & 7);
            }
        }

        /* animated sprite */
        /*
            needs to copy playfield cells to offscreen
            make a copy of it
            compose sprite form on top
            copy composed to playfield        
        */


        showinfo(infoline);

        /* scroll */
        pfx += pfdir;
    }
}
