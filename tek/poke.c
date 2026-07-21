#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>

/*

native: cc +v poke.c

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o poke poke.c 

*/

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


int main(argc, argv)
int argc;
char **argv;
{
  FILE *fp;
  int offset,newval;
  int i;
  char c;
    
  fp = fopen(argv[1], "r+");
  if (fp)
  {
      offset = 0;
      if (sscanf(argv[2], "0x%x", &offset) == 1 || sscanf(argv[2], "%d", &offset) == 1)
      {
        if (sscanf(argv[3], "0x%x", &newval) == 1)
        {
          c = (char)newval;
          if (fseek(fp, offset, SEEK_SET) >= 0)
          {
            fprintf(stderr, "modifying byte 0x%8.8x to 0x%2.2x\n", offset, newval);
            fwrite(&c,1,1,fp);
          }
        }
        else
        {
          fprintf(stderr, "failed to read value: %s\n", argv[3]);
        }
      }
      else
      {
        fprintf(stderr, "failed to read offset: %s\n", argv[2]);
      }

      fclose(fp);
  }
  else
  {
    fprintf(stderr,"file not found\n");
  }

  return(0);
}

