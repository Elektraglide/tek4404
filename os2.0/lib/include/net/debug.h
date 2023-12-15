/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.debug.h  */
/*  Version 11.1 as of 86/01/28 at 09:32:59  */

#ifndef	_DEBUG_
#define	_DEBUG_

#ifdef	DEBUG
#ifdef	KERNEL
#define _p0(m,c,f)	if(c) {os_printf("m: "); os_printf(f);} else
#define _p1(m,c,f,z)	if(c) {os_printf("m: "); os_printf(f,z);} else
#define _p2(m,c,f,z,y)	if(c) {os_printf("m: "); os_printf(f,z,y);} else
#define _p3(m,c,f,z,y,x)	if(c) {os_printf("m: "); os_printf(f,z,y,x);} else
#define _p4(m,c,f,z,y,x,w)	if(c) {os_printf("m: "); os_printf(f,z,y,x,w);} else
#else
#define _p0(m,c,f)	if(c) {printf("m: "); printf(f);} else
#define _p1(m,c,f,z)	if(c) {printf("m: "); printf(f,z);} else
#define _p2(m,c,f,z,y)	if(c) {printf("m: "); printf(f,z,y);} else
#define _p3(m,c,f,z,y,x)	if(c) {printf("m: "); printf(f,z,y,x);} else
#define _p4(m,c,f,z,y,x,w)	if(c) {printf("m: "); printf(f,z,y,x,w);} else
#endif	KERNEL
#define debug0(c,f)	_p0(debug,(c),(f))
#define debug1(c,f,z)	_p1(debug,(c),(f),(z))
#define debug2(c,f,z,y)	_p2(debug,(c),(f),(z),(y))
#define debug3(c,f,z,y,x)	_p3(debug,(c),(f),(z),(y),(x))
#define debug4(c,f,z,y,x,w)	_p4(debug,(c),(f),(z),(y),(x),(w))
#define trace0(c,f)	_p0(trace,(c),(f))
#define trace1(c,f,z)	_p1(trace,(c),(f),(z))
#define trace2(c,f,z,y)	_p2(trace,(c),(f),(z),(y))
#define trace3(c,f,z,y,x)	_p3(trace,(c),(f),(z),(y),(x))
#define trace4(c,f,z,y,x,w)	_p4(trace,(c),(f),(z),(y),(x),(w))
#else	DEBUG
#define	debug0(c,f)
#define debug1(c,f,z)
#define debug2(c,f,z,y)
#define debug3(c,f,z,y,x)
#define debug4(c,f,z,y,x,w)
#define	trace0(c,f)
#define trace1(c,f,z)
#define trace2(c,f,z,y)
#define trace3(c,f,z,y,x)
#define trace4(c,f,z,y,x,w)
#endif	DEBUG

#ifdef	SKEPTIC
#ifdef	KERNEL
#define	assert(c,m)	if (!(c)) os_panic(m); else
#define verify(c,f,act)	if (!(c)) {os_printf(f); act;} else
#else
#define	assert(c,m)	if (!(c)) {printf(m); exit(BAD_EXIT);} else
#define verify(c,f,act)	if (!(c)) {printf(f); act;} else
#endif	KERNEL
#else	SKEPTIC
#define assert(c,m)
#define verify(c,f,act)
#endif	SKEPTIC

#endif	_DEBUG_

/*
 *	@(#)debug.h	11.1 (NRC)
 */
