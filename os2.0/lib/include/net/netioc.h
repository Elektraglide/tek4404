/*  Copyright (C) 3/10/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.netioc.h  */
/*  Version 11.2 as of 86/03/10 at 10:31:12  */

#ifndef	_NETIOC_
#define _NETIOC_

#ifndef	_STD_
#include <net/std.h>
#endif

#define	EL_VERSION	0
#define EL_MAX_SIZE	10

#ifndef	SOIOCBASE
#define	SOIOCBASE	('s' << 8)
#endif

#ifndef	SOIOCGNTTY
#define	SOIOCGNTTY	(SOIOCBASE-2)
#define	SOIOCSNTTY	(SOIOCBASE-1)
#endif

#ifndef SOIOCATMARK
#define	SOIOCATMARK	(SOIOCBASE-3)	/* for MSG_OOB */
#endif

#ifndef	ENIOCBASE
#define	ENIOCBASE	('e' << 8)
#endif

#ifndef	INPIOCBASE
#define	INPIOCBASE	('i' << 8)
#endif

#define	INPIOCDOWNLOAD		(INPIOCBASE + 0)
#define	INPIOCEXECUTE		(INPIOCBASE + 1)
#define	INPIOCINITIALIZE	(INPIOCBASE + 2)
#define INPIOCPROBE		(INPIOCBASE + 3)
#define INPIOCDIDDLE		(INPIOCBASE + 4)

/* Values for raw ethernet ioctls.  These select the action inside
 * the link layer ioctl routine.
 */
#define ENIOCEND	(ENIOCBASE+10)
#define	ENIOCNORMAL	ENIOCBASE	/* accept broadcast and specific */
#define	ENIOCPROMISC	(ENIOCBASE+1)	/* accept all undamaged packets */
#define	ENIOCALL	(ENIOCBASE+2)	/* accept ALL packets */
#define	ENIOCRESET	(ENIOCBASE+3)

#ifdef	MSDOS
/* Raw socket driver interface ioctls for MSDOS only! */
#define SDIOCBASE	('o' << 8)

#define	SDIOCSTK	SDIOCBASE	/* give pre-sockdrv stack */
#endif	MSDOS

/* Defined values for el.el_u2.i field of el array header */
#define	ELC_ACCEPT	0	/* accept a new connection */
#define ELC_ADDR	1	/* give back the current net address */
#define	ELC_ASYNC	2	/* set/reset the socket as async */
#define	ELC_ATTACH	3	/* attach to a protocol */
#define	ELC_BETIMER	4	/* assume the timeout chores */
#define	ELC_BIND	5	/* bind to a network address */
#define	ELC_BLOCKING	6	/* turn off/on blocking */
#define	ELC_CONNECT	7	/* actively seek connection */
#define	ELC_DEBUG	8	/* network debugging entrypoint */
#define	ELC_DIDDLE	9	/* peek/poke kernel variables */
#define	ELC_EBDEV	10	/* external block device attach */
#define	ELC_GETOPT	11	/* get protocol options now current */
#define	ELC_INDEX	12	/* give/get the socket index */
#define	ELC_LISTEN	13	/* listen for connection requests */
#define	ELC_NEXTSOCKET	14	/* give me the next available socket index */
#define	ELC_PAIR	15	/* activate the socketpair() code */
#define	ELC_PEERADDR	16	/* deliver up the peer address */
#define	ELC_RECV	17	/* recv a packet */
#define	ELC_SEND	18	/* send a packet */
#define	ELC_SETOPT	19	/* set protocol specific options */
#define	ELC_SHUTDOWN	20	/* disallow read/write on socket */
#define	ELC_SREAD	21	/* read data (used in VMS) */
#define	ELC_SWRITE	22	/* write data (used in VMS) */
#define	ELC_NSELECT	23	/* select available io on multiple sockets */
#define	ELC_PIOCTL	24	/* protocol specific ioctl */
#define	ELC_REJECT	25	/* reject a connection request */

#define	EL_HEADER	(0x4000 | EL_VERSION)
#define	EL_NOOP		0
#define	EL_PROTOCOL	1	/* as in XNS_DGRAM */
#define	EL_FAMILY	2	/* as in XNS, DARPA etc */
#define	EL_INDEX	3	/* request/give a socket index */
#define	EL_A_LEN	4	/* where to place the address length */
#define	EL_D_LEN	5	/* where to place the data length */
#define	EL_DATA		6	/* where's the beef? */
#define	EL_DEST		7	/* destination network address */
#define	EL_SRC		8	/* src network address */
#define	EL_FLAGS	9	/* as in MSG_OOB MSG_PEEK etc. */
#define	EL_TYPE		10	/* type field, interpreted by protocol */
#define	EL_P0		11	/* protocol specific param 0 */
#define	EL_P1		12	/* protocol specific param 1 */
#define	EL_P2		13	/* protocol specific param 2 */
#define	EL_P3		14	/* protocol specific param 3 */
#define	EL_P4		15	/* protocol specific param 4 */
#define	EL_PHDR		16	/* protocol header */
#define	EL_PHLEN	17	/* protocol header length */

/* Flags for el.el_flags, musn't overlap common flag area (see <flags.h>) */
#define	F_EL_STRUCTURED	0x1	/* structured data (32 bits) */
#define	F_EL_IN		0x2	/* incoming (user ==> kernel) data element */
#define	F_EL_OUT	0x4	/* outgoing (kernel ==> user) data element */
#define	F_EL_KERNEL	0x8	/* element is in kernel space */
#define F_EL_ALLOC	0x10	/* element is in heap space -- must be freed */
#define	F_EL_PROCESS	0x20	/* some additional processing to do */
#define	F_EL_PPROCESS	0x40	/* don't free, more processing to do */
#define	F_EL_MPRE_ALLOC	0x80	/* m structure is pre-allocate */

#define	F_EL_KRESERVED	F_EL_KERNEL	/* el flags reserved for kernel use */

/* if the size of el changes fix el_cnt()!! */
typedef struct el {
	u16	el_type;
	u16	el_flags;
	u32	el_len;
	u32	el_u1;
	u32	el_u2;
} el;

import	el	* _elc(), * _el_init();

/* How many used. Expects size of an el to be 32 bytes */
#define	el_cnt(elp1)	(((int)((char *)elp1[0].el_u1 - (char *)(elp1))) >> 4)

/* the next one available */
#define	el_end(elp1)	((el *)(elp1)->el_u1)

/* the last valid one */
#define	el_last(elp1)	(((el *)elp1->el_u1) - 1)

/* size in bytes*/
#define	el_size(elp1)	(int)((char *)(elp1)[0].el_u1 - (char *)(elp1))

#define	el_ioc(elp)	((SOIOCBASE-1)+el_cnt(elp))
#define	el_u(elp, type, u1) \
	_elc(elp, type, (u32)sizeof(u32), (u16)F_EL_STRUCTURED)->el_u1 = (u32)(u1)

#ifdef	KERNEL

#define	el_init(elp, cmnd, flags) \
	_el_init(elp,(u32)(cmnd),(u16)flags| F_EL_KERNEL)
#ifndef kcanonptr
#define kcanonptr(p)	((u32)p)
#endif
#define el_ucp(elp, type, p, size, f) \
	_elc(elp, type, (u32)(size), (u16)(f))->el_u1=((u32)p)
#define el_cp(elp, type, p, size, f) \
	_elc(elp, type, (u32)(size), (u16)(f)|(u16)F_EL_KERNEL)->el_u1=(kcanonptr(p))
#define	el_up(elp, type, p, f) \
	_elc(elp, type, (u32)sizeof(u32), (u16)(f)|(u16)(F_EL_STRUCTURED|F_EL_KERNEL))->el_u1=(kcanonptr(p))
#define	el_xin(elp,kdest,c)	((elp[0].el_len==c)?el_in(elp,kdest,c):EINVAL)
#define	el_xout(elp,udest,c)	((elp[0].el_len==c)?el_out(elp,udest,c):EINVAL)
#else	KERNEL
#define	el_init(elp, cmnd, flags)	_el_init(elp, (u32)(cmnd), (u16)flags)
#ifndef canonptr
#define	canonptr(p)	((u32)p)
#endif
#define el_cp(elp, type, p, size, f) \
	_elc(elp, type, (u32)(size), (u16)(f))->el_u1=(canonptr(p))
#define	el_up(elp, type, p, f) \
	_elc(elp, type, (u32)sizeof(u32), (u16)(f)|(u16)F_EL_STRUCTURED)->el_u1=(canonptr(p))
#endif	KERNEL

#endif	_NETIOC_

/*
 *	@(#)netioc.h	11.2 (NRC)
 */
