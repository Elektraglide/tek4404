CFLAGS= +v
LIBS= +l=netlib

dhcp:	dhcp.c
	cc +v $@.c $(LIBS)

ifdump:	ifdump.c
	cc +v $@.c $(LIBS)

telnetd:	telnetd.c
	cc +v $@.c $(LIBS)


