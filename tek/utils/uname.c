#include <stdio.h>
#include <fcntl.h>
#include <sys/systat.h>
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

main(argc,argv)
int argc;
char **argv;
{
struct sstat results;
struct sstat *ss;
unsigned char *ptr;
int all;

  all = (argc >1 && argv[1][1] == 'a');

  ptr = systat(&results);
  ss = (struct sstat *)ptr;

  if (all || (argc > 1 && argv[1][1] == 's') || (argc == 1))
	  printf("Uniflex ");

  if (all || (argc > 1 && argv[1][1] == 'n'))
	  printf("%s ", nget_str("my_name"));

  if (all || (argc > 1 && argv[1][1] == 'v'))
	  printf("%d ", ss->ss_ver);

  if (all || (argc > 1 && argv[1][1] == 'c'))
	  printf("%s ", confignames[ss->ss_config]);

  if (all || (argc > 1 && argv[1][1] == 'm'))
	  printf("%s/%s ", ss->ss_hdwr[0] & SS_68020 ? "68020" : "68010", ss->ss_hdwr[0] & SS_68881 ? "68881" : "32081");

  putchar('\n');
}
