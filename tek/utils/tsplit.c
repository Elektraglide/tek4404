#include <stdio.h>

main(argc,argv)
int argc;
char **argv;
{
  if (argv[1][0] == 't')
    printf("\033[1;16r\033[16H");
  else
  if (argv[1][0] == 'b')
    printf("\033[17;33r\033[33H");
  else
    printf("\033[1;33r\033[33H");
  
  fflush(stdout);  
}
