#ifndef PH_H
#define PH_H

/* tek4404 executable header */

#ifdef __clang__
#pragma pack(push, 1)
#endif
/* 64 bytes, big endian */
typedef struct
{
  unsigned char magic[2];
  /* 0x04 = exe
     0x05 = relocatable
     0x06 = manifest???
     0x8d = BASIC compiled
     
     0x00 = normal load
     0x20 = demand paged
     0x40 = quick load
	*/
  int textsize;
  int datasize;
  int bsssize;
  int relocsize;

  int xferaddress;
  int textstart;
  int datastart;
  char minpage;
  char maxpage;
  char stacksize;
  char initialstack;
  int symbolsize;

  short commentsize;
	short namesize;
  short flags;
  /* 0x8000 = valid xfer address
     0x1000 = has 68881 instructions
     0x0800 = produce no core dump
     0x0400 = has 68020 instructions
     0x0200 = 68881 signals enabled
     0x0020 = bss not cleared
     0x00c0 = segment xfer code
	*/

  short minpagealloc;
	short maxpagealloc;
	
	short taskdescriptor;
  short rcssize;
  
  int unknown3;
  
  short source;			/* 1 = assembler, 2 = C, 3 = Pascal, 4 = Fortran, 5 = Cobol */
  short unknown4;
  
  short configuration;
  /* 0 = unknown
     1 = Momentum Hawk
     2 = Pixel 100
     4 = Tektronix 4404
     5 = TSC SBC
     6 = Tektronix CBI
     7 = Force CPU-3V
     8 = VME/10
     9 = Ironics IV1600
		10 = Gimix GMX-20
		11 = NCR Tower
		12 = Gimix MICRO-20
		13 = SWTPc SB68K
		14 = VTM-1000
		15 = Tektronix 4406
		17 = SWTPcVME System
		64 = Gimix MICRO-20 (non-MMU)
  */

  short unknown5;
} PH;

typedef struct {

  short kind;
  int offset;
  short segment;

  short len;
  
} symbolheader;

#define SEGABS 8
#define SEGTEXT 9
#define SEGDATA 10
#define SEGBSS 11

typedef struct {

	int offset;
	short kind;
	short len;
	
} relocheader;

/* memory map */

/* Uniflex device driver entry */
typedef struct {
	int (*d_open)();
	int (*d_close)();
	int (*d_read)();
	int (*d_write)();
	int (*d_ioctl)();
} devicedriver;

/* 10 standard device drivers */
/* devicedriver *majordevices = (devicedriver *)0x4c4; */

#ifdef __clang__
#pragma pack(pop)
#endif

#endif /* PH_H */

