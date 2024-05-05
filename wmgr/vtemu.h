
#ifndef vtemu_h
#define vtemu_h

enum VTstyle
{
  vtBOLD = 1,
  vtINVERTED = 2
};

#define MAXTERMCOLS 128
#define MAXTERMROWS 32

/* NB assumes stride is 128; else use ((R)*MAXTERMWIDTH + (C)) */
#define RC2OFF(R,C)  (((R)<<7) + (C))
#define OFF2R(O)  ((O)>>7)

/* (1<<vt.rows)-1 overflows uint32_t, so we dont want 64bit arithmetic */
#define ALLDIRTY 0xffffffff

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
  char buffer[MAXTERMCOLS*MAXTERMROWS];
  char attrib[MAXTERMCOLS*MAXTERMROWS];
  short cx,cy;
  unsigned int dirtylines;      /* mask of which lines are dirty so we can skip rendering => max 32 lines */
  unsigned char linelengths[MAXTERMROWS];
} VTemu;

extern void VTreset();
extern void VTmovelines();
extern void VTclearlines();
extern void VTnewline();
extern int VToutput();
extern void VTfocus();
extern void VTblur();

#endif /* vtemu_h */
