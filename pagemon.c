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
    pagemonitor(RETURN_READ_INFORMATION, finfo, sizeof(finfo));

    printf("fault: %8.8x ",  finfo.fault_address);	
    printf("%d\n", finfo.data_size);

	signal(sig, sh_fault);	
}

int bigdata[1024];

main(argc,argv)
int argc;
char **argv;
{
struct sstat results;
struct sstat *ss;
unsigned char *ptr;
int i, pmem;
char *symbols;
int symbolsize;
char bootfile[32],buffer[32];
struct fault_information *fresult;

  memman(1, bigdata, bigdata+255);
  printf("memman %d\n", errno);
  
  /* unknown params; start add, end addr?  */
  fptr = pagemonitor(SET_READ_MONITOR, bigdata, bigdata+64);
  printf("%8.8x\n", fptr);

  signal(SIGRFAULT, sh_fault);
  
  /* do some reads */
  for (i=0; i<1024; i+=128)
    printf("bigdata = %d\n", bigdata[i]);

  printf("cleanup\n");
  pagemonitor(CLEAR_READ_MONITOR, 0, 16);
  
}
