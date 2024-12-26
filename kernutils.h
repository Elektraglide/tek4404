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

struct kerntty {
	unsigned int buffstart,buffread,buffend;

	int device_addr,unknown2;
	
	short print_column;
	char t_erase,t_kill,t_char,t_ispeed,t_ospeed,t_unknown;
	
	char unknown[14];   /* 0x1e owning tskid */
};

#ifdef __clang__
#pragma pack(pop)
#endif
