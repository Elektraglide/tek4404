	lib sysdef

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
	bcs L2
	move #1,d0
L2
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
