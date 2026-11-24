#include <stdio.h>
#include <fcntl.h>
#include <graphics.h>


unsigned short nop_instr = 0x4e71;
unsigned int shared_page1 = 0x708;
unsigned int videocntrladdr = 0x00784000;
int donothing;


main(argc,argv)
int argc;
char **argv;
{
    register unsigned short* videocntrlptr;
    register unsigned short *dst;
    unsigned short t[512];
    register unsigned short i;
    int pmem_fd,start,stop;

    /* map kernel variables into our space */
    pmem_fd = open("/dev/pmem", O_RDWR);

    /* overwrite BEQ with NOP so we can use phys(3) */
    lseek(pmem_fd, 0x000fe04, SEEK_SET);
    write(pmem_fd, &nop_instr, 2);

    /* phys(3) gets address to map from shared_page1 */
    lseek(pmem_fd, shared_page1, SEEK_SET);
    write(pmem_fd, &videocntrladdr, 4);

    /* map into our space */
    videocntrlptr = phys(3);

    dst = t;
    start = EGetTime();
    for (i = 0; i < 512; i++)
        *dst++ = *videocntrlptr;
    stop = EGetTime();
    
    /* just show when reading differs from previous */
    printf("recorded %d samples in %d ms\n", 512, stop - start);
    printf("%d\n", start);
    for (i = 0; i < 512; i++)
     printf("%2d: %c%c%c%c%c\n", i, 
     	t[i] & 0x8000 ? 'D' : '-',
     	t[i] & 0x4000 ? 'H' : '-',
     	t[i] & 0x2000 ? 'V' : '-',
     	t[i] & 0x1000 ? 'A' : '-',
     	t[i] & 0x0800 ? 'S' : '-');
         
    printf("%d\n", stop);
}
