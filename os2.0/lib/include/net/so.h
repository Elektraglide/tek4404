/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.so.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:34  */

#ifndef	_SO_
#define	_SO_

/* Definitions and Macros for dealing with the `so_t' structure */

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif
#ifndef	_TCB_
#include <net/tcb.h> /* for <psu.h> */
#endif
#ifndef	_PSU_
#include <net/psu.h>
#endif

/* flags for sock_t.so_flags */
#define	F_SO_OPEN	0x0001
#define	F_SO_CONNECTED	0x0002
/*			*/
#define	F_SO_CLOSING	0x0010
#define	F_SO_RSHUTDOWN	0x0020		/* reading shutdown */
#define	F_SO_WSHUTDOWN	0x0040		/* writing shutdown */
#define	F_SO_CLSDELAY	0x0080		/* actual close will be delayed */
#define	F_SO_KILL	0x0100		/* remove when unlocked */

#define	is_closing(sop)		bT1((sop)->so_flags,F_SO_CLOSING)
#define	is_connected(sop)	bT1((sop)->so_flags,F_SO_CONNECTED)
#define	is_open(sop)		bT1((sop)->so_flags,F_SO_OPEN)
#define	is_packet(sop)		(!is_stream(sop))
#define	is_stream(sop)		((sop)->so_prp->pr_type == SOCK_STREAM)
#define	is_rshutdown(sop)	bT1((sop)->so_flags,F_SO_RSHUTDOWN)
#define	is_wshutdown(sop)	bT1((sop)->so_flags,F_SO_WSHUTDOWN)
#define	is_info_socket(sop)	((sop)->so_index == 0)
#define	needs_connect(sop)	bT1((sop)->so_prp->pr_flags,F_PR_NEEDS_CONNECT)

#define	so_cqhead(sop,errp,wait_ok)	((m *)gq_head(&((sop)->so_cq),errp,wait_ok))
#define	so_cqin(sop,mp)		so_qin(sop,&(sop)->so_cq,mp)
#define	so_hqin(sop,mp)		so_qin(sop,&(sop)->so_hq,mp)
#define	so_rqhead(sop,errp,wait_ok)	((m *)gq_head(&((sop)->so_rq),errp,wait_ok))
#define	so_rqin(sop,mp)		so_qin(sop,&(sop)->so_rq,mp)
#define	so_rqwaiting(sop)	((sop)->so_rq.gq_sleeping)
#define	so_rqout(sop)		gq_out(&(sop)->so_rq,1)

/* socket */
typedef struct so_t {
	q	so_q;			/* for protocol/family use */
	gq	so_cq;			/* connection queue */
	gq	so_sq;			/* send queue */
	gq	so_rq;			/* receive queue */
	gq	so_hq;			/* holding queue */
	so_addr	so_src;			/* our network address (sans net #) */
	so_addr	so_dest;		/* where we're going to */
	secure	so_secure;		/* security structure */
	struct	pr_t	* so_prp;	/* protocol entrypoint pointer */
	struct	netdev	* so_ndp;	/* Link for RAW stuff */
	struct	so_t	* so_pair;	/* paired socket */
	struct	os	* so_osp;	/* os specific structure (os.h) */
	u16	so_flags;		/* socket flags */
	u16	so_index;		/* this sockets index */
	u16	so_psize;		/* maximum packet size */
	u16	so_hsize;		/* maximum header size */
	u16	so_options;		/* socket option flags */
	u16	so_poptions;		/* protocol specific option flags */
	char	so_signal[4];		/* signals we want on async activity */
	char	so_fsig[4];		/* if signals are armed */
	int	so_err;
	u32	so_rtrkey;		/* router key (cache) */
	psu	so_psu;			/* protocol specific union */
} so_t;

#endif	_SO_

/*
 *	@(#)so.h	11.1 (NRC)
 */
