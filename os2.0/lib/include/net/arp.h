/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.arp.h  */
/*  Version 11.1 as of 86/01/28 at 09:32:50  */

/* arp.h - Address Resolution Protocol */

#ifndef	_ARP_
#define	_ARP_

#ifndef	_STD_
#include <net/std.h>
#endif

/* Arp Link Layer Types (Not to be confused with link layer protocol types) */
#define	ARP_UNSPEC	0	/* who knows.. (defined by NRC) */
#define	ARP_EN		1	/* Ethernet (defined by D.C.P.) */
#define	ARP_PN		2	/* Pronet (defined by NRC) */
#define	ARP_XLL		3	/* External Link Layer (defined by NRC) */

/* Opcodes */

#define ARP_REQUEST	0x1
#define	ARP_REPLY	0x2

typedef struct arp_t {
	a16	ar_ltype;
	a16	ar_ptype;
	u8	ar_llen;
	u8	ar_plen;
	a16	ar_opcode;
/* variable length addresses */
/*	link layer source address (ar_llen) */
/*	protocol source address (ar_plen) */
/*	link layer destination address (ar_llen) */
/*	protocol destination address (ar_plen) */
} arp_t;

#define LINK_SRC_ADDR(arp)	(((char *)arp) + sizeof(*arp))
#define	PROTO_SRC_ADDR(arp)	(LINK_SRC_ADDR(arp) + (arp->ar_llen & 0xFF))
#define	LINK_DEST_ADDR(arp)	(PROTO_SRC_ADDR(arp) + (arp->ar_plen & 0xFF))	
#define	PROTO_DEST_ADDR(arp)	(LINK_DEST_ADDR(arp) + (arp->ar_llen & 0xFF))

#endif	_ARP_

/*
 *	@(#)arp.h	11.1 (NRC)
 */
