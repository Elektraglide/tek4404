/*  Copyright (C) 5/3/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.q.h  */
/*  Version 11.2 as of 86/05/03 at 10:02:10  */

#ifndef	_Q_
#define	_Q_

/* Definitions and Macros for dealing with `q', `gq' and `mq' structures */

#ifndef	_STD_
#include <net/std.h>
#endif	_STD_

/* flags for q.q_flags */
#define	F_Q_HEADER	0x0001	/* denotes a queue head */
#define	F_Q_LOCKED	0x0002	/* don't mess with this right now */
#define	F_Q_ZAPPED	0x0004	/* this entry awaiting burial */
#define	F_Q_PRESERVE	0x0008	/* preserve across unlocks */
#define F_Q_BYTE_QUEUE	0x0800	/* byte-style vs. packet-style queue */

#define	is_header(ptr)		bT1(((q *)(ptr))->q_flags,F_Q_HEADER)
#define	is_zapped(ptr)		bT1(((q *)(ptr))->q_flags,F_Q_ZAPPED)
#define	is_locked(ptr)		bT1(((q *)(ptr))->q_flags,F_Q_LOCKED)
#define	q_empty(qp)		(((q *)(qp))->q_next == (q *)(qp))

typedef	struct mq {
	struct	m	* mq_hmp;	/* head link */
	struct	m	* mq_tmp;	/* tail link */
} mq;

typedef struct	q {
	struct	q	* q_next;	/* forward link */
	struct	q	* q_prev;	/* backward link */
		u16	q_flags;	/* used for locking, zapping etc. */
		u16	q_pad1;	
} q;

/* guarded q structure */
typedef struct gq {
	q	gq_q;		/* head and tail links */
	i16	gq_cnt;		/* how many elements are on this queue */
	i16	gq_max;		/* how many elements can be on this queue */
	i16	gq_sleeping;	/* how many procs are sleeping on this queue */
	i16	gq_status;	/* status to be passed up when awakened */
	struct	gq * gq_gqp;	/* used by select to forward wakeups/sleeps */
	char	* gq_ppad1;
} gq;

import	q	* q_in();

/* internal queue manipulation macros, to be used in critical section */
#define _q_init(qp)	((qp)->q_next=(qp),(qp)->q_prev=(qp))
#define _q_intail(a,b)	q_in((a)->q_prev, b)
#define	_q_rem(qp)	((qp)->q_next->q_prev=(qp)->q_prev,(qp)->q_prev->q_next=(qp)->q_next)

#endif	_Q_

/*
 *	@(#)q.h	11.2 (NRC)
 */
