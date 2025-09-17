	lib sysdef

	extern Pdata,Phex1,Phex2,Phex4,Phex,cpass,passc

	global _kpassc,_kcpass,_kstrlen,_kstrcat,_kprint,_kprinthex

	text
* innvokes cdinit(chrtab) to setup device, returns device major index
Start
	move.l #cdfunc,-(sp)
	jsr _cdinit
	add.l #4,sp
* major device in d0 and struct is 20 bytes
	move.l d0,d1
	lsl.w #4,d0
	lsl.w #2,d1
	add.w d1,d0
	move.l #chrtab,a0
	add.l d0,a0
* install trampolines to C functions
	move.l #cd_open,(a0)
	move.l #cd_close,4(a0)
	move.l #cd_read,8(a0)
	move.l #cd_write,12(a0)
	move.l #cd_special,16(a0)
	
	move.l #0,d0
	rts
	
cd_open
	move.l a3,(userblk)
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l a3,-(sp)
	move.l (cdfunc),a0
	jsr (a0)
	add #12,sp
	rts
	
cd_close
	move.l a3,(userblk)
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l a3,-(sp)
 	move.l (cdfunc+4),a0
	jsr (a0)
	add #12,sp
	rts

cd_read
	move.l a3,(userblk)
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l a3,-(sp)
 	move.l (cdfunc+8),a0
	jsr (a0)
	add #12,sp
	rts

cd_write
	move.l a3,(userblk)
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l a3,-(sp)
 	move.l (cdfunc+12),a0
	jsr (a0)
	add #12,sp
	rts

cd_special
	move.l a3,(userblk)
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l a3,-(sp)
 	move.l (cdfunc+16),a0
	jsr (a0)
	add #12,sp
	rts

	data
cdfunc fqb 0,0,0,0,0
userblk fqb 0

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
	bne fillbuf0
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
