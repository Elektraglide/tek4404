/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.udp.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:58  */

#ifndef	_UDP_
#define	_UDP_

/* DARPA IP / User Datagram Protocol defines */

/* the real UDP header */
typedef	struct {
	a16	udph_sport;	/* source port number */
	a16	udph_dport;	/* destination port number */
	a16	udph_length;	/* bytes in header + data */
	a16	udph_checksum;	/* optional checksum */
} udph_t;

#define	UDP_ROUTE_PORT	520	/* the NIP socket number */

#ifdef	KERNEL
#define	UDP_MHR	(sizeof(udph_t) + 1024)	/* max bytes in a UDP header + data */
#define	UDP_MIN_FREE	256	/* see page(417) */
#endif	KERNEL

#endif _UDP_

/*
 *	@(#)udp.h	11.1 (NRC)
 */
