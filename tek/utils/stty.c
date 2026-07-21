#include <stdio.h>
#include <sys/sgtty.h>

int main(argc, argv)
int argc;
char **argv;
{
struct sgttyb settings;

  gtty(0, &settings);
  
 printf("sg_flag: ");
 if (settings.sg_flag & RAW) printf("raw ");	
 if (settings.sg_flag & XTABS) printf("xtabs ");	
 if (settings.sg_flag & CRMOD) printf("crmod ");	
 if (settings.sg_flag & CBREAK) printf("cbreak ");	
 if (settings.sg_flag & CNTRL) printf("cntrl ");	
 printf("\n");

 printf("sg_prot: ");
 if (settings.sg_prot & ESC) printf("esc ");	
 if (settings.sg_prot & OXON) printf("oxon ");	
 if (settings.sg_prot & ANY) printf("any ");	
 if (settings.sg_prot & TRANS) printf("tran ");	
 if (settings.sg_prot & IXON) printf("ixon ");	
 printf("\n");

  printf("erase: 0x%2.2x\n", settings.sg_erase);
  printf("kill: 0x%2.2x\n", settings.sg_kill);
  
  printf("sg_speed: 0x%2.2x\n", settings.sg_speed);

  gtty(1, &settings);
  printf("baud: 0x%2.2x\n", settings.sg_prot & BAUD_RATE);

  
   return 0;
 
}
