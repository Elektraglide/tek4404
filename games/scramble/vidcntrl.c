#include <stdio.h>
#include <fcntl.h>
#include <graphics.h>


unsigned short nop_instr = 0x4e71;
unsigned int shared_page1 = 0x708;
unsigned int videocntrladdr = 0x00784000;
int donothing;

selftest_errlog(testid, val,count)
int testid;
int val,count;
{
    printf("Self Test Error-%s [%02d, %04X] loop=%d\n", "Display Control", testid, val, count);
}

fill_video_rect(fb, minx, maxx, miny, maxy, val)
char* fb;
short minx, maxx, miny, maxy, val;
{
    short i;

    for (i = miny; i < maxy; i++)
    {
        short* dst = fb + (i << 7) + ((minx >> 3) & 0x7e);
        short* dstend = fb + (i << 7) + ((maxx >> 3) & 0x7e);
        while(dst < dstend)
        {
            *dst++ = val;
        }
    }
}

main(argc,argv)
int argc;
char **argv;
{
    register unsigned char* videocntrlptr;
    register unsigned char *dst;
    register unsigned short* fb;
    unsigned char val, t[512];
    register short i;
    int pmem_fd,start,stop, count, origval, samplecount;

    /* map kernel variables into our space */
    pmem_fd = open("/dev/pmem", O_RDWR);

    /* overwrite BEQ with NOP so we can use phys(3) */
    lseek(pmem_fd, 0x000fe04, SEEK_SET);
    write(pmem_fd, &nop_instr, 2);

    /* phys(3) gets address to map from shared_page1 */
    lseek(pmem_fd, shared_page1, SEEK_SET);
    write(pmem_fd, &videocntrladdr, 4);

    /* map into our space */
    videocntrlptr = (unsigned char *)phys(3);
    origval = (*videocntrlptr & 0x07) | 0x40 | 0x20 | 0x10;

    /* video ram too */
    fb = (unsigned short*)phys(1);

    /* test 6

      During display area, if VideoData off, read again and expect on
    */
    count = 0x20000;

    /* fills framebuffer with 0xffff */
    fill_video_rect(fb, 0, 1024, 0, 1024, 0xffff);

    *videocntrlptr = 0x0;
    while (--count)
    {
        val = *videocntrlptr;

        /* skip if vblank */
        if (val & 0x20)
            continue;

        /* skip if NOT hblank */
        if ((val & 0x40))
            continue;

        if ((val & 0x80) == 0)
            continue;
    }
    if (count)
        selftest_errlog(6, val, count);


    /* test 7
    
      During display area, if VideoData off, read again and expect on
    */
    count = 0x20000;
    *videocntrlptr = 0x20;  /* ScreenEnable */
    while (--count)
    {
        val = *videocntrlptr;

        /* skip if vblank */
        if (val & 0x20)
            continue;

        /* skip if hblank */
        if ((val & 0x40) == 0)
            continue;

        if (val & 0x80)
            continue;

        /* re-read videocntrl IFF previous VideoData==0 */
        val = *videocntrlptr;
        if ((val & 0x80) == 0)
            break;
    }
    if (count)
        selftest_errlog(7, val, count);


    /* test 8 
    
      During hblank, expect VideoData
    */
    count = 0x20000;
    *videocntrlptr = 0x30;  /* ScreenEnable ScreenInverse */
    while (--count)
    {
        val = *videocntrlptr;

        /* skip if vblank */
        if (val & 0x20)
            continue;

        /* skip if NOT hblank */
        if (val & 0x40)
            continue;

        if (val & 0x80)
            break;
    }
    if (count)
        selftest_errlog(8, val, count);


    /* test 9 
    
      During display area, if VideoData off, read again and expect on
    */
    count = 0x20000;

    /* fills framebuffer with 0 */
    fill_video_rect(fb, 0, 1024, 0, 1024, 0);

    while (--count)
    {
        val = *videocntrlptr;

        /* skip if vblank */
        if (val & 0x20)
            continue;

        /* skip if hblank */
        if ((val & 0x40) == 0)
            continue;

        if (val & 0x80)
            continue;

        /* re-read videocntrl */
        val = *videocntrlptr;
        if ((val & 0x80) == 0)
            break;
    }
    if (count)
        selftest_errlog(9, val, count);

    /* test 10 
    
     Panning bits read/write
    */
    for (i = 0; i < 8; i++)
    {
        *videocntrlptr = i;
        if ((*videocntrlptr & 7) != i)
        {
            selftest_errlog(10, i << 8, count);
            break;
        }

    }

    /* test 11 
    
     During display area, ASSERT(VideoData == 0) when setting VideoPan > 34751
    */
    count = 0x20000;
    *videocntrlptr = 0x20;  /* Screen Enable */
    while(--count)
    {
        val = *videocntrlptr;

        /* skip if vblank */
        if (val & 0x20)
            continue;

        /* skip if hblank */
        if ((val & 0x40) == 0)
            continue;

        if (val & 0x80)
            break;
    }
    if (count)
        selftest_errlog(11, val, count);

    /* clean up */
    *videocntrlptr = origval;

    /* no args => skip dumping */
    if (argc == 1)
        exit(0);

    /* how many samples */
    samplecount = atoi(argv[1]);
    if (samplecount < 16) samplecount = 16;
    if (samplecount > 512) samplecount = 512;

    dst = t;
    start = EGetTime();
    i = samplecount;
    while(--i >= 0)  /* for gcc1.42 faster loop; but no dbra... */
    {
        *dst++ = *videocntrlptr;
    }
    stop = EGetTime();
    
    /* just show when reading differs from previous */
    printf("recorded %d samples in %d ms\n", samplecount, stop - start);
    printf("%d\n", start);
    for (i = 0; i < samplecount; i++)
     printf("%2d: %c%c%c%c%c\n", i, 
     	t[i] & 0x80 ? 'D' : '-',
     	t[i] & 0x40 ? 'H' : '-',
     	t[i] & 0x20 ? 'V' : '-',
     	t[i] & 0x10 ? 'A' : '-',
     	t[i] & 0x08 ? 'S' : '-');
         
    printf("%d\n", stop);
}
