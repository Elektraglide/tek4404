CFLAGS= +v +i=../ +DTEK4404
LIBS= +l=netlib
LFLAGS= +x=1M

all:	rcat kset top ash dhcp ifdump telnetd ntpclient diskutil screencastd

coreinfo: coreinfo.r
	cc +v coreinfo.r +o=$@ 
	headset +d $@

top: top.r ../kernutils.r ../kernasm.r
	cc +v +x=b=1M top.r ../kernutils.r ../kernasm.r +o=$@ +l=netlib
	perms s+ $@

lsof: lsof.r ../kernutils.r ../kernasm.r
	cc +v +x=b=1M lsof.r ../kernutils.r ../kernasm.r +o=$@ +l=netlib
	perms s+ $@

ctrace: ctrace.r ../kernutils.r ../kernasm.r
	cc +v ctrace.r ../kernutils.r ../kernasm.r +o=$@

uname: uname.r ../kernasm.r
	cc +v uname.r ../kernasm.r +o=$@ +l=netlib
	perms s+ $@

systat: systat.r kernutils.r kernasm.r
	cc +v systat.r kernutils.r kernasm.r +o=$@
	perms s+ $@

dhcp:	dhcp.r
	cc +v $*.r $(LIBS) +o=$@

ntpclient:	ntpclient.r
	cc +v $*.r $(LIBS) +o=$@

ifdump:	ifdump.r
	cc +v $*.r $(LIBS) +o=$@

diskutil:	diskutil.r
	cc +v $*.r +o=$@



