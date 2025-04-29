	lib sysdef
	lib sysfloat

*
* memory-mapped FPU functions
*
	global _vmmulf
	global _vmdivf
	global _vmaddf
	global _vmsubf

*
* single precision traps
*
	global _ftoi
	global _addf
	global _subf
	global _mulf
	global _divf
	global _maddf

	text

*
* ftoi(a)
*
_ftoi	move.l 4(sp),d0
	move.l #FFtoIf,d2
	trap #sysfp
	rts	

*
* addf(a,b)
*
_addf	move.l 4(sp),d0
	move.l 8(sp),a0
	move.l #FADD,d2
	trap #sysfp
	rts	

*
* subf(a,b)
*
_subf	move.l 4(sp),d0
	move.l 8(sp),a0
	move.l #FSUB,d2
	trap #sysfp
	rts	

*
* mulf(a,b)
*
_mulf	move.l 4(sp),d0
	move.l 8(sp),a0
	move.l #FMUL,d2
	trap #sysfp
	rts	

*
* divf(a,b)
*
_divf	move.l 4(sp),d0
	move.l 8(sp),a0
	move.l #FDIV,d2
	trap #sysfp
	rts	

*
* maddf(a,b,c)
*
_maddf	move.l 4(sp),d0
	move.l 8(sp),a0
	move.l #FMUL,d2
	trap #sysfp
	move.l 12(sp),a0
	move.l #FADD,d2
	trap #sysfp
	rts	

*  NB includes swap on inputs and output

_vmmulf
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1
    swap d0
	swap d1
    move.w #190,8(a0)
    move.w #12610,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L3
    move.w (a0),d0
    dbpl d1,L3
    move.l -(a0),d0
	swap d0

	unlk a6
	rts

_vmdivf
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1
    swap d0
	swap d1
    move.w #$be,8(a0)
    move.w #$2142,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L5
    move.w (a0),d0
    dbpl d1,L5
    move.l -(a0),d0
	swap d0

	unlk a6
	rts

	text
_vmaddf
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1
    swap d0
	swap d1

    move.w #190,8(a0)
    move.w #322,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L4
    move.w (a0),d0
    dbpl d1,L4
    move.l -(a0),d0
	swap d0

	unlk a6
	rts

_vmsubf
	link a6,#0
	move.l #$7de004,a0
	move.l 8(a6),d0
	move.l 12(a6),d1
    swap d0
	swap d1

    move.w #$be,8(a0)
    move.w #$1142,(a0)
    move.l d0,(a0)
    move.l d1,(a0)+
    move #32,d1
L6
    move.w (a0),d0
    dbpl d1,L6
    move.l -(a0),d0
	swap d0

	unlk a6
	rts

	end

