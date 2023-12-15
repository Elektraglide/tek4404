/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.select.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:29  */

#ifndef	_SELECT_
#define	_SELECT_

#ifndef	_STD_
#include <net/std.h>
#endif

typedef struct sel {
	u16	se_flags;
	i16	se_fd;		/* holds process local file id */
	u32	se_dev;		/* holds kernel file id */
} sel;

#define	F_SE_READ	0x1	/* select if read completes */
#define	F_SE_CREAD	0x2	/* indicates read completed */
#define	F_SE_WRITE	0x4	/* select if write completes */
#define	F_SE_CWRITE	0x8	/* indicates write completed */
#define	F_SE_OTHER	0x10	/* select if other completes */
#define	F_SE_COTHER	0x20	/* indicates other completed */
#define	F_SE_MASK	0x3F

#ifdef	NATIVE_BSD_KERNEL
#define	F_SE_CONNECT	F_SE_READ
#define	F_SE_CCONNECT	F_SE_CREAD
#else
#define	F_SE_CONNECT	F_SE_OTHER
#define	F_SE_CCONNECT	F_SE_COTHER
#endif

#define	F_SE_FIRSTFREE	0x200	/* used as base for local flags */

#endif _SELECT_

/*
 *	@(#)select.h	11.1 (NRC)
 */
