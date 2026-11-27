#include <stdio.h>
#include <fcntl.h>
#include <graphics.h>
#include <sys/sgtty.h>


unsigned short nop_instr = 0x4e71;
unsigned int shared_page1 = 0x708;
unsigned int videocntrladdr = 0x00784000;
short donothing;

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
        short* dst = (short *)(fb + (i << 7) + ((minx >> 3) & 0x7e));
        short* dstend = (short*)(fb + (i << 7) + ((maxx >> 3) & 0x7e));
        while(dst < dstend)
        {
            *dst++ = val;
        }
    }
}

void video_selftest(fb, video)
unsigned char* fb;
unsigned char* video;
{
    register unsigned char* videocntrlptr;
    register unsigned char* dst;
    unsigned char val;
    register short i;
    int count, origval;

    videocntrlptr = video;
    origval = (*videocntrlptr & 0x07) | 0x40 | 0x20 | 0x10;

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
            break;
    }
    if (count)
        selftest_errlog(11, val, count);

    /* clean up */
    *videocntrlptr = origval;
}

void phase0(t, video, samplecount)
unsigned char* t;
unsigned char* video;
int samplecount;
{
    register volatile unsigned char* videocntrlptr2;
    register unsigned char* videocntrlptr;
    register unsigned char* dst;
    register short i;

    dst = t;
    videocntrlptr = video;
    videocntrlptr2 = video;
    i = samplecount;
    /* wait for end of hblank*/
    while ((*videocntrlptr2 & 0x40) == 0);

    do    /* NB constructed to get a dbra loop */
    {
        *dst++ = *videocntrlptr;
    } while (--i != -1);
}

void phase1(t, video, samplecount)
unsigned char* t;
unsigned char* video;
int samplecount;
{
    register volatile unsigned char* videocntrlptr2;
    register unsigned char* videocntrlptr;
    register unsigned char* dst;
    register short i;

    dst = t;
    videocntrlptr = video;
    videocntrlptr2 = video;
    i = samplecount;
    /* wait for end of hblank*/
    while ((*videocntrlptr2 & 0x40) == 0);

    donothing++;

    do    /* NB constructed to get a dbra loop */
    {
        *dst++ = *videocntrlptr;
    } while (--i != -1);
}

void phase2(t, video, samplecount)
unsigned char* t;
unsigned char* video;
int samplecount;
{
    register volatile unsigned char* videocntrlptr2;
    register unsigned char* videocntrlptr;
    register unsigned char* dst;
    register short i;

    dst = t;
    videocntrlptr = video;
    videocntrlptr2 = video;
    i = samplecount;
    /* wait for end of hblank*/
    while ((*videocntrlptr2 & 0x40) == 0);

    donothing++;
    donothing++;

    do    /* NB constructed to get a dbra loop */
    {
        *dst++ = *videocntrlptr;
    } while (--i != -1);
}

void phase3(t, video, samplecount)
unsigned char* t;
unsigned char* video;
int samplecount;
{
    register volatile unsigned char* videocntrlptr2;
    register unsigned char* videocntrlptr;
    register unsigned char* dst;
    register short i;

    dst = t;
    videocntrlptr = video;
    videocntrlptr2 = video;
    i = samplecount;

    /* wait for end of hblank*/
    while ((*videocntrlptr2 & 0x40) == 0);

    donothing++;
    donothing++;
    donothing++;

    do    /* NB constructed to get a dbra loop */
    {
        *dst++ = *videocntrlptr;
    } while (--i != -1);
}

void interleaveresults(waveform, t, samplecount)
unsigned char waveform[5][256];
unsigned char t[4][64];
int samplecount;
{
    register short i;
    register unsigned char* dst;

#if 0
    /* build a string we can scrub through */
    dst = waveform[0];
    for (i = 0; i < samplecount; i++)
    {
        *dst++ = t[0][i] & 0x80 ? 'D' : '-';
        *dst++ = t[1][i] & 0x80 ? 'D' : '-';
        *dst++ = t[2][i] & 0x80 ? 'D' : '-';
        *dst++ = t[3][i] & 0x80 ? 'D' : '-';
    }
    *dst++ = '\0';

    dst = waveform[1];
    for (i = 0; i < samplecount; i++)
    {
        *dst++ = t[0][i] & 0x40 ? 'H' : '-';
        *dst++ = t[1][i] & 0x40 ? 'H' : '-';
        *dst++ = t[2][i] & 0x40 ? 'H' : '-';
        *dst++ = t[3][i] & 0x40 ? 'H' : '-';
    }
    *dst++ = '\0';

    dst = waveform[2];
    for (i = 0; i < samplecount; i++)
    {
        *dst++ = t[0][i] & 0x20 ? 'V' : '-';
        *dst++ = t[1][i] & 0x20 ? 'V' : '-';
        *dst++ = t[2][i] & 0x20 ? 'V' : '-';
        *dst++ = t[3][i] & 0x20 ? 'V' : '-';
    }
    *dst++ = '\0';

    dst = waveform[3];
    for (i = 0; i < samplecount; i++)
    {
        *dst++ = t[0][i] & 0x10 ? 'A' : '-';
        *dst++ = t[0][i] & 0x10 ? 'A' : '-';
        *dst++ = t[0][i] & 0x10 ? 'A' : '-';
        *dst++ = t[0][i] & 0x10 ? 'A' : '-';
    }
    *dst++ = '\0';

    dst = waveform[4];
    for (i = 0; i < samplecount; i++)
    {
        *dst++ = t[0][i] & 0x08 ? 'S' : '-';
        *dst++ = t[1][i] & 0x08 ? 'S' : '-';
        *dst++ = t[2][i] & 0x08 ? 'S' : '-';
        *dst++ = t[3][i] & 0x08 ? 'S' : '-';
    }
#else
    /* compare phase results for H */
    dst = waveform[0];
    for (i = 0; i < samplecount * 4; i++)
    {
        *dst++ = t[0][i & 63] & 0x40 ? 'H' : '-';
    }
    *dst++ = '\0';

    dst = waveform[1];
    for (i = 0; i < samplecount*4; i++)
    {
        *dst++ = t[1][i & 63] & 0x40 ? 'H' : '-';
    }
    *dst++ = '\0';

    dst = waveform[2];
    for (i = 0; i < samplecount*4; i++)
    {
        *dst++ = t[2][i & 63] & 0x40 ? 'H' : '-';
    }
    *dst++ = '\0';

    dst = waveform[3];
    for (i = 0; i < samplecount*4; i++)
    {
        *dst++ = t[3][i & 63] & 0x40 ? 'H' : '-';
    }
    *dst++ = '\0';
#endif

    /* SoundRdy is always ON, so make a ruler */
    dst = waveform[4];
    for (i = 0; i < samplecount * 4; i++)
        *dst++ = '0' + (i & 7);
    *dst++ = '\0';

}

main(argc,argv)
int argc;
char **argv;
{
    register short i;
    register unsigned char *dst;
    register unsigned short* fb;

    volatile unsigned char* videocntrlptr;
    unsigned char t[4][64];
    unsigned char waveform[5][256];
    int pmem_fd, start,stop, samplecount;

    int tty_fd;
    struct sgttyb orig_term_settings,new_term_settings;

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

    /* video ram too */
    fb = (unsigned short*)phys(1);

    video_selftest(fb, videocntrlptr);

    /* how many samples (times 4) */
    samplecount = 64;

    start = EGetTime();

    /* We cannot sample fast enough, so we repeat 4 times with a phase offset 
      we then splice the 4 sample runs together    
    */

    /* phase 0 */
    phase0(t[0], videocntrlptr, samplecount);

    /* phase 1 */
    phase1(t[1], videocntrlptr, samplecount);

    /* phase 2 */
    phase2(t[2], videocntrlptr, samplecount);

    /* phase 3 */
    phase3(t[3], videocntrlptr, samplecount);

    stop = EGetTime();
    
    printf("recorded %d samples in %d ms\n", samplecount, stop - start);

    interleaveresults(waveform, t, samplecount);

    /* read keyboard */
    tty_fd = open(ttyname(fileno(stdin)), O_RDWR);
    gtty(tty_fd, &orig_term_settings);
    new_term_settings = orig_term_settings;
    new_term_settings.sg_flag |= CBREAK ;
    stty(tty_fd, &new_term_settings);

    start = 0;
    while (1)
    {
        printf("%.75s\n", waveform[0] + start);
        printf("%.75s\n", waveform[1] + start);
        printf("%.75s\n", waveform[2] + start);
        printf("%.75s\n", waveform[3] + start);
        printf("%.75s\n", waveform[4] + start);

        i = getchar();

        if (i == 'a' && start > 0)
            start--;
        if (i == 'd' && start < 256-80)
            start++;

        printf("\033[5A");
    }

    stty(tty_fd, &orig_term_settings);
}
