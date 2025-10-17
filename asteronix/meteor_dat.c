#include <graphics.h>
#include "asteronix.h"

#define MEDHEIGHT 22
#define BIGHEIGHT 41


short medium_meteor[] = {
0xFC0F, 0xFFFF, 0xFA04, 0x1FFF, 0xF502, 0x0FFF,
0xF404, 0x07FF, 0xFA00, 0x03FF, 0xD540, 0x03FF,
0xBAA0, 0x03FF, 0xFE40, 0x03FF, 0xBE20, 0x27FF,
0xDD00, 0x4FFF, 0xFEB0, 0x27FF, 0xF740, 0x47FF,
0xFEA0, 0x87FF, 0xFD48, 0x07FF, 0xEFA8, 0x0FFF,
0xDD50, 0x1FFF, 0xEFA8, 0x3FFF, 0xFFF5, 0x7FFF,
0xFAEA, 0xFFFF, 0xFF5F, 0xFFFF, 0xFFFF, 0xFFFF,
0xFFFF, 0xFFFF
};

short big_meteor[] = {
0xffff, 0x83ff, 0xffff, 0xffff, 0x003f, 0xffff,
0xfffc, 0x000f, 0xffff, 0xfff0, 0x0003, 0xffff,
0xffc0, 0x0001, 0xffff, 0xff00, 0x0000, 0x0fff,
0xfe80, 0x0000, 0x03ff, 0xfd40, 0x0000, 0x01ff,
0xfa80, 0x0000, 0x00ff, 0xfd00, 0x0005, 0x00ff,
0xfa80, 0x0008, 0x007f, 0xf404, 0x0014, 0x007f,
0xea04, 0x0008, 0x007f, 0xd508, 0x0010, 0x007f,
0xbaa0, 0x0000, 0x007f, 0x5540, 0x0000, 0x013f,
0xe880, 0x0000, 0x021f, 0x7440, 0x0000, 0x051f,
0xba80, 0xa000, 0x029f, 0x7741, 0x0000, 0x055f,
0xafaa, 0x0000, 0x00bf, 0x7544, 0x0010, 0x005f,
0xeeaa, 0x08a8, 0x009f, 0x7575, 0x5140, 0x011f,
0xeefa, 0xa0a0, 0x0a3f, 0x7d54, 0x5500, 0x147f,
0xeaea, 0x2a80, 0x2a7f, 0x7d54, 0x0540, 0x1dff,
0xfaea, 0x00a0, 0x3b7f, 0xfbda, 0x0055, 0x7f7f,
0xffb5, 0x680a, 0xaaff, 0xffda, 0xb415, 0x75ff,
0xffad, 0x6aaa, 0xabff, 0xf7d7, 0xd5d5, 0x57ff,
0xfaee, 0xaab7, 0xafff, 0xfddd, 0x5e57, 0x7fff,
0xfefe, 0xaebb, 0xffff, 0xffff, 0xd7ff, 0xffff,
0xffab, 0xeeeb, 0xffff, 0xffd7, 0x7f57, 0xffff,
0xfffe, 0xfeff, 0xffff, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
0xffff, 0xffff, 0xffff
};

short medium_shadowmask[] = {
0x03f0, 0x0000, 0x07ff, 0xe000, 0x0fff, 0xf000,
0x0fff, 0xf800, 0x1fff, 0xfc00, 0x3fff, 0xfc00,
0x7fff, 0xfc00, 0xffff, 0xfc00, 0xffff, 0xf800,
0xffff, 0xf000, 0xffff, 0xfc00, 0xffff, 0xfc00,
0x7fff, 0xfc00, 0x7fff, 0xfc00, 0x7fff, 0xf800,
0x3fff, 0xf000, 0x3fff, 0xe000, 0x3fff, 0xc000,
0x3fff, 0x8000, 0x1fff, 0x0000, 0x03e0, 0x0000,
0x0000, 0x0000
};

short big_shadowmask[] = {
0x0000, 0x7c00, 0x0000, 0x0000, 0xffc0, 0x0000,
0x0003, 0xfff0, 0x0000, 0x000f, 0xfffc, 0x0000,
0x003f, 0xfffe, 0x0000, 0x00ff, 0xffff, 0xf000,
0x01ff, 0xffff, 0xfc00, 0x03ff, 0xffff, 0xfe00,
0x07ff, 0xffff, 0xff00, 0x07ff, 0xffff, 0xff00,
0x0fff, 0xffff, 0xff80, 0x0fff, 0xffff, 0xff80,
0x1fff, 0xffff, 0xff80, 0x3fff, 0xffff, 0xff80,
0x7fff, 0xffff, 0xff80, 0xffff, 0xffff, 0xffc0,
0xffff, 0xffff, 0xffe0, 0xffff, 0xffff, 0xffe0,
0xffff, 0xffff, 0xffe0, 0xffff, 0xffff, 0xffe0,
0xffff, 0xffff, 0xffe0, 0xffff, 0xffff, 0xffe0,
0xffff, 0xffff, 0xffe0, 0xffff, 0xffff, 0xffe0,
0xffff, 0xffff, 0xffc0, 0xffff, 0xffff, 0xff80,
0xffff, 0xffff, 0xff80, 0xffff, 0xffff, 0xff80,
0x7fff, 0xffff, 0xff80, 0x7fff, 0xffff, 0xff80,
0x7fff, 0xffff, 0xff00, 0x3fff, 0xffff, 0xfe00,
0x1fff, 0xffff, 0xfc00, 0x0fff, 0xffff, 0xf800,
0x07ff, 0xffff, 0xf000, 0x03ff, 0xffff, 0xe000,
0x01ff, 0xffff, 0x8000, 0x00ff, 0xffff, 0x0000,
0x007f, 0xffff, 0x0000, 0x003f, 0xfffe, 0x0000,
0x000f, 0xfff8, 0x0000
};


struct FORM big, big_m;
struct FORM medium, medium_m;
struct FORM* bigshift[16];
struct FORM* big_mshift[16];
struct FORM* mediumshift[16];
struct FORM* medium_mshift[16];

void makeshifted(src, dstarr, rule)
struct FORM* src;
struct FORM** dstarr;
int rule;
{
    int i, len;
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
    for (i = 0; i < 16; i++)
    {
        dstarr[i] = FormCreate(src->w + 16, src->h);

        memset(dstarr[i]->addr, 0xff, len);

        bb.destform = dstarr[i];
        bb.destrect.x = i;
        bb.destrect.y = 0;
        bb.destrect.w = src->w;
        bb.destrect.h = src->h;
        BitBlt(&bb);
    }
}

/* build forms from embedded data borrowed from Megaroids */
void makemeteorforms()
{
	big.addr = big_meteor;
    big.w = 48;
    big.h = BIGHEIGHT;
    big.offsetw = big.offseth = 0;
    big.inc = 6;

	big_m.addr = big_shadowmask;
    big_m.w = 48;
    big_m.h = BIGHEIGHT;
    big_m.offsetw = big_m.offseth = 0;
    big_m.inc = 6;

	medium.addr = medium_meteor;
    medium.w = 32;
    medium.h = MEDHEIGHT;
    medium.offsetw = medium.offseth = 0;
    medium.inc = 4;

	medium_m.addr = medium_shadowmask;
    medium_m.w = 32;
    medium_m.h = MEDHEIGHT;
    medium_m.offsetw = medium_m.offseth = 0;
    medium_m.inc = 4;

    /* build shifted versions */
    makeshifted(&big, bigshift, bbnS);
    makeshifted(&big_m, big_mshift, bbnS);
    makeshifted(&medium, mediumshift, bbnS);
    makeshifted(&medium_m, medium_mshift, bbnS);
}


void blitfast(splash, halftone)
struct FORM* splash;
struct FORM* halftone;
{
    register unsigned long* dst, * src;
    register unsigned short* mask;
    register short h = splash->h;

    src = splash->addr;
    mask = halftone->addr;

    /* draw on current page */
    dst = ((char*)screen->addr) + (page.y << 7);
    do
    {
        /* mask 640px scanline */
        register unsigned long amask = mask[h & 15];
        amask |= amask << 16;

        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask); *dst++ = (*src++ & amask);
        dst += 12;
    } while (--h > 0);
}

void restoreunder(asprite, w, h)
sprite* asprite;
int w, h;
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

void blitclear64(sx, sy, sh)
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

void bigblitflash(sx, sy)
int sx, sy;
{
    bb.srcform = &big_m;
    bb.destform = screen;
    bb.destrect.x = sx;
    bb.destrect.y = sy + page.y;
    bb.destrect.w = big.w;
    bb.destrect.h = big.h;
    bb.rule = bbSorD;
    BitBlt(&bb);
}

void bigblitmasked(sx, sy, shift)
int sx, sy;
struct FORM** shift;
{
    register unsigned long* dst, * src, * mask;
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
        dst[0] = (dst[0] & *mask++) | (*src++);
        dst[1] = (dst[1] & *mask++) | (*src++);
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
sprite* asprite;
{
    register unsigned long* dst, * src, * mask;
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

    dst = ((char*)screen->addr) + (sy << 7) + ((sx & -16) >> 3);
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
    register unsigned short* dst, * src, * mask;
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
sprite* asprite;
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
