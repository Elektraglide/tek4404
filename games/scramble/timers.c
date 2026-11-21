#include <stdio.h>
#include <fcntl.h>
#include <graphics.h>

/* Searching for a fast clock to use for timing.
* 
*  Call EGetTime() to read the am9513 takes around 110us / call
*
*  Mapping AM9513 into our address space and reading directly takes 34us / call
 * NB The donithing++ is to force the required delay between CMD and READ
*
*/

unsigned short nop_instr = 0x4e71;
unsigned int shared_page1 = 0x708;
unsigned int am9513addr = 0x007b8000;
unsigned short* am9513ptr;
int donothing;

/* directly read chained S1,S2 timers */
unsigned int use_direct()
{
    register unsigned short* am9513 = am9513ptr;
    unsigned short t0, t1;

    am9513[1] = 0xffa3; /* latch S1 & S2 */
    am9513[1] = 0xff11; /* load S1 */
    donothing++;
    t0 = am9513[0];     /* read */
    am9513[1] = 0xff12; /* load S2 */
    donothing++;
    t1 = am9513[0];     /* read */
    return (t1 << 16) | t0;
}

main(argc,argv)
int argc;
char **argv;
{
    unsigned int i,t[64];
    int pmem_fd;

    /* approx 110us / call */
    for (i = 0; i < 32; i++)
        t[i] = EGetTime();

    /* just show when reading differs from previous */
    for (i = 1; i < 32; i++)
        if (t[i] - t[i - 1])
            printf("EGetTime[%2d] = %d (%d)\n", i, t[i], t[i] - t[i - 1]);

    /* map kernel variables into our space */
    pmem_fd = open("/dev/pmem", O_RDWR);

    /* overwrite BEQ with NOP so we can use phys(3) */
    lseek(pmem_fd, 0x000fe04, SEEK_SET);
    write(pmem_fd, &nop_instr, 2);

    /* phys(3) gets address to map from shared_page1 */
    lseek(pmem_fd, shared_page1, SEEK_SET);
    write(pmem_fd, &am9513addr, 4);

    /* map into our space */
    am9513ptr = phys(3);

    /* cc: approx 45us / call */
    /* gcc -O: approx 34us / call */
    for (i = 0; i < 64; i++)
        t[i] = use_direct();

    /* just show when reading differs from previous */
    for (i = 1; i < 64; i++)
        if (t[i] - t[i - 1])
            printf("use_direct[%2d] => %d (%d)\n", i, t[i], t[i] - t[i - 1]);
}
