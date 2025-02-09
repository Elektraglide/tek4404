#include <stdio.h>
#include <sys/systat.h>

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
  printf("config:%2.2x ver:%2.2x vendor:%2.2x memsize:%d\n",ss->ss_config, ss->ss_ver, ss->ss_vendor,ss->ss_memsiz);

}
