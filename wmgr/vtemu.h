
#ifndef vtemu_h
#define vtemu_h

enum VTstyle
{
  vtBOLD = 1,
  vtINVERTED = 2
};

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
  char buffer[132*32];
  char attrib[132*32];
  short cx,cy;
  unsigned int dirtylines;      /* mask of which lines are dirty so we can skip rendering => max 32 lines */
} VTemu;

extern void VTreset();
extern void VTmovelines();
extern void VTclearlines();
extern void VTnewline();
extern int VToutput();
extern void VTfocus();
extern void VTblur();

#endif /* vtemu_h */
