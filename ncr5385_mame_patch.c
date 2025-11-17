#include <stdio.h>

#define SEEK_SET 0
#define O_RDWR 2

int main(argc, argv)
int argc;
char *argv[];
{
  int pmem_fd,n;
  unsigned char beq_instr = 0x60;
  unsigned char buffer[16];
  unsigned int *ptr;
  
  pmem_fd = open("/dev/pmem", O_RDWR);
  if (pmem_fd < 0)
  {
    fprintf(stderr,"failed to open /dev/pmem\n");
    exit(1);
  }

  // see mame/src/machine/device/ncr5385.cpp for more details
  printf("MAME ncr5385 emulation causes a hang for tek4404 due to a late arrival of scsi IRQ3\n");
  printf("A solution is to patch the scsi driver so an early IRQ does not hang the machine.\n");

  /* sanity check this address is correct; should be 0x66 there */
  lseek(pmem_fd, 0x1285e, SEEK_SET);
  n = read(pmem_fd, buffer, 1);
  if (n !=1 || buffer[0] != 0x66)
  {
    close(pmem_fd);
    fprintf(stderr,"Did not find expected instruction to patch\n");
    exit(2);
  }
  
  /* overwrite BNE with BEQ so we always call SCSI_wait_continue */
  lseek(pmem_fd, 0x1285e, SEEK_SET);
  write(pmem_fd,  &beq_instr, 1);
  close(pmem_fd);

  printf("MAME ncr5385 emulation should now not hang tek4404\n");

  exit(0);
}

