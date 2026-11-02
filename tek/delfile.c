#include <stdio.h>

/* if you create a file that looks like a cmd option, its impossible to
delete it because the shell interprets the fielname as a option.
This just deletes the file.
*/
main(argc, argv)
int argc;
char **argv;
{
  printf("deleting '%s'\n", argv[1]);	
  unlink(argv[1]);
}
