/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.flags.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:02  */

#ifndef	_FLAGS_
#define	_FLAGS_

/* Definitions and Macros for dealing with common flags */

#ifndef	_STD_
#include <net/std.h>
#endif

/* flags reserved in all common flag fields */

#define	F_BLOCKING	MSG_BLOCKING	/* transaction can block */
#define	F_NONBLOCKING	MSG_NONBLOCKING	/* transaction cannot block */
#define	F_FDBROADCAST	MSG_FDBROADCAST /* want full duplex broadcasts */

/* blocking / status is a tri-state thing: F_BLOCKING means we want
   a status back and are willing and able to sleep; F_NONBLOCKING means
   we want a status back immediately without sleeping; neither of these
   means we don't care to see a status back, allowing queueing of processing
   at appropriate places. */

#define	F_BS_MASK	(F_BLOCKING|F_NONBLOCKING)
#define	F_COMM_MASK	(F_BS_MASK | F_FDBROADCAST)
#define	give_bs(f)	((f)&F_BS_MASK)
#define	no_bs(f)	bC2((f),F_BLOCKING,F_NONBLOCKING)
#define	F_NOBS		0		/* for clarity */

#define	can_block(flags)	bT1((flags),F_BLOCKING)
#define	cant_block(flags)	bF1((flags),F_BLOCKING)
#define	want_status(flags)	(!bF2((flags),F_BLOCKING,F_NONBLOCKING))

#endif	_FLAGS_

/*
 *	@(#)flags.h	10.1 (NRC)
 */
