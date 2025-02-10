#include <stdio.h>
#include <sys/systat.h>

char *confignames[] =
{
  "Unknown",
  "Momentum_Hawk_32",
  "PIXEL_100/AP",
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

main(argc,argv)
int argc;
char **argv;
{
struct sstat results;
struct sstat *ss;
unsigned char *ptr;
int i;

  ptr = systat(&results);
  ss = (struct sstat *)ptr;
  printf("config:%s ver:%2.2x vendor:%2.2x memsize:%d\n",confignames[ss->ss_config], ss->ss_ver, ss->ss_vendor,ss->ss_memsiz);
}
