#include <stdio.h>

#define SEEK_SET 0
#define O_RDWR 2

int main(argc, argv)
int argc;
char *argv[];
{
  int pmem_fd,n;
  unsigned short nop_instr = 0x4e71;
  unsigned int shared_page0 = 0x704;
  unsigned int shared_page1 = 0x708;
  unsigned int fpumem = 0x0078a000;   /* NS32081 mapped here */
  unsigned char buffer[16];
  unsigned int *ptr;
  
  pmem_fd = open("/dev/pmem", O_RDWR);
  if (pmem_fd < 0)
  {
    fprintf(stderr,"failed to open /dev/pmem\n");
    exit(1);
  }

  /* custom address to map? */
  if (argc > 1)
  {
    if (sscanf(argv[1], "0x%x", &n) == 1)
    {
      fpumem = n;
    }
  }

  /* sanity check this address is correct; should be 0x67,0xec there */
  lseek(pmem_fd, 0x000fdfc, SEEK_SET);
  n = read(pmem_fd, buffer, 4);
  if (n !=4 || buffer[0] != 0x67 || buffer[1] != 0xec)
  {
    close(pmem_fd);
    fprintf(stderr,"error: did not find expected instruction to patch\n");
    exit(2);
  }
  
  /* overwrite BEQ with NOP so we can do phys(2) */
  lseek(pmem_fd, 0x000fdfc, SEEK_SET);
  write(pmem_fd,  &nop_instr, 2);

  /* phys(2) gets address to map from shared_page0 */
  lseek(pmem_fd, shared_page0, SEEK_SET);
  write(pmem_fd, &fpumem, 4);
  printf("phys(2) will now map %8.8x\n", fpumem);

  /* test it out */
  ptr = phys(2);
  printf("phys(2) =>  %8.8x\n", ptr);

  close(pmem_fd);
  exit(0);
}

