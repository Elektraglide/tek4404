/*  Copyright (C) 4/29/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.xns.h  */
/*  Version 11.3 as of 86/04/29 at 06:03:53  */

#ifndef	_XNS_
#define	_XNS_

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif

/* Option name for use with getopt() and setopt().  Option value
 * determines whether to turn checksumming on or off.
 */

#define	XNS_CHECKSUM	0x1	/* software checksumming enabled */

#define	XNS_MIN_FREE	3001	/* first free socket number */
#define XNS_NO_CHECKSUM	(u16)0xFFFF

/* protocol identifiers */
#define	XNS_SEQPACKET	1
#define	XNS_ROUTER	2
#define	XNS_PACKET_EX	3
#define	XNS_ECHO	4
#define	XNS_RELIABLE	5
#define	XNS_DGRAM	6

#define	XEP_REQUEST	1	/* this is an echo request */
#define	XEP_REPLY	2	/* this in an echo reply */

#define	XIR_PACKET	1	/* (Xerox) */
#define	XECHO_PACKET	2	/* (Xerox) */
#define	XERR_PACKET	3	/* error packet type */
#define	XPE_PACKET	4	/* (Xerox) */
#define	XSP_PACKET	5	/* (Xerox) */
#define XRD_PACKET	6	/* packet type for reliable message protocol */

#define	XSPT_END	254
#define	XSPT_REPLY_END	255

#define	ECHO_TYPE	2
#define	ECHO_REQUEST	1
#define	ECHO_REPLY	2

#define XE_DUNK		0	/* unspecified err. at destination */
#define XE_DBOGUS	1	/* bogus packet at destination */
#define XE_NOSOCK	2	/* no socket at destination */
#define XE_RESOURCE	3	/* no resource at destination */
#define XE_RUNK		01000	/* unspecified err. enroute */
#define XE_RBOGUS	01001	/* bogus packet enroute */
#define XE_NOHOST	01002	/* no host enroute */
#define XE_HOPPY	01003	/* too many hops enroute */
#define XE_NARROW	01004	/* size overrun enroute */

#define	F_XSP_SYSTEM_PACK	0x80
#define	F_XSP_SEND_ACK		0x40
#define	F_XSP_ATTENTION		0x20
#define	F_XSP_END_OF_MSG	0x10
#define	F_XSP_UNRELIABLE	0x100	/* for internal use only, not sent */

#define	F_XST_DONE		0x1	/* the other side is done */
#define F_XST_CONN_INPROG	0x2
#define F_XST_WANTS_ACK		0x4
#define F_XST_PEER_SHOULD_RETRY	0x8
#define F_XST_WE_SHOULD_RETRY	0x10
#define	F_XST_PROBE_PEER	0x20
#define F_XST_MAINT	(F_XST_PEER_SHOULD_RETRY|F_XST_WE_SHOULD_RETRY|F_XST_WANTS_ACK|F_XST_PROBE_PEER)

#define HOP_DIFF	(u32)0x1000 /* used in modify checksum - hop count */

#define XRD_ESN			1

#define	is_system_packet(xsp)	((xsp)->xs_c_control & F_XSP_SYSTEM_PACK)
#define	is_attention(xsp)	((xsp)->xs_c_control & F_XSP_ATTENTION)
#define	is_eom(xsp)		((xsp)->xs_c_control & F_XSP_END_OF_MSG)
#define	ack_requested(xsp)	((xsp)->xs_c_control & F_XSP_SEND_ACK)
#define	xns_this_host(addr)	cheql(addr.a6, host_addr.a6, sizeof(a48))

/* Xerox header (datagrams) */
typedef struct xh {
	a16     xh_checksum;
	a16     xh_length;
	char    xh_transport_control;
	char    xh_packet_type;
	xna     xh_dest;
	xna     xh_src;
} xh;

/* Xerox error protocol header */
typedef struct xerr {
	a16	xerr_number;
	a16	xerr_parameter;
} xerr;

/* Xerox sequence packet header */
typedef struct xs {
	char	xs_c_control;
	char	xs_type;
	a16	xs_s_c_id;
	a16	xs_d_c_id;
	a16	xs_seq_num;
	a16	xs_ack_num;
	a16	xs_all_num;
} xs;

/* Xerox based reliable message header */
typedef	struct	xr {
	a16	xr_packet_id;
	a16	xr_type;
} xr;

/* combined sequence packet and datagram header */
typedef struct xc {
	xh	xc_xh;
	xs	xc_xs;
} xc;

/* Xerox packet exchange header */
typedef struct xpe {
	a32	xpe_id;
	a16	xpe_client_type;
} xpe;

/* Xerox echo protocol header */
typedef struct xe {
	a16	xe_operation;
} xe;

/* Xerox routing information protocol header */
typedef struct xrip {
	a16	xrip_operation;
	a32	xrip_on1;
	a16	xrip_id1;
	a32	xrip_on2;
	a16	xrip_id2;
} xrip;

#endif	_XNS_

/*
 *	@(#)xns.h	11.3 (NRC)
 */
