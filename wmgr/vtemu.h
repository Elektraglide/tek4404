//
//  vtemu.h
//  winprocmgr
//
//  Created by Adam Billyard on 05/08/2023.
//

#ifndef vtemu_h
#define vtemu_h

typedef struct
{
  char state;
  char wrapping;
  char hidecursor;
  char escseq[16];
  short sx,sy;
  short margintop,marginbot;

  short cols,rows;
  char buffer[80*25];
  short cx,cy;
  int dirty;
} VTemu;

extern void VTreset();
extern void VTmovelines();
extern void VTclearlines();
extern void VTnewline();
extern int VToutput();


#endif /* vtemu_h */
