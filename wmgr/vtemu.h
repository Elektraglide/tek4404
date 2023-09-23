
#ifndef vtemu_h
#define vtemu_h

typedef struct
{
  char state;
  char wrapping;
  char hidecursor;
  char focusblur;
  char style;
  char escseq[16];
  short sx,sy;
  short margintop,marginbot;

  short cols,rows;
  char buffer[80*32];
  char attrib[80*32];
  short cx,cy;
  int dirty;
} VTemu;

extern void VTreset();
extern void VTmovelines();
extern void VTclearlines();
extern void VTnewline();
extern int VToutput();
extern void VTfocus();
extern void VTblur();

#endif /* vtemu_h */
