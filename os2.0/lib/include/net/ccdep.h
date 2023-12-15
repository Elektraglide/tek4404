/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.ccdep.h  */
/*  Version 11.1 as of 86/01/28 at 09:32:53  */

#ifndef	_CCDEP_
#define	_CCDEP_

/* 'register' storage class macro */
#ifdef	NO_FAST
#define	fast	/**/
#else
#define	fast	register
#endif

/* structure assignment macro */
#ifdef	NO_STASS
#define	stass(d,s)	sdcp((char *)&(s), (char *)&(d), sizeof(s))
#else
#define	stass(d,s)	((d) = (s))
#endif

/* local 'static' storage class macro */
#ifdef	NO_STATIC_DATA
#define	static	"static data NOT supported. Have fun . . ."
#endif

/* global 'static' storage class macro */
#ifdef	NO_GLOBAL_STATIC
#define	local	/**/
#else
#define	local	static
#endif

/* for compiler's that can't deal with intra segment pointers */
#ifdef	NO_PFI_STATICS
#define	pflocal	/**/
#else
#define	pflocal	local
#endif

/* for compiler's that can't deal with unsigned char's */
#ifdef	NO_UCHAR
#define	uchar	char
#else
#define	uchar	unsigned char
#endif
#define	u8	uchar

/* 'void' type macro */
#ifdef	NO_VOID
#define	void	int
#endif

/* primitive types for machines w/ 32-bit 'int's */
#ifdef	INT_32
#define	i16	short
#define	u16	unsigned short
#define	i32	int
#define	u32	unsigned
#define	INT_SHIFT	2
#endif

/* primitive types for machines w/ 16-bit 'int's */
#ifdef	INT_16
#define	i16	int
#define	u16	unsigned
#define	i32	long
/* if no 'unsigned long', use 'long' */
#ifdef	NO_ULONG
#define	u32	long
#else
#define	u32	unsigned long
#endif
#define	INT_SHIFT	1
#endif	INT_16

/* fast register types */
#ifdef	FAST16
#define ifast	i16
#define ufast	u16
#else	FAST16
#define ifast	int
#define ufast	unsigned
#endif	FAST16

typedef	union any_32 {
	u32	a32_u;
	i32	a32_i;
	u16	a32_ua[2];
	i16	a32_ia[2];
	char	a32_ca[4];
} any_32;

typedef union any_16 {
	u16	a16_u;
	i16	a16_i;
	char	a16_ca[2];
} any_16;

/* bit field macros, bit clear, set, true, false */
#define bC1(v,f)	((v) &= ~(f))
#define bS1(v,f)	((v) |= (f))
#define bT1(v,f)	((v) & (f))
#define bF1(v,f)	(((v) & (f)) == 0)
#ifdef	BITOPS
#define bC2(v,f1,f2)	(bC1(v,f1), bC1(v,f2), v)
#define bC3(v,f1,f2,f3)	(bC1(v,f1), bC1(v,f2), bC1(v,f3), v)
#define bS2(v,f1,f2)	(bS1(v,f1), bS1(v,f2), v)
#define bS3(v,f1,f2,f3)	(bS1(v,f1), bS1(v,f2), bS1(v,f3), v)
#define bT2(v,f1,f2)	(bT1(v,f1) && bT1(v,f2))
#define bT3(v,f1,f2,f3)	(bT1(v,f1) && bT1(v,f2) && bT1(v,f3))
#define bF2(v,f1,f2)	(bF1(v,f1) && bF1(v,f2))
#define bF3(v,f1,f2,f3)	(bF1(v,f1) && bF1(v,f2) && bF1(v,f3))
#else	BITOPS
#define bC2(v,f1,f2)	bC1(v, ((f1)|(f2)))
#define bC3(v,f1,f2,f3)	bC1(v, ((f1)|(f2)|(f3)))
#define bS2(v,f1,f2)	bS1(v, ((f1)|(f2)))
#define bS3(v,f1,f2,f3)	bS1(v, ((f1)|(f2)|(f3)))
#define bT2(v,f1,f2)	(((v) & ((f1)|(f2))) == ((f1)|(f2)))
#define bT3(v,f1,f2,f3)	(((v) & ((f1)|(f2)|(f3))) == ((f1)|(f2)|(f3)))
#define bF2(v,f1,f2)	bF1(v, ((f1)|(f2)))
#define bF3(v,f1,f2,f3)	bF1(v, ((f1)|(f2)|(f3)))
#endif	BITOPS

/* definitions allowing explicit precedence of 'fast' definitions */
#ifdef	UNISOFTV7
/* not quite accurate due to partitioned register set */
#define	fast_1	fast
#define	fast_2	fast
#define fast_3	fast
#define fast_4	fast
#define fast_5	fast
#define fast_6	fast
#define fast_7	fast
#define	fast_8	fast
#define fast_9	fast
#define fast_10	/**/
#define fast_11	/**/
#define fast_12	/**/
#define fast_13	/**/
#define fast_14	/**/
#define fast_15	/**/
#define fast_16	/**/
#define fast_n	/**/
#ifdef	WICAT
#undef fast_10
#define fast_10	fast
#endif	WICAT
#endif	UNISOFTV7

#ifndef	fast_1
/* We all wish we had a C/70! */
#define	fast_1	fast
#define	fast_2	fast
#define fast_3	fast
#define fast_4	fast
#define fast_5	fast
#define fast_6	fast
#define fast_7	fast
#define	fast_8	fast
#define fast_9	fast
#define fast_10	fast
#define fast_11	fast
#define fast_12	fast
#define fast_13	fast
#define fast_14	fast
#define fast_15	fast
#define fast_16	fast
#define fast_n	fast
#endif	fast_1

#endif	_CCDEP_

/*
 *	@(#)ccdep.h	11.1 (NRC)
 */
