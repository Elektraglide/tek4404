#include "ph.h"

extern int kernbootfile();
extern char *kernalloc();
extern int kernfree();

extern char *getsymbols();
extern symbolheader * listsymbol();
extern symbolheader * findsymbol();
extern int getkconstant();
extern int getkvalue();
extern int readkint32();

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
	unsigned int tsswap;
	short tsalrm,tscpu,tssize,tstxtp,tsdatp,tsstkp,tsutop,x2c,tssignal;
	
	short x30,x32,x34;
	
	unsigned int tssigsoft[2];
	unsigned int tssigmsk[2];
	unsigned char tsstat,tsmode,tsmode2,tsprir;
	unsigned char	tsprb,tsact;
	short tsage;
};


#ifdef __clang__
#pragma pack(pop)
#endif
