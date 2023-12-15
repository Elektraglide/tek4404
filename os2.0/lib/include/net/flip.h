/*  Copyright (C) 4/22/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.flip.h  */
/*  Version 11.2 as of 86/04/22 at 07:40:18  */

/* Macros to do various flips */

#ifndef _FLIP_
#define _FLIP_

#ifndef	_STD_
#include <net/std.h>
#endif

#ifdef L2R_16
	/* flips a byte pattern of 0102 to 0201 */
import	u16		_flip16();	/* should be sed scripted */
#define _FLIP16(C)	(u16)((((C)>>8)&0xFF)|(((C)<<8)&0xFF00))
#else
	/* flips a byte pattern of 0102 to 0102 (cast only) */
#define	_flip16(u)	(u16)(u)
#define	_FLIP16(C)	(u16)(C)
#endif	L2R_16

#ifdef	L2R_32
	/* flips a byte pattern of 01020304 to 04030201 */
import	u32		_flip32();	/* should be sed scripted */
#ifdef	NO_32_SHIFT
#define	_FLIP32(C)	_flip32((u32)(C))	/* no macro */
#else
#define	_FLIP32(C)	(u32)((((C)>>24)&0xFF)|(((C)>>8)&0xFF00)|(((C)<<8)&0xFF0000)|(((C)<<24)&0xFF000000))
#endif	NO_32_SHIFT
#else	L2R_32
#ifdef	R2L_32
	/* flips a byte pattern of 01020304 to 02010403 */
import	u32		_flip32();	/* should be sed scripted */
#ifdef	NO_32_SHIFT
#define	_FLIP32(C)	_flip32((u32)(C))	/* no macro */
#else
#define	_FLIP32(C)	(u32)((((C)>>8)&0xFF0000L)|(((C)>>8)&0xFFL)|(((C)<<8)&0xFF000000L)|(((C)<<8)&0xFF00L))
#endif	NO_32_SHIFT
#else	R2L_32
	/* flips a byte pattern of 01020304 to 01020304 (cast only) */
#define	_flip32(u)	(u32)(u)
#define	_FLIP32(C)	(u32)(C)
#endif	R2L_32
#endif	L2R_32

/* Conversions between axx, ixx and uxx types.  */
	
#define U16(a)		_flip16(*(u16 *)((a).a2))	/* a16 to u16 */
#define	I16(a)		(i16)(U16(a))			/* a16 to i16 */

#define	U32(a)		_flip32(*(u32 *)((a).a4))	/* a32 to u32 */
#define	I32(a)		(i32)(U32(a))			/* a32 to i32 */

/* axx Comparisons */

#define	A16A(a, op, b)	(U16(a) op U16(b))
#define	A32A(a, op, b)	(U32(a) op U32(b))

/* axx Operations */

#define	A16U(a, op, u)	(*(u16 *)(a).a2 op _flip16((u16)(u)))	/* a16 op u16 */
#define	A16C(a, op, C)	(*(u16 *)(a).a2 op _FLIP16(C))	/* a16 op u16 */

#define	A32U(a, op, u)	(*(u32 *)(a).a4 op _flip32((u32)(u)))	/* a32 op u32 */
#define	A32C(a, op, C)	(*(u32 *)(a).a4 op _FLIP32(C))	/* a32 op u32 */

#endif	_FLIP_

/*
 *	@(#)flip.h	11.2 (NRC)
 */
