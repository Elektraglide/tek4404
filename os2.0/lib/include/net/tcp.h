/*  Copyright (C) 5/5/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.tcp.h  */
/*  Version 11.9 as of 86/05/05 at 02:45:49  */

/* DARPA Transaction Control Protocol defines */

#ifndef	_TCP_
#define	_TCP_

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_FLIP_
#include <net/flip.h>
#endif
#ifndef	_SO_
#include <net/so.h>
#endif
#ifndef	_IP_
#include <net/ip.h>
#endif	_IP_

/* TCP header */
typedef	struct {
	a16	tcph_sport;	/* source port number */
	a16	tcph_dport;	/* destination port number */
	a32	tcph_seqno;	/* sequence number of first data byte */
	a32	tcph_ackno;	/* sequence number we next expect */
	a16	tcph_flags;	/* 4 bits data offset : 12 bits flags */
	a16	tcph_window;	/* how much data can be accepted now */
	a16	tcph_checksum;	/* ones complement checksum */
	a16	tcph_urgent;	/* urgent pointer */
} tcph_t;

/* Receive Maximum Segment Size TCP option */
typedef	struct {
	u8	tcpo_kind;
	u8	tcpo_size;
	a16	tcpo_mrss;
} tcpo_t;

/* flags in TCP header ('tcph_flags') */
#define	URG	((u16)0x0020)	/* URGent pointer is significant */
#define	ACK	((u16)0x0010)	/* ACKnowledgement field is significant */
#define	PSH	((u16)0x0008)	/* PuSH function active in this segment */
#define	RST	((u16)0x0004)	/* connection ReSeT directive */
#define	SYN	((u16)0x0002)	/* SYNchronize seq numbers directive */
#define	FIN	((u16)0x0001)	/* FINished, no more data from sender,
				   start close */

#define	FLAG_MASK	((u16)0x0FFF)	/* top 4 bits is data offset */
#define	RSVD_FLGS	((u16)0x0FC0)	/* these bits are reserved */

/* macros to test the states of flags */
#define	URG_ON(f)	bT1((f), URG)
#define	URG_OFF(f)	bF1((f), URG)
#define	ACK_ON(f)	bT1((f), ACK)
#define	ACK_OFF(f)	bF1((f), ACK)
#define	PSH_ON(f)	bT1((f), PSH)
#define	PSH_OFF(f)	bF1((f), PSH)
#define	RST_ON(f)	bT1((f), RST)
#define	RST_OFF(f)	bF1((f), RST)
#define	SYN_ON(f)	bT1((f), SYN)
#define	SYN_OFF(f)	bF1((f), SYN)
#define	FIN_ON(f)	bT1((f), FIN)
#define	FIN_OFF(f)	bF1((f), FIN)

/* length of TCP header */
#define	tcphl(tcphp)	((U16((tcphp)->tcph_flags) >> 10) & 0x3C)

#define	TCP_MHL		(sizeof(tcph_t)+sizeof(tcpo_t))	/* max bytes in a TCP header */

/* 9.3.11 Options. [92..3] */

#define	TCPO_EOL	0x00	/* End of option list. */
#define	TCPO_NOP	0x01	/* No-operation. */
#define	TCPO_MSS	0x02	/* Maximum segment size. */


#endif	_TCP_

/*
 *	@(#)tcp.h	11.9 (NRC)
 */
