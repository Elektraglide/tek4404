CFLAGS= +vq
LIBS= +l=netlib
LFLAGS= +x=1M


flip:	flip.r 3d.r line.r mathfasm.r
	cc +x=b=1M +v flip.r 3d.r line.s.r mathfasm.r +o=$@ +l=graphics

