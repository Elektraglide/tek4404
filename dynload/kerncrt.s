	lib sysdef

	extern Pdata,Phex1,Phex2,Phex4,Phex,cpass,passc

	global _save_reg_params,_get_userblk,_get_fd,_get_majmin
	global _kpassc,_kcpass,_kstrlen,_kstrcat,_kprint,_kprinthex

	text
* innvokes init(chrtab, blktab) to setup devices
Start
	move.l #blktab,-(sp)
	move.l #chrtab,-(sp)
	jsr _init
	add.l #8,sp
	move.l #0,d0
	rts

_save_reg_params
	move.l a3,(userblk)
	move.l a5,(fd)
	move.l d7,(majmin)
	rts

_get_userblk
	move.l (userblk),d0
	rts

_get_fd
	move.l (fd),d0
	rts

_get_majmin
	move.l (majmin),d0
	rts

	data
userblk fqb 0
fd fqb 0
majmin fqb 0

	text

* from kernel TO userspace
* kpassc(char *, int)
_kpassc
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a1
	move.l (userblk),a3
fillbuf0
	move.b (a1)+,d6
	jsr passc
	bmi eofread
	move.l 12(a6),d2
	sub.l #1,d2
	move.l d2,12(a6)
	bpl fillbuf0
eofread
	move.l a1,d0
	sub.l 8(a6),d0
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

* from userspace TO kernel
* kcpass(char *, int)
_kcpass
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a1
	move.l (userblk),a3
fillbuf
	jsr cpass
	bmi.w eofwrite
	move.b d6,(a1)+
	move.l 12(a6),d2
	sub.l #1,d2
	move.l d2,12(a6)
	bpl fillbuf
eofwrite
	move.l a1,d0
	sub.l 8(a6),d0
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

_kstrlen
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
findend0
    move.b (a0)+,d0
	bne findend0
	sub #1,a0
	move.l a0,d0
	sub.l 8(a6),d0
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

_kstrcat
	link a6,#0
	movem.l d2-d7/a2-a5,-(sp)
	move.l 8(a6),a0
findend
    move.b (a0)+,d0
	bne findend
	move.l 12(a6),a1
	sub #1,a0
append
	move.b (a1)+,d0
	move.b d0,(a0)+
	bne append
	movem.l (sp)+,d2-d7/a2-a5
	unlk a6
	rts

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
	rts

	data
_environ fqb 0

	end Start
