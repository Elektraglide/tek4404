CFLAGS= +v +DTEK4404
LIBS= +l=netlib
LFLAGS= +x=1M

all:	scroller

scroller:	scroller.r landdat.r
	cc +v +o=$@ +x=b=1M scroller.r landdat.r +l=graphics

timers:	timers.c
	gcc -O -c timers.c
	cc +v +o=$@ timers.r +l=graphics
