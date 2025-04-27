	lib sysdef

	global _pagemonitor
	
; this is just missing Vendor call
	text
_pagemonitor	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),pbuffer+2
	move.l 12(a6),pbuffer+6
	move.l 16(a6),pbuffer+10
	sys ind,pbuffer
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	data
pbuffer	dc.w $100
		dc.l 0
		dc.l 0
		dc.l 0

	global _systat
	
; this is just missing from clibs
	text
_systat	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),ibuffer+2
	sys ind,ibuffer
    move.l 8(a6),d0
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	data
ibuffer	dc.w systat
		dc.l 0

	global _system_control

; this is just missing from clibs
	text
_system_control	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),d0
	sys system_control
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	global _kernscsidump
	
; expects #0x1084 or #0x1b1b824 magic values for choosing message
	text
_kernscsidump	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	move #70,d0
	trap #13
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	global _kernbootfile
	
	text
_kernbootfile	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	move #71,d0
	trap #13
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	global _kernalloc
	
	text
_kernalloc	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),d1
	move #61,d0
	trap #13
	bcs L2
	move.l a0,d0
L1
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts
L2
	move.l #0,d0
	bra L1

	global _kernfree

	text
_kernfree	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	move.l 12(a6),d1
	move #62,d0
	trap #13
	bcs L3
	move #1,d0
L3
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	global _kernexec

	text
_kernexec	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	move #60,d0
	trap #13
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

	end

; need a way of validating this absolute address has not moved
;
add_custom_map EQU $fe6a

	global _kernmapper

	text
_kernmapper	link a6,#0

  lea fpu_seg(pc),a0
; if we could pass arguments to kernexec()  (via usarg0?)
;  move.l 8(a6),(a0)
;  move.l 12(a6),4(a0)
;  move.l 16(a6),d0
;  move.w d0,8(a0)
  bsr add_custom_map
  move.l (a0),d0
  rts
  
; Uniflex standard umdep_segs
; 0x007e0000  0x00600000  videoram
; 0x007de000  0x00000000  shared_page0 (phys(2) is unsupported)
; 0x007dd000  0x00000000  shared_page1 (phys(3) is unsupported)
; 0x007dc000  0x007ba000  rtc
;

fpu_seg    EQU     *
        dc.l 0x007db000
        dc.l 0x0078a000   ;fpu
        dc.w 1

mappersize    EQU     *-_kernmapper
