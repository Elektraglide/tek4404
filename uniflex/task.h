/*
        UniFlex Task Table Entry
*/

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uword
#define uword unsigned short
#endif

#ifndef ulong
#define ulong unsigned long
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifdef __clang__
#pragma pack(push, 1)
#endif

/* compiling with 64-bit means we need to keep pointer fields as 32-bit */
#ifdef __clang__
#undef ulong
#define ulong unsigned int
typedef unsigned int ptr32;
#else
typedef char *ptr32;
#endif

/* used to play fast and loose with signed */
/* #define char unsigned char  */

struct task {
  ptr32   tslink,	/*  link to running tasks */
				  tsslnk;	/*  link to sleeping tasks  */
  uword		tsuid;		/*  user id field  */
  uword		tstid;		/*  unique task id */
  uword		tstidp;		/*  parent's task id  */
  uword		tstty;		/*  control terminal id  */
  uword   unknown0;
  int		tsevnt;		/*  event task is waiting on */
  ptr32		tstext;	/*  task text table pointer  */
  char		tsswap[4];	/*  disk offset of swap image  */
  uword		tsalrm;		/*  seconds until alarm  */
  uword		tscpu;		/*  cpu information */
  uword		tssize;		/*  size of swap image  */
  uword		tstxtp;		/*  # of text pages  */
  uword		tsdatp;		/* number of data pages  */
  uword		tsstkp;		/*  number of stack pages  */
  ulong		tsutop;		/*  physical user block address  */
  ulong		tssignal[2];	/*  pending signals  */
  ulong		tssigsoft[2];	/*  pending soft signals  */
  ulong		tssigmsk[2];	/* signal masks (ON: signal not being ignored)*/
  uchar		tsstat;		/*  task status code  */
  uchar		tsmode;		/*  task mode code  */
  uchar		tsmode2;	/*  task mode code 2  */
  uchar		tsprir;		/*  task priority  */
  uchar		tsprb;		/*  task priority bias  */
  uchar		tsact;		/*  task activity counter */
  uchar		tsage;		/*  task  residency age */
  
  char padding[1];	/* cant get sizeof == 78 */
};

#ifdef __clang__
#pragma pack(pop)
#undef ulong
#endif


#undef char

/*
      task status codes
*/
#define TRUN   '\1'   /*  running  */
#define TSLEEP '\2'   /*  sleeping (high priority)  */
#define TWAIT  '\3'   /*  waiting (low priority)  */
#define TCREAT '\4'   /*  creating new task  */
#define TTERM  '\5'   /*  termination process  */
#define TTRACE '\6'   /*  trace mode  */

/*
      task mode codes
*/

#define TCORE  0x01   /*  task is in core  */
#define TLOCK  0x02   /*  task is locked in core  */
#define TSYSTM 0x04   /*  task is system scheduler  */
#define TTRACP 0x08   /*  task is being traced  */
#define TSWAPO 0x10   /*  task is being swapped  */
#define TARGX  0x20   /*  task is in argument expansion  */
