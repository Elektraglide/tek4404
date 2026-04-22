#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(argc, argv)
int argc;
char **argv;
{
  struct stat info;
  char *apath,*search;
  char filepath[256];
    
  search = getenv("PATH");
  printf("%s: ", argv[1]);
   
  apath = strtok(search, ":");
  while (apath)
  {
    strcpy(filepath, apath);
    strcat(filepath, "/");
    strcat(filepath, argv[1]);
      	
    if (stat(filepath, &info) == 0)
    {
      printf("%s ", filepath);
    }

    apath = strtok(NULL,  ":");
  }
  printf("\n");
  
  return 0;
}
 
