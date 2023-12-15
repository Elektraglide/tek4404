/*  Copyright (C) 3/8/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.ip.h  */
/*  Version 11.3 as of 86/03/08 at 06:26:57  */

/* DARPA Internet Protocol defines */

#ifndef	_IP_
#define	_IP_

#ifndef	_STD_
#include <net/std.h>
#endif

/* ULP pseudo IP header */
typedef	struct {
	/* The order is not correct, but it doesn't matter for the checksum.
	 * This was designed to lay over the IP header overlay.
	 */
	u8	piph_zero;	/* zero byte */
	u8	piph_protocol;	/* Upper Level Protocol */
	a16	piph_length;	/* ULP length */
	a32	piph_saddr;	/* source address */
	a32	piph_daddr;	/* destination address */
} piph_t;

/* IP header */
typedef	struct {
	u8	iph_ihl;	/* version and IP header length */
	u8	iph_tos;	/* type of service */
	a16	iph_tlength;	/* total length */
	a16	iph_ident;	/* identification */
	a16	iph_fragoff;	/* flags and fragmentation offset */
	piph_t	iph_piph;	/* Pseudo IP Header */
} iph_t;

/* defines to map rest of IP header on ULP pseudo header */
#define	iph_ttl		iph_piph.piph_zero
#define	iph_protocol	iph_piph.piph_protocol
#define	iph_checksum	iph_piph.piph_length
#define	iph_saddr	iph_piph.piph_saddr
#define	iph_daddr	iph_piph.piph_daddr

/* iph_ihl fields */

#define	IP_V4		4	/* IP version 4 */
#define	IP_VER		0xF0	/* IP version mask */

/* macro to extract IP version from IP header pointer */
#define	iphv(iphp)	(((iphp)->iph_ihl & IP_VER) >> 4)

#define	IP_IHL		0x0F	/* IP header length mask */

/* macro to extract IP header length from IP header pointer */
#define	iphl(iphp)	(((iphp)->iph_ihl & IP_IHL) << 2)

#define	IP_MHL		60	/* max header length of any IP protocol */

/* bit fields in iph_tos */

#define	IP_PRECEDENCE	0xE0	/* top 3 bits of 'iph_tos' */
#define	IP_DELAY	0x10	/* delay */
#define	IP_THROUGHPUT	0x08	/* throughput */
#define	IP_RELIABLE	0x04	/* reliability */

/* IP socket security option */
typedef	struct {
	a16	ipos_security;		/* security */
	a16	ipos_compartments;	/* compartments */
	a16	ipos_handling;		/* handling restrictions */
	a32	ipos_tcc;		/* transmission control code (m.s. 24 bits) */
} ipos_t;

/* Precedence values (masked by IP_PRECEDENCE) */
#define	IP_NC		0xE0	/* Network Control */
#define IP_IC		0xC0	/* Internetwork Control */
#define	IP_CRITIC	0xA0	/* CRITIC/ECP */
#define	IP_FLSHO	0x80	/* Flash Overide */
#define	IP_FLASH	0x60	/* Flash */
#define	IP_IMMED	0x40	/* Immediate */
#define	IP_PRIORITY	0x20	/* Priority */
#define	IP_ROUTINE	0x00	/* Routine */

/* bit fields in iph_fragoff */
#define	IP_RS		0x8000	/* reserved - must be off */
#define	IP_DF		0x4000	/* Don't Fragment flag */
#define	IP_MF		0x2000	/* More Fragments flag */
#define	IP_OFFSET	0x1FFF	/* Fragment Offset in segment */

/* defaults */
#define	IP_DTOS		0	/* default type of service */
#ifndef TEK4404
#define	IP_DTTL	3	/* default time to live (3 seconds) (Ethernet based) */
#else
#define	IP_DTTL	60	/* B42 decreases that value by 5, in one shot! */
#endif TEK4404
#define	IP_MAXTTL	255	/* maximum packet life (4:15 minutes) */

/* IP protocol codes */
#define	IP_IP		0	/* Internet Protocol (for use with get/setopt only) */
#define	IP_ICMP		1	/* Internet Control Message Protocol */
#define	IP_TCP		6	/* Transaction Control Protocol */
#define	IP_UDP		17	/* User Datagram Protocol */

/* Bit fields of iph_saddr & iph_daddr */

#define	INADDR_ANY	((u32)0)	/* unknown, or broadcast address */

#define	MASK_A_CLASS	((u32)0x80000000)
#define	MASK_B_CLASS	((u32)0xC0000000)
#define	MASK_C_CLASS	((u32)0xE0000000)
#define	MASK_EX_CLASS	((u32)0xE0000000)
#define A_CLASS		((u32)0x00000000)
#define B_CLASS		((u32)0x80000000)
#define C_CLASS		((u32)0xC0000000)
#define EX_CLASS	((u32)0xE0000000)
#define	NET_A_CLASS	((u32)0xFF000000)
#define NET_B_CLASS	((u32)0xFFFF0000)
#define NET_C_CLASS	((u32)0xFFFFFF00)
#define	HOST_A_CLASS	((u32)0x00FFFFFF)
#define	HOST_B_CLASS	((u32)0x0000FFFF)
#define	HOST_C_CLASS	((u32)0x000000FF)

/* defines for IP level options */

#define	IPO_CF		0x80	/* copy flag */
#define	IPO_CLASS	0x60	/* option class */
#define	IPO_NUMBER	0x1F	/* option number */

#define	IPO_EOL		0x00	/* end of option list */
#define	IPO_NOP		0x01	/* 'u16' filler */
#define	IPO_SECURITY	0x02	/* security:compartment:handling:tcc */
#define	IPO_LSR		0x03	/* loose source and record route */
#define	IPO_SSR		0x09	/* strict source and record route */
#define	IPO_RR		0x07	/* record route */
#define	IPO_SID		0x80	/* stream identifier */
#define	IPO_TS		0x04	/* Internet timestamp */

#endif	_IP_

/*
 *	@(#)ip.h	11.3 (NRC)
 */
