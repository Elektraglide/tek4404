#include "ph.h"

extern char *getsymbols();
extern symbolheader * findsymbol();
extern int getkvalue();

#ifdef __clang__
#pragma pack(push, 1)
#endif

/* created using kernel labels (sys/task.h bears no relation to reality) */
struct kerntask {
	unsigned int tslink,tsslnk;
	short tsuid, tstid, tstidp, tstty;
	short x10;
	int tsevnt;
	int tstext;
	int tsswap;
	short tsalrm,tscpu,tssize,tstxtp,tsdatp,tsstkp,tsutop,x2c,tssignal;
	
	short x30,x32,x34;
	
	int tssigsoft[2];
	int tssigmsk[2];
	char tsstat,tsmode,tsmode2,tsprir,tsprb,tsact;
	short tsage;
};


#ifdef __clang__
#pragma pack(pop)
#endif