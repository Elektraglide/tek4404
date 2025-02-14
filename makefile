CFLAGS= +v
LIBS= +l=netlib
LFLAGS= +x=1M

all:	rcat kset status ash dhcp ifdump telnetd ntpclient diskutil screencastd

rcat:	rcat.c
	cc +v +x=b=1M $*.c

kset: kset.r kernutils.r kernasm.r
	cc +v kset.r kernutils.r kernasm.r +o=$@

status: status.r kernutils.r kernasm.r
	cc +v status.r kernutils.r kernasm.r +o=$@

systat: systat.r kernutils.r kernasm.r
	cc +v systat.r kernutils.r kernasm.r +o=$@

ash:	ash.r
	cc +v $*.r +o=$@

dhcp:	dhcp.r
	cc +v $*.r $(LIBS)

screencastd:	screencastd.c
	cc +v $< $(LIBS)

ntpclient:	ntpclient.r
	cc +v $*.r $(LIBS)

ifdump:	ifdump.r
	cc +v $*.r $(LIBS)

telnetd:	telnetd.r kernasm.r
	cc +v telnetd.r kernasm.r $(LIBS) +o=$@

diskutil:	diskutil.r
	cc +v $*.r



