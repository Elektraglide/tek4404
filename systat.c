#include <stdio.h>
#include <fcntl.h>
#include <sys/systat.h>
#include <sys/page_monitor.h>
#include <signal.h>

#include "kernutils.h"

char *confignames[] =
{
  "Unknown",
  "Momentum_Hawk_32",
  "PIXEL_100/AP",
  "Unknown",
  "Tektronix_4404",
  "TSC_SBC",
  "Tektronix_CBI",
  "Force_CPU-3V",
  "VME/10",
  "Ironics_IV1600",
  "Gimix_GMX-20",
  "NCR_Tower",
  "Gimix_MICRO-20",
  "SWTPc_SB68K",
  "VME-1000",
  "Tektronix_4406",
  "Tektronix_4405",
  "SWTPc_VME_System",
  "Gimix_MICRO-20",
  "Standard_VM"
};

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

  /* unknown params; start add, end addr?  */
  fptr = pagemonitor(SET_READ_MONITOR,0, 32);
  printf("%8.8x\n", fptr);

  signal(SIGRFAULT, sh_fault);
  
  /* do some reads */
  for (i=0; i<1024; i+=128)
    printf("bigdata = %d\n", bigdata[i]);


  kernbootfile(buffer);
  sprintf(bootfile, "/%s", buffer);

	pmem = open("/dev/pmem", O_RDWR);
	if (pmem < 0)
	{
		fprintf(stderr, "%s: failed to open /dev/pmem\n", argv[0]);
		exit(1);
	}

  symbols = getsymbols(bootfile, &symbolsize);

  printf("system boot file: %s\n", bootfile);
  printf("system boot time:  %s\n", kernboottime(symbols, symbolsize, pmem));

  ptr = systat(&results);
  ss = (struct sstat *)ptr;
  printf("config:%s ver:%2.2x vendor:%2.2x\n",confignames[ss->ss_config], ss->ss_ver, ss->ss_vendor);
  printf("memsize:%dk\n", ss->ss_memsiz / 1024);

  printf("protected memory: %s\n", ss->ss_hdwr[0] & SS_PROT ? "YES" : "NO");
  printf("virtual memory: %s\n", ss->ss_hdwr[0] & SS_VM ? "YES" : "NO");
  printf("long filenames: %s\n", ss->ss_hdwr[0] & SS_LNAM ? "YES" : "NO");
  printf("68020: %s\n", ss->ss_hdwr[0] & SS_68020 ? "YES" : "NO");
  printf("68881: %s\n", ss->ss_hdwr[0] & SS_68881 ? "YES" : "NO");


  printf("usedmem: %dk\n", getkint32(symbols, symbolsize, "usedmem", pmem) / 1024);
  printf("68000: %d\n", getkconstant(symbols, symbolsize, "IS_68000", pmem));
  printf("68010: %d\n", getkconstant(symbols, symbolsize, "IS_68010", pmem));
  printf("68020: %d\n", getkconstant(symbols, symbolsize, "IS_68020", pmem));

  printf("RAM_DISK: %d\n", getkconstant(symbols, symbolsize, "RAM_DISK_OPTION", pmem));
  printf("REAL_TIME: %d\n", getkconstant(symbols, symbolsize, "REAL_TIME_OPTION", pmem));
  printf("CLOCK Hz: %d\n", getkconstant(symbols, symbolsize, "CLOCKS_PER_SECOND", pmem));

  printf("cleanup\n");
  pagemonitor(CLEAR_READ_MONITOR, 0, 16);
  
}
