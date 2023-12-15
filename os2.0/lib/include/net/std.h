/*  Copyright (C) 4/22/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.std.h  */
/*  Version 11.2 as of 86/04/22 at 06:28:08  */

#ifndef	_STD_
#define	_STD_

#define	ARR_CNT(a)		(sizeof(a)/sizeof(a[0]))
#define	ARR_END(a)		(&a[ARR_CNT(a)])
#define	ARR_LAST(a)		(&a[ARR_CNT(a) - 1])
#define	U16_PATTERN		(u16)0x0100
#define	U32_PATTERN		(u32)0x03020100
#define	SYS_ERR			(-1)
#define	ERR			(-1)

/* pseudo keywords */
#define	boolean			int
#define	import			extern
#define	export
#define	OK			0
#define	true			1
#define	false			0
#define	fallthru
#define	skip
#define forever			while(1)
#define	until(cond)		while(!(cond))
#define	unless(cond)		if(!(cond))
#define	ifnot(cond)		if(!(cond))

/* kernel pseudo keywords */
#ifdef	KERNEL
#define use_critical		fast int _savpri
#define critical		(_savpri = os_critical())
#define normal			(os_normal(_savpri))
#endif

/* standard FUSION types */

typedef	boolean	(*pfb_t)();
typedef int	(*pfi_t)();
typedef long	(*pfl_t)();
typedef	pfi_t	(*pfpfi_t)();
typedef	char	* (*st)();	/* ptr. to a state function */

typedef struct a16 {		/* a 16-bit byte-ordered structure */
	char	a2[2];
} a16;
typedef struct a32 {		/* a 32-bit byte-ordered structure */
	char	a4[4];
} a32;
typedef struct a48 {		/* a 48-bit byte-ordered structure */
	char	a6[6];
} a48;

#ifndef TCP_ONLY
#define brd_host_addr(p)	/* normally, don't care what the board address is */
#endif

#include <net/config.h>		/* system configuration */
#include <net/ccdep.h>		/* C compiler dependencies */

#ifndef	_DEBUG_
#include <net/debug.h>
#endif

#ifndef	nil
#define	nil			(mem_addr)(0)
#endif

#ifndef	odd
#define	odd(x)			((x)&1)
#endif

#ifndef abs
#define abs(i)			 ((i < 0) ? -i : i)
#endif

#ifndef max
#define	max(x,y)		((x) >= (y) ? (x) : (y))
#endif

#ifndef min
#define	min(x,y)		((x) <= (y) ? (x) : (y))
#endif

#ifndef bound
#define bound(v,min,max)	((v)<(min) ? (min) : (v)<(max) ? (v) : (max))
#endif

#ifndef inbound
#define inbound(v,min,max)	((min)<=(v) && (v)<=(max))
#endif

#ifndef farelp
#define farelp			el *
#define farcp			char *
#endif

#ifndef SEP_STACK
#define cptostack(val, ptr)	(*(ptr) = val)
#endif

#ifndef newa
extern	char *	calloc();
#define	newa(type, cnt)	((type *)(calloc((unsigned)(cnt), (unsigned)sizeof(type))))
#endif

#ifndef resize
extern	char *	realloc();
#define	resize(type, ptr, cnt)	((type *)(realloc(ptr, (unsigned)(cnt) * (unsigned)sizeof(type))))
#endif

#ifndef	new
#define	new(type)		newa(type, 1)
#endif

#ifndef cfree
#define	cfree(ptr)		free((char *)(ptr))
#endif

#endif _STD_

/*
 *	@(#)std.h	11.2 (NRC)
 */
