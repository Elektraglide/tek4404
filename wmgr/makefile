TCC=cc
CFLAGS= +r +v
#CFLAGS= -c -O
LIBS= +l=graphics +l=netlib

wmgr:	wmgr.r vtemu.r events.r ../kernutils.r ../kernasm.r
	cc +x=b=1M +v wmgr.r vtemu.r events.r ../kernutils.r ../kernasm.r singlegnulib.r +o=$@ +l=graphics +l=netlib
	perms s+ $@

wmgr.r:		wmgr.c
	$(CC) $(CFLAGS) $<
vtemu.r:	vtemu.c
	$(CC) $(CFLAGS) $<
events.r:	events.c
