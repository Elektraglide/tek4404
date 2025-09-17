# builds custom character device driver
# create the device with:
# /etc/makdev <name> 10 0    for a test device
# /etc/makdev <name> 10 1    for a device that gives process reflection 
# /etc/makdev <name> 10 7    for a source of random bytes

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



