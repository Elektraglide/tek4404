#include <stdio.h>
#include <signal.h>
#include <graphics.h>

#ifdef __clang__

#include <stdarg.h>
#include "uniflexshim.h"

#endif

unsigned char shifted[128] =
{
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\"',
  '\0','\0','\0','\0', '<', '_', '>', '?',
   ')', '!', '@', '#', '$', '%', '^', '&',
   '*', '(','\0', ':','\0', '+','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0', '{', '`', '}','\0','\0',
  '\0', 'A', 'B','C','D','E','F','G',
  'H','I','J','K','L','M','N','O',
  'P','Q','R','S','T','U','V','W',
  'X','Y','Z','~','\0','\0','\0',0x7f
};

unsigned char keymap[256];
unsigned long timestamp = 0;

unsigned char *getkeymap()
{
  return keymap;
}

int readkeyboardevents(buffer, maxlen)
char *buffer;
int maxlen;
{
  int ecount;
  union EVENTUNION ev;
  unsigned long timelo,timehi;
  int total = 0;
  unsigned char ch;
  
  ecount = EGetCount();
  while (ecount-- > 0 && total < maxlen)
  {
    ev.evalue = EGetNext();
    
    switch(ev.estruct.etype)
    {
      case E_ABSTIME:
        timehi = EGetNext();
        timelo = EGetNext();
        ecount -= 2;
        timestamp = (timehi << 16) | timelo;
        break;
      case E_DELTATIME:
        timestamp += ev.estruct.eparam;
        break;

      case E_PRESS:
        keymap[ev.estruct.eparam] = 1;
        /* need to syntheise Ctrl / Shift ? */
        ch = ev.estruct.eparam;
        /* filter out modifier keys */
        if (ch < 0x80)
        {
            if (keymap[0x88] || keymap[0x89])   /* shifted */
            {
                if (shifted[ch])
                    ch = shifted[ch];
            }

            if (keymap[0x8a])                   /* control */
            {
                ch -= 0x60;
            }

            buffer[total++] = ch;
        }
        break;
        
      case E_RELEASE:
        keymap[ev.estruct.eparam] = 0;
        break;
        
      case E_XMOUSE:
        break;
      case E_YMOUSE:
        break;
        
    }
  }

  return total;
}

#ifdef TESTING
void sh_input(sig)
int sig;
{
  printf("event\n");	

  signal(SIGEVT, sh_input);
  ESetSignal();
}

int main(argc,argv)
int argc;
char **argv;
{
  int loop = 32;
  int n;
  unsigned char input[128];

  EventEnable();
  SetKBCode(0);

  signal(SIGEVT, sh_input);
  ESetSignal();
  
   while(loop > 0)
   {
     n = readkeyboardevents(input, 128);
     if (n > 0)
     {
       printf("down: count(%d) 0x%02x\n", n, input[0]);
       if(input[0] == 0x01)
         break;
       
      loop--;
     }   	
   }	
   
   EventDisable();
   SetKBCode(1);
}
#endif

