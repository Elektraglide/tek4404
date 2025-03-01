
	lib sysdef

	global _asmflmul

	text
_asmflmul
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1

    move.w #190,8(a0)
    move.w #12610,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L3
    move.w (a0),d0
    dbpl d1,L3
    move.l -(a0),d0
	unlk a6
	rts

	global _asmfladd

	text
_asmfladd
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1

    move.w #190,8(a0)
    move.w #322,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L4
    move.w (a0),d0
    dbpl d1,L4
    move.l -(a0),d0
	unlk a6
	rts

	end

