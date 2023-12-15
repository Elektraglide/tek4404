/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.modulo.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:57  */

/* Macros to do modular arithmetic */

#ifndef _MODULO_
#define _MODULO_

#ifndef	_STD_
#include <net/std.h>
#endif

/* This file will define macros to do 8, 16 or 32-bit modular arithmetic.
 * The mxx structures are defined in such a way that once a variable is
 * defined as an mxx type, it cannot be used in the same fashion that
 * normal variables (like ints, u16's, i32's etc.).  They must be dealt with
 * using the appropriate macros from this file.  Hence a variable defined
 * to be of the modular-type, cannot be accidentally confused or used with
 * ordinary types.
 *
 * To obtain usage of these macros, along with including this file, the
 * following preprocessor defines must be defined before the inclusion :
 *
 * #define MOD8		- defines the modulo-8 macros.
 * #define MOD16	- defines the modulo-16 macros.
 * #define MOD32	- defines the modulo-32 macros.
 */

/* Modular Arithmetic types */

#ifdef	MOD8
typedef struct m8 {		/* a 8-bit host-ordered structure */
	char	m1[1];
} m8;
#endif

#ifdef	MOD16
typedef struct m16 {		/* a 16-bit host-ordered structure */
	char	m2[2];
} m16;
#endif

#ifdef	MOD32
typedef struct m32 {		/* a 32-bit host-ordered structure */
	char	m4[4];
} m32;
#endif

/* Conversions between mxx, ixx and uxx types */
	
#ifdef	MOD8
#define MU8(m)		*(u8 *)((m).m1)			/* m8 to u8 */
#define	MI8(m)		(char)(MU8(a))			/* m8 to char */
#endif

#ifdef	MOD16
#define MU16(m)		*(u16 *)((m).m2)		/* m16 to u16 */
#define	MI16(m)		(i16)(MU16(a))			/* m16 to i16 */
#endif

#ifdef	MOD32
#define MU32(m)		*(u32 *)((m).m4)		/* m32 to u32 */
#define	MI32(m)		(i32)(MU32(a))			/* m32 to i32 */
#endif

/* The basic modular arithmetic macro */

#define	MODULO(a, op, b) ((int)((a) - (b)) op 0)
#define	MODULO32(a, op, b) ((i32)((a) - (b)) op 0L)

/* Implementation Note :
 *	There is some concern as to whether or not the preceeding macro
 *	will work across all processor models.  Those with 16-bit structures
 *	may encounter difficulty with the 'int' cast.  The purpose is to
 *	sign-extend the result of the subtraction for comparision with 0.
 */

/* mxx Comparisons [ <, <=, ==, !=, >=, > ] */

#ifdef	MOD8
#define MC8M(a, cop, b)		MODULO(MU8(a), cop, MU8(b))
#define MC8U(m, cop, u)		MODULO(MU8(m), cop, (u8)(u))
#define UC8M(u, cop, m)		MODULO((u8)(u), cop, MU8(m))
#endif

#ifdef	MOD16
#define MC16M(a, cop, b)	MODULO(MU16(a), cop, MU16(b))
#define MC16U(m, cop, u)	MODULO(MU16(m), cop, (u16)(u))
#define UC16M(u, cop, m)	MODULO((u16)(u), cop, MU16(m))
#endif

#ifdef	MOD32
#define MC32M(a, cop, b)	MODULO32(MU32(a), cop, MU32(b))
#define MC32U(m, cop, u)	MODULO32(MU32(m), cop, (u32)(u))
#define UC32M(u, cop, m)	MODULO32((u32)(u), cop, MU32(m))
#endif

/* mxx Operations [ +, -, =, +=, -=, etc. ] */

#ifdef	MOD8
#define	M8M(a, op, b)	(MU8(a) op MU8(b))
#define	M8U(m, op, u)	(MU8(m) op (u8)(u))
#define	U8M(u, op, m)	((u8)(u) op MU8(m))
#endif

#ifdef	MOD16
#define	M16M(a, op, b)	(MU16(a) op MU16(b))
#define	M16U(m, op, u)	(MU16(m) op (u16)(u))
#define	U16M(u, op, m)	((u16)(u) op MU16(m))
#endif

#ifdef	MOD32
#define	M32M(a, op, b)	(MU32(a) op MU32(b))
#define	M32U(m, op, u)	(MU32(m) op (u32)(u))
#define	U32M(u, op, m)	((u32)(u) op MU32(m))
#endif

#endif _MODULO_

/*
 *	@(#)modulo.h	11.1 (NRC)
 */
