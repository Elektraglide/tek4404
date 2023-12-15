/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.frctlm.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:22  */

/* FUSION router control message structures */

#ifndef _FRCTLM_
#define	_FRCTLM_

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif

/* Router control message format; provided to router processing by
 * kernel as a result of control protocols (e.g. ICMP, XERP)
 * and internal events such as the death of a device or network.
 */
typedef struct frctlh {  /* control message header */
	u32	rech_errno;	/* error value in echoed message */
	u16	rech_vsn;	/* version */
	u16	rech_type;	/* control message type (see below) */
	u16	rech_seqno;	/* sequence number if desired */
	u16	rech_1junk;	/* pad */
} frctlh;

typedef struct frc1 {  /* single route values */
	u16	rec1_u1;	/* device index (in 'ndevsw') */
	u16	rec1_u2;	/* hop cnt or max packet size */
	u16	rec1_u3;	/* flags (masked inside) */
	u16	rec1_u4;	/* time-to-live (in sec) */
	u32	rec1_u5;	/* delay (in msec) */
	rte	rec1_rte;	/* route key */
} frc1;

#ifdef	WANT_ROUTER_PROCESS

typedef struct frcc {  /* character array values */
	u16	recc_reno;	/* first router entry index in bit map */
	char	recc_ca[2];	/* router entries referenced */
} frcc;

typedef struct frcr {  /* indexed router entry */
	u16	recr_reno;	/* router entry index */
	u16	recr_2junk;	/* pad */
	frent	recr_re;	/* router entry */
} frcr;

#endif	WANT_ROUTER_PROCESS

typedef struct frctlm {  /* master structure for control messages */
	frctlh	recm_h;		/* header */
	union {
		frc1	recm_c1;
#ifdef	WANT_ROUTER_PROCESS
		frcc	recm_cc;
		frcr	recm_cr;
#endif	WANT_ROUTER_PROCESS
	} recm_u;
} frctlm;

/* some cuties to allow easy access to control message components */
#define recm_errno	recm_h.rech_errno
#define recm_vsn	recm_h.rech_vsn
#define recm_type	recm_h.rech_type
#define recm_seqno	recm_h.rech_seqno
#define recm_addr	recm_u.recm_c1.rec1_rte.rte_gwy
#define recm_rte	recm_u.recm_c1.rec1_rte
#define recm_net		recm_rte.rte_net
#define recm_gwy		recm_rte.rte_gwy
#define recm_secure		recm_rte.rte_secure
#define recm_u1		recm_u.recm_c1.rec1_u1
#define recm_nd			recm_u1
#define	recm_u2		recm_u.recm_c1.rec1_u2
#define recm_hops		recm_u2
#define recm_maxpkt		recm_u2
#define recm_u3		recm_u.recm_c1.rec1_u3
#define recm_flags		recm_u3
#define recm_u4		recm_u.recm_c1.rec1_u4
#define recm_ttl		recm_u4
#define recm_u5		recm_u.recm_c1.rec1_u5
#define recm_delay		recm_u5
#ifdef	WANT_ROUTER_PROCESS
#define recm_reno	recm_u.recm_cc.recc_reno
#define recm_ca		recm_u.recm_cc.recc_ca
#define recm_re		recm_u.recm_cr.recr_re
#endif	WANT_ROUTER_PROCESS

#define FR_C_VSN	0x200	/* don't conflict w/ old 'recm_type' */

/* Control message types 'recm_type'.
 * In general, if the net number has its type == AF_UNSPEC, then the
 * gateway address must be valid and the route is presumed to refer
 * to a router server.  If the gateway address has its type == AF_UNSPEC,
 * then the net number must be valid and the route given is presumed
 * to be a directly connected net.
 */
#define FR_C_WANT_ECHO	0x8000	/* send back msg. w/ error number */
#define FR_C_DDOWN	1	/* take device down */
#define FR_C_DDAMAGE	2	/* device good at torture techniques */
#define FR_C_DUP	3	/* bring up device */
#define FR_C_RDOWN	4	/* route down */
#define FR_C_RDAMAGE	5	/* route flaky */
#define FR_C_RUP	6	/* add a new route */
#define FR_C_RSLOW	7	/* route overworked */
#define FR_C_RNARROW	8	/* route has packet size restriction */

#ifdef	WANT_ROUTER_PROCESS
#define FR_C_REFED	253	/* list of routes recently referenced */
#define FR_C_KREAD	254	/* get route from kernel slave table */
#define FR_C_KWRITE	255	/* put route into kernel slave table */
#endif	WANT_ROUTER_PROCESS

/* router control message flag values (internal values are in "fr.h") */
#define F_R_HARD_WIRED	0x0800	/* forced from outside */

#endif	_FRCTLM_

/*
 *	@(#)frctlm.h	11.1 (NRC)
 */
