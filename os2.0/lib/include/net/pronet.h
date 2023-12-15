/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.pronet.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:17  */

#define	PN_VERSION	2	/* as laid out by Proteon */
#define	PN_MAX_PACKET	2046	/* max packet for proteon boards */
				/* max data for proteon boards */
#define	PN_MAX_DATA	(PN_MAX_PACKET-sizeof(proh))

/* Proteon link layer header */
typedef struct {
	char	proh_dest;
	char	proh_src;
	char	proh_version;
	char	proh_protocol;
	u16	proh_reserved;
} proh;

#define	PRONET_RAW	0

#define	PN_IP		1	/* DARPA Internet Protocol Packet */
#define	PN_ARP		3	/* Address Resolution Protocol Packet */
#define	PN_XNS		14	/* XNS Packet */ 
#define	PN_DIAGNOSTIC	15	/* Diagnostic Packet (ignored by FUSION) */ 

#define	isbroadcast(addr)	((addr) == (char)0xff)

/*
 *	@(#)pronet.h	11.1 (NRC)
 */
