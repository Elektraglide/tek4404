#include "ph.h"

extern int kernbootfile();
extern char *kernboottime();
extern char *kernalloc();
extern int kernfree();

extern char *getsymbols();
extern symbolheader * listsymbol();
extern symbolheader * findsymbol();
extern int getkconstant();
extern int getkint32();
extern int getkint16();
extern int readkint32();
extern int readkint16();
extern char * readkstring();

#ifdef __clang__
#pragma pack(push, 1)
#endif

/* created using kernel labels (sys/task.h bears no relation to reality) */
struct kerntask {
	unsigned int tslink,tsslnk;
	short tsuid, tstid, tstidp, tstty;
	short x10;
	unsigned int tsevnt;
	unsigned int tstext;
	
	unsigned int tsswap;	/* or {char tsstate,tsstate2,tsstate3} */
	short tsalrm;
	/* 0x20 */
	short tscpu,tssize,tstxtp,tsdatp,tsstkp;
	unsigned int tsutop;		/* <== this is a pointer to a userblk */
	
	unsigned int tssignal[2];  /* 0x2e */
	
	unsigned int tssigsoft[2];  /* 0x36 */
	unsigned int tssigmsk[2];   /* 0x4e */
	unsigned char tsstat,tsmode,tsmode2,tsprir, tsprb, tsact; /* 46,47,48,49  State Mode Pri CPU */

	short tsage;
};

struct clist {
    char           *c_cf;
    char           *c_cl;
    short           c_cc;
};

struct kerntty {
    struct clist   *t_rawq;       /* 0x00 */
    struct clist   *t_canq;       /* 0x04 */
    struct clist   *t_outq;       /* 0x08 */
    char           *t_addr;       /* 0x0c */
    unsigned short  t_dev;        /* 0x10 */
    unsigned char   t_ispeed;     /* 0x12 */
    unsigned char   t_ospeed;     /* 0x13 */
    char            t_delct;      /* 0x14 */
    char            t_col;        /* 0x15 */
    char            t_kill;       /* 0x16 */
    char            t_erase;      /* 0x17 */
    unsigned short  t_flags;      /* 0x18 */
    unsigned char   t_state;      /* 0x1a */
    unsigned char   t_xflags;     /* 0x1b */
    char            t_line;       /* 0x1c */
    char            t_pad1;       /* 0x1d */
    unsigned char   t_taskid;     /* 0x1e */
    char            t_pad2;       /* 0x1f */
    char            t_reserved[10];/* 0x20 */
}; /* Total size = 42 bytes (0x2a) */

#define T_TIMEOUT 1
#define T_WOPEN   2
#define T_ISOPEN  4
#define T_CARR_ON 16
#define T_BUSY    32
#define T_ASLEEP  64
#define T_XCLUDE  128

#define X_TTSTOP  2
#define X_XOFFPEND  4
#define X_XONPEND  8
#define TTASLEEP  16
#define X_INTERL  128

#ifdef __clang__
#pragma pack(pop)
#endif
