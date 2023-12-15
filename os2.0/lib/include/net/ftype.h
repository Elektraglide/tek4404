/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.ftype.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:27  */

#ifndef _FTYPE_
#define	_FTYPE_

#ifndef	B42
#ifndef DST_NONE

/* types used in Fusion code for non-4.2bsd systems */

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

#define	DST_NONE	0
#define	DST_USA		1

#endif DST_NONE
#endif	B42

#endif _FTYPE_

/*
 *	@(#)ftype.h	11.1 (NRC)
 */
