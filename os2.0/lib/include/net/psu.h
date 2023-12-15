/*  Copyright (C) 3/8/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.psu.h  */
/*  Version 11.3 as of 86/03/08 at 06:35:29  */

#ifndef	_PSU_
#define	_PSU_

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>	/* for <tcb.h> */
#endif
#ifndef	_TCB_
#include <net/tcb.h>
#endif

/* Protocol specific structures and their union */

/* Transmission Control Protocol Specific Union */
typedef	struct {
	struct tcpsv_t	* tsv_s;	/* pointer to the TCP state vector */
} tcppsu_t;

/* IP security compartment */
typedef	struct {
	a16	ips_sec;	/* security */
	a16	ips_cmp;	/* compartments */
	a16	ips_hr;		/* handling restrictions */
	a32	ips_tcc;	/* transmission control code (24 bits) */
} ips_t;

/* Internet Protocol Specific Union */
typedef struct {
	u8	ipsu_ttl;	/* ttl for outgoing packets */
	ips_t	ipsu_sec;	/* security:compartment:handling:tcc */
} ipsu_t;

/* Darpa Protocols Specific Union */
typedef	struct {
	ipsu_t	dpsu_ip;		/* IP */
	union {	/* union of darpa protocols */
		tcppsu_t	dpsu_utcp;	/* TCP */
	} dpsu_u;
} dpsu_t;

/* Xerox sequenced packet state */
typedef struct xst {
	u16	xst_flags;	/* see defines in <xns.h> */
	u16	xst_seq_num;	/* the next send side seq num */
	u16	xst_s_c_id;	/* source connection id */
	u16	xst_d_c_id;	/* destination connection id */
	u16	xst_l_c_id;	/* last connection id for listener */
	u16	xst_expect;	/* expected recv side seq num */
	a48	xst_l_host;	/* last connection's host id */
	i16	xst_sq_max;	/* send q max */
	i16	xst_rq_max;	/* recv q max */
	i16	xst_countdown;	/* retransmission/abort countdown */
	u32	xst_delay;
	u16	xst_tperiod;
	u16	xst_timer;
	u16	xst_clocked;
	u16	xst_first_unacked;
	u16	xst_last_alloc;
	u16	xst_window;
	tcb	xst_tcb;	/* timer control block */
} xst;

/* Xerox packet exchange state */
typedef struct xpes {
	i32	xpes_countdown;
	u32	xpes_id;
	tcb	xpes_tcb;
} xpes;

/* Xerox base reliable message protocol */
typedef	struct xrs {
	xna	x_src;
	u16	x_packet_id;
	u16	x_age;
} xrs;

typedef struct xrds {
	i16	xrt_errindex;
	i16	xrt_pad1;
	tcb	xrt_tcb;
	xrs	xrt_xrsa[5];
} xrds;

/* Intra machine protocol */
typedef	struct	iasu {
	u32	ia_flags;
	u32	ia_id;
	st	ia_rfn;		/* fn to place into receive queue */
	char	* ia_ppad1;
} iasu;

#ifdef	XLL_PROTOCOL
typedef struct pllsu {
	struct	netdev	* ps_ndp;	/* ptr. to bound netdev */
	pfi_t	ps_sendsav;		/* prev. contents of ndp->nd_send */
	u32	ps_netidsav;		/* prev. contents of ndp->nd_netid */
	u32	ps_flagsav;		/* prev. contents of ndp->nd_flags */
} pllsu;
#else
#define	pllsu	int
#endif

typedef union {
	iasu	ia_state;
	dpsu_t	dpu_state;
	pllsu	pll_state;
	xpes	xpe_state;
	xrds	xrd_state;
	xst	xsp_state;
} psu;

#endif	_PSU_

/*
 *	@(#)psu.h	11.3 (NRC)
 */
