//
//  events.c
//  tek4404_utilities
//
//  Created by Adam Billyard on 05/02/2025.
//

#include <stdio.h>
#include <graphics.h>

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
  
  ecount = EGetCount();
  while (ecount-- > 0 && maxlen)
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
        *buffer++ = ev.estruct.eparam;
        maxlen--;
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

}
