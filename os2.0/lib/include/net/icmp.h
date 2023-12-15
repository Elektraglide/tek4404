/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.icmp.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:19  */

/* DARPA IP / Internet Control Message Protocol defines */

#ifndef	_ICMP_
#define	_ICMP_

#ifndef	_STD_
#include <net/std.h>
#endif

typedef	union {  /* miscellaneous fields */
	/* Unused */
	a32	icmp_uunused;
	/* Parameter Problem Message */
	struct {
		u8 icmp_upointer;
		u8 icmp_upu[3];	/* filler */
	}	icmp_uppm;
	/* Redirect Message */
	a32	icmp_urm;
	a32	icmp_ugia;
	/* Timestamp/Echo or Timestamp/Echo Reply Message */
	struct {
		a16 icmp_uid;  /* Identifier */
		a16 icmp_usn;  /* Sequence Number */
	}	icmp_uerm;
} icmp_u;

typedef	struct {  /* the ICMP header */
	u8	icmp_type;	/* message type */
	u8	icmp_code;	/* error code */
	a16	icmp_checksum;	/* checksum */
	icmp_u	icmp_misc;	/* miscellaneous fields */
} icmp_t;

#define	icmp_unused	icmp_misc.icmp_uunused
#define	icmp_pointer	icmp_misc.icmp_uppm.icmp_upointer
#define	icmp_rm		icmp_misc.icmp_urm
#define	icmp_id		icmp_misc.icmp_uerm.icmp_uid
#define	icmp_sn		icmp_misc.icmp_uerm.icmp_usn
#define	icmp_gia	icmp_misc.icmp_ugia

/* ICMP type values */

#define	ICMP_DUR	3	/* destination unreachable message */
#define ICMP_TE		11	/* time exceeded message */
#define	ICMP_PP		12	/* parameter problem message */
#define	ICMP_SQ		4	/* source quench message */
#define	ICMP_RED	5	/* redirect message */
#define	ICMP_ECH	8	/* echo message */
#define	ICMP_ECR	0	/* echo reply message */
#define	ICMP_TIM	13	/* timestamp message */
#define	ICMP_TMR	14	/* timestamp reply */
#define	ICMP_IN		15	/* information request */
#define	ICMP_INR	16	/* information reply */

/* ICMP code values */

#define	DUR_NET		0	/* network unreachable */
#define	DUR_HOST	1	/* host unreachable */
#define	DUR_PROTO  	2	/* protocol unreachable */
#define	DUR_PORT	3	/* port unreachable */
#define	DUR_FRAG	4	/* fragmentation needed & DF set */
#define	DUR_ROUTE	5	/* source route failed */
#define	TE_TRANSIT	0	/* time to live exceeded in transit */
#define	TE_REASSEMBLY	1	/* fragment reassembly time exceeded */
#define	PP_PERR		0	/* pointer indicates the error */
#define	RED_NET		0	/* redirect datagrams for the network */
#define	RED_HOST	1	/* redirect datagrams for the host */
#define	RED_NTOS	2	/* redirect datagrams for the tos & network */
#define	RED_HTOS	3	/* redirect datagrams for the tos & host */

#ifdef	KERNEL

import	st	icmp_msg();
#define	icmp_er(mp, type, code, x, y) ((mp)->m_p0 = (type), (mp)->m_p1 = (code), (mp)->m_p2 = (x), (mp)->m_p3 = (y), (st)icmp_msg)

#endif	KERNEL

#endif	_ICMP_

/*
 *	@(#)icmp.h	10.1 (NRC)
 */
