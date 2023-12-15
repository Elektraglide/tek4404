/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.ntty.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:55  */

#ifndef _XNT_
#define	_XNT_

#ifndef	_STD_
#include <net/std.h>
#endif

/* net tty structure (xnt.c) */
typedef struct nt {
	struct	tty * nt_tp;		/* tty structure associated with us */
	char	* nt_buf;		/* output buffering area */
	u16	nt_ccnt;		/* output buffering count */
	u16	nt_flags;		/* nt state */
	i16	nt_unbuffered;		/* disables buffering (stackable) */
	i16	nt_sleeping;
	i16	nt_soindex;		/* sop that we're bound to */
	u16	nt_pad1;
} nt;

#define	is_real_owner()		(os_ppid() == 1)

#define	F_NT_INITOPEN		0x1
#define	F_NT_AVAILABLE		0x2
#define	F_NT_BETWEEN		0x4
#define	F_NT_ABORTING		0x8
#define	F_NT_DELAYCLOSE		0x10

#endif	_XNT_

/*
 *	@(#)ntty.h	10.1 (NRC)
 */
