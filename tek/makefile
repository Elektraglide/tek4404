CFLAGS= +v
LIBS= +l=netlib
LFLAGS= +x=1M

all:	rcat kset top ash dhcp ifdump telnetd ntpclient diskutil screencastd


rcat:	rcat.c
	cc +v +x=b=1M $*.c

kset: kset.r kernutils.r kernasm.r
	cc +v kset.r kernutils.r kernasm.r +o=$@

top: top.r kernutils.r kernasm.r
	cc +v +x=b=1M top.r kernutils.r kernasm.r +o=$@ +l=netlib

systat: systat.r kernutils.r kernasm.r
	cc +v systat.r kernutils.r kernasm.r +o=$@

fpu:	fpu.r mathfasm.r
	cc +v fpu.r mathfasm.r +o=$@

ash:	ash.r
	cc +v $*.r +o=$@

flip:	flip.r 3d.r mathfasm.r
	cc +v flip.r 3d.r mathfasm.r +o=$@ +l=graphics

dhcp:	dhcp.r
	cc +v $*.r $(LIBS) +o=$@

screencastd:	screencastd.c
	cc +v $< $(LIBS)

ntpclient:	ntpclient.r
	cc +v $*.r $(LIBS) +o=$@

ifdump:	ifdump.r
	cc +v $*.r $(LIBS) +o=$@

telnetd:	telnetd.r kernasm.r
	cc +v telnetd.r kernasm.r $(LIBS) +o=$@

diskutil:	diskutil.r
	cc +v $*.r +o=$@



