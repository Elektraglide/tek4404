#include <stdio.h>
#include <fcntl.h>
#include <sys/systat.h>
#include <sys/page_monitor.h>
#include <signal.h>


struct fault_information finfo;
struct fault_information *fptr;

void sh_fault(sig)
int sig;
{
	/* unknown params; buffer, 10 (sizeof fault_information) */
    pagemonitor(RETURN_READ_INFORMATION, &finfo, sizeof(finfo));

    printf("fault: %8.8x ",  finfo.fault_address);	
    printf("%d\n", finfo.data_size);

	signal(sig, sh_fault);	
}

int bigdata[1024] = { 1,2,3,4 };

main(argc,argv)
int argc;
char **argv;
{
struct sstat results;
struct sstat *ss;
unsigned char *ptr;
int i;

signal(SIGRFAULT, sh_fault);

#if 0
  memman(1, bigdata, bigdata+255);
  printf("memman %d\n", errno);
#endif

  /* unknown params; start add, end addr?  */
  ptr = pagemonitor(SET_READ_MONITOR, bigdata, bigdata+1023);
  printf("%8.8x\n", fptr);

  /*
    Gets stuck constantly reading bigdata[0] and each time recording the fault_information correctly.  Its like the PC 
    does not get incremented.  Is this broken pagemonitor() code or broken MMU emulation?
  */


  /* do some reads */
  for (i=0; i<1024; i+=128)
    printf("bigdata = %d\n", bigdata[i]);

  printf("cleanup\n");
  pagemonitor(CLEAR_READ_MONITOR, 0, 1023);
  
}
