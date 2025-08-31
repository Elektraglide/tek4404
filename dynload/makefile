CFLAGS= +v +i=/tek
LIBS= +l=netlib
LFLAGS= +x=1M

devices.rel: kerncrt.r devices.r
	load +ro=$@ kerncrt.r devices.r

kerncrt.r:	kerncrt.s
	$(AS) +ueo=$@ $<

devices.r:	devices.c

load:
	/gen/system/dynamicLoad +n devices.rel +o=devices.load



