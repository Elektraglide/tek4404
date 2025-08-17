	lib sysdef

	extern Pdata
	extern Phex4

	global _kprint,_kprinthex,_kgetD7

	text
* innvokes init(chrtab, blktab) to setup devices
Start
	move.l #blktab,-(sp)
	move.l #chrtab,-(sp)
	jsr _init
	add.l #8,sp
	move.l #0,d0
	rts

	data


	text
* kprint(char *str)
_kprint	
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	jsr Pdata
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

* kprinthex(char *str, int value, int precision)	
_kprinthex
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
	jsr Pdata
	move.l 12(a6),d0
	move.l 16(a6),d1
	move.l #1,d2
	jsr Phex
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6

_kgetD7
	move.l d7,d0
	rts

	data
_environ fqb 0

	end Start
