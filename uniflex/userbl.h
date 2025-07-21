/*
    definition of UniFlex user block
*/

#ifndef uword
#define uword unsigned short
#endif
#ifndef ulong
#define ulong unsigned long
#endif

#define MTSIZE 10
#define SIGCNT 63
#define MAX_DIR_SIZE 64
#define UNFILS 32

#define MAX_CHUNKS 64		/* fixed for all versions */
#define MAX_MDEP_SEGS 8		/* # of machine dependent mappable segments */

#ifdef __clang__
#pragma pack(push, 1)
#endif

/* compiling with 64-bit means we need to keep pointers as 32-bit */
#ifdef __clang__
#undef ulong
#define ulong unsigned int
#define long int
typedef unsigned int ptr32;
typedef unsigned int ulongptr32;
#else
typedef char *ptr32;
typedef ulong *ulongptr32;
#endif

/* used to play fast and loose with signed */
#define char unsigned char

struct mt {
	ulong paddr;
	ulong vaddr;
	uword numpages;
};

struct userbl {
  ulong  utimu;		/*  task user time  */
  ulong  utims;		/*  task system time  */
  uword  utchunk;	/*  number of text chunks  */
  uword  udchunk;	/*  number of data chunks  */
  uword  uschunk;	/*  number of stack chunks  */
  ulongptr32  uregs;	/*  user register pointer */
  char	 umemc;		/*  memory config byte (binary descriptor) */
  char	 uerror;	/*  error code returned */
  char	 umem[MAX_CHUNKS*MTSIZE]; /* memory map proper */
  char	 expSwap[MTSIZE];/*  reserve space for expansion swap chunk */
  long	 mapTerm; /*  reserve space for map terminator (-1) */
  char	 umdep_segs[MAX_MDEP_SEGS*MTSIZE];/* machine dept mapped segments */
  long	 mapTerm2;	/* map terminator */
  uword	 udep_chunks;	/* # chunks valid in above map */
  char	 utask_ord;	/* Task ordinal # (index into tables) */
  char	 uproc_id;	/*  Process ID (memory map identifier) */
  uword	 ubin_flags;	/* flags from current binary */
  ulongptr32	 utmat;	/* Pointer to task's "tmat" entry */
  ulongptr32	 utext_flink;	/* Shared TEXT forward link */
  ulongptr32	 utext_blink;	/* Shared TEXT backward link */
  ulong	 umark0;	/* mark reg 0 */
  ulong	 umark1;	/* mark reg 1 */
  ulong	 umark2;	/* mark reg 2 */
  ptr32  utask;	/* pointer to task structure */
  ptr32  uvfork;	/* pointer to parent's task structure during "vfork" */
  uword	 uuid;		/* effective user id */
  uword	 uuida;		/* actual user id */
  ptr32	 ucrdir;	/* fdn ptr of current dir */
  ulong	 ufdel;		/* first deleted directory */
  ulong	 ulstdr;	/* fdn of last dir searched */
  ptr32	 ucname;	/* pointer to command name arg */
  ptr32	 ufiles[UNFILS];/* pointers to open files */
  ulong	 usarg0;	/* user argument 0 */
  ulong	 usarg1;	/* user argument 1 */
  ulong	 usarg2;	/* user argument 2 */
  ulong	 usarg3;	/* user argument 3 */
  ptr32	 uistrt;	/* start address for I/O */
  long	 uicnt;		/* I/O byte counter */
  long	 uipos;		/* I/O file offset */
  long	 utimuc;	/* total childs user time (100ths of a second) */
  long	 utimsc;	/* total childs system time (100ths of a second) */
  long	 usigs[SIGCNT+1];/* condition of all signals (0 is "quiet" mode flag)*/
  long	 uprfpc;	/* profile pc */
  long	 uprfbf;	/* profile buffer */
  long	 uprfsz;	/* buffer size */
  long	 uprfsc;	/* profile scale */
  long	 unxtbl;	/* read ahead block */
  long	 utxtfwa;	/* fwa text segment */
  long	 udatfwa;	/* fwa data segment */
  long	 ustkfwa;	/* fwa stack segment */
  long	 ustklim;	/* lwa+1 of stack */
  uword	 usizet;	/* number of text pages */
  uword	 usized;	/* number of data pages */
  uword	 usizes;	/* number of stack pages */
  long	 umapreg[2];	/* mapping register scratch (machine dependent) */
  uword	 utimlmt;	/* user time limit */
  uword	 uiotlmt;	/* user I/O transfer limit */
  uword	 umemlmt;	/* user memory limit */
  uword	 udname_size;	/* size of name in "udname" */
  uword	 udname_slot;	/* number of directory slots used by "udname" */
  uword	 uwrk_size;	/* size of name in "uwrkbf" */
  uword	 uwrk_slot;	/* number of directory slots used by "uwrkbf" */
  char	 ufdn[MAX_DIR_SIZE]; /*  current dir entry */
  char	 uwrkbf[MAX_DIR_SIZE];	/* work buffer for path name */
  char	 udperm;	/* default file permissions */
  char	 uiosp;		/* i/o space(0=system,1=user data,-1=user instruction)*/
  char	 umaprw;	/* read write mapping flag */
  char	 ufault_ok;	/* Non-zero if system is doing "moves" for user */
  long	 ustart;	/* task start time */
  ptr32	 uexnam;	/* exec'd file name pointer */
  uword	 uiocnt;	/* I/O block count */
  uword	 umxmem;	/* max mem usage */
  uword	 uhltpri;	/* reason for halt (priority) */
  ulongptr32	 uworkspace;	/* pointer to temp user workspace */
  
  /* reverse engineered */
  ptr32 unknownptr;
  char padding[550 + 174];
  ulong	ubits;					/* 0x882 */
  ulong ustack_bottom;	/* 0x886 */
  uword udfc;						/* 0x88a */
  uword usfc;						/* 0x88c */
  uword uquantum;				/* 0x88e */
  uword ucpu;						/* 0x890 */
  uword usys_ratio;			/* 0x892 */
  ulong upersonality;		/* 0x894 */
  
};

#ifdef __clang__
#pragma pack(pop)
#undef ulong
#undef long
#endif

#undef char

#define udname &ufdn[2] /* name portion of directory entry */

#define USTSIZ sizeof(userbl)  /* user structure size */

#define upostb urglst[7]       /*  system call post byte  */
