#include <stdio.h>
#include <fcntl.h>

int sndfd;
unsigned char sb[32];

noise(f,v,l)
int f,v,l;
{
	sb[0] = 2;
	sb[1] = 0x80 | 0x60 | (f & 7);
    sb[2] = 0x80 | 0x70 | (v & 15);
    sb[3] = l;
	write(sndfd, sb, 4);
}

voice(voice,f,v,l)
int voice,f,v,l;
{
	/* 10-bit frequency */
	sb[0] = 3;
	sb[1] = 0x80 | (voice<<5) | 0x00 | (f & 15);
	sb[2] = (f >> 4) & 63;
    /* 4-bit volume */
	sb[3] = 0x80 | (voice<<5) | 0x10 | (v & 15);
	sb[4] = l;
	write(sndfd, sb, 5);
}

main()
{
  char aline[80];
  int f,v,l;
  int i;
      
	sndfd = open("/dev/sound", O_WRONLY);

	/* tempo 0.1Hz */
	sb[0] = 0;
	sb[1] = 0;
	sb[2] = 6;
    write(sndfd, sb, 3);

printf("voice0\n");
	voice(0,  200, 0,1);
	voice(0,  400, 8,2);
	voice(0,  800, 12,2);
	voice(0,  999, 15,50);

printf("noise\n");
	noise(2, 0, 1);
	noise(2, 4, 2);
	noise(2, 6, 2);
	noise(2, 8, 2);
	noise(2, 10, 2);
	noise(2, 12, 2);
	noise(2, 15,0);


  while(1)
  {
    gets(aline);

	/* only way of cancelling a sound! */
	close(sndfd);
	sndfd = open("/dev/sound", O_WRONLY);
    
    if (sscanf(aline, "%d %d %d\n", &f,&v,&l) == 3)
    {
      voice(2, f-1,v,0);
      voice(1, f+3,v,0);
      voice(0, f,v,l);

	  while (v < 15)
	  {
	  	v++;
	    voice(2, f-1,v,0);
        voice(1, f+3,v,0);
        voice(0, f,v,8);
	  }      
    }
    else
    if (sscanf(aline, "%d %d\n", &f,&v) == 2)
    {
      if (f < 0)
      {
	      voice(2, -f,v,0);
	      f = 3;
	  }
      noise(f,v,30);
	  while (v < 15)
	  {
	  	v++;
        noise(f,v,2);
	  }      
    }	
  
  }

	
	close(sndfd);
}

