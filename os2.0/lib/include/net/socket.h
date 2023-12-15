/*  Copyright (C) 4/29/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.socket.h  */
/*  Version 11.2 as of 86/04/29 at 05:37:30  */

#ifndef	_SOCKET_
#define	_SOCKET_

#ifndef	_STD_
#include <net/std.h>
#endif	_STD_

/* address families */

#define	AF_UNSPEC	0	/* unspecified */
#define	AF_INTRA	1	/* O/S-dependent path names */
#define	AF_INET		2	/* ARPA Internet address */
#define	AF_NS		3	/* Xerox Network Systems protocols */
#define	AF_NBS		4	/* NBS protocols */
#define	AF_CCITT	5	/* CCITT protocols, X.25 etc */
#define	AF_SNA		6	/* IBM SNA */
#define	AF_ARP		7	/* Address Resolution Protocol */
#define AF_1		8	/* customer expansion */
#define AF_2		9	/* " */

/* link layer address families */

#define	AF_XLL		10	/* Raw External Link Layer */
#define	AF_ETHER	11	/* Raw Ethernet */
#define	AF_PRONET	12	/* Raw Proteon */
#define	AF_IMPLINK	13	/* IMP "host at IMP" addresses */
#define AF_L1		14	/* customer expansion */
#define AF_L2		15	/* " */
#define AF_L3		16	/* " */
#define AF_L4		17	/* " */
#define MAX_AF		AF_L4	/* last address family */
#define MIN_LLAF	AF_XLL	/* first link layer address family */
#define MAX_LLAF	MAX_AF	/* last link layer address family */
#define	AF_UNIX		AF_INTRA	/* UNIX path names */

/* although the following address families alluded to by Berkeley have
 * a dubious future under Fusion, nevertheless, their names are reserved:
 *	AF_PUP		Xerox PUP-I internet address
 *	AF_CHAOS	MIT CHAOS protocols
 *	AF_ECMA		European computer manufacturers
 *	AF_DATAKIT	datakit protocols
 */

/* address family and link layer address family counts / macros */

#define	NAF	(MAX_AF+1)	/* number of AF's */
#define NLLAF	(MAX_LLAF-MIN_LLAF+1)	/* number of link layer AF's */
#define af_ix(a)	(a)	/* AF to index */
#define aflim(a)	bound((a),(u16)0,(u16)MAX_AF)	/* bound AF */
#define afok(a)		inbound((a),(u16)0,(u16)MAX_AF)
#define llaf_ix(a)	((a)-(u16)MIN_LLAF)	/* link layer AF to index */
#define llaflim(a)	bound((a),(u16)MIN_LLAF,(u16)MAX_LLAF)	/* bound link layer AF */
#define llafok(a)	inbound((a),(u16)MIN_LLAF,(u16)MAX_LLAF)

/* socket types */

#define	SOCK_STREAM	1	/* virtual circuit */
#define	SOCK_DGRAM	2	/* datagram */
#define	SOCK_RAW	3	/* raw socket */
#define	SOCK_RDM	4	/* reliable-delivered message */
#define	SOCK_SEQPACKET	5	/* sequenced packet */
#define	SOCK_PACK_EX	6	/* packet exchange non Berkeley */

/* level number for get/setopt to apply to socket itself */

#define	SOL_SOCKET	0xFFFF

/* socket options used with get/setopt calls */

#define	SO_DEBUG	0x1	/* turn on debugging */
#define	SO_ACCEPTCONN	0x2	/* socket has had listen() */
#define	SO_REUSEADDR	0x4	/* allow local address reuse */
#define	SO_KEEPALIVE	0x8	/* keep connections alive */
#define	SO_DONTROUTE	0x10	/* just use interface addresses */
#define	SO_USELOOPBACK	0x40	/* bypass hardware where possible */
#define	SO_LINGER	0x80	/* linger on close if data is present */
#define	SO_DONTLINGER	(~SO_LINGER)

#define	MAXOPTVALEN	32	/* max. size of socket option value */

/* 4.2bsd send/recv standard flags */
#define	MSG_OOB		0x1	/* process out of band information */
#define	MSG_PEEK	0x2	/* peek at incoming message */
#define	MSG_DONTROUTE	0x4	/* send without using routing tables */
/* 4.2bsd standard flag mask */
#define	MSG_FLAGS	(MSG_OOB|MSG_PEEK|MSG_DONTROUTE)

/* NRC value added */
#define	MSG_BLOCKING	0x8000	/* override any nonblocking state */
#define	MSG_NONBLOCKING	0x4000	/* override any blocking state */
#define	MSG_FDBROADCAST	0x2000	/* want full duplex broadcasts */
#define MSG_TRUNCATE	0x1000	/* truncate received packet */
#define	MSG_MASK	(MSG_FLAGS|MSG_BLOCKING|MSG_NONBLOCKING|MSG_FDBROADCAST|MSG_TRUNCATE)

/* condition types for the async call */
#define	INCOMING_DATA	0
#define	CONNECTION_COMP	1
#define	ACCEPT_COMP	2
#define	WRITE_READY	3

/* Address structures for all supported families */

typedef struct xna {	/* Xerox Internet Address */
	a32	xn_network;
	a48	xn_host;
	a16	xn_socket;
} xna;

typedef	struct ipa {	/* DARPA Internet Protocol Address */
	a16	ip_port;	/* an IP port number */
	union {
		struct {
			char	ip_u_b1;
			char	ip_u_b2;
			char	ip_u_b3;
			char	ip_u_b4;
		} ip_u_b;
		struct {
			a16	ip_u_w1;
			a16	ip_u_w2;
		} ip_u_w;
		a32 ip_u_a;
	} ip_u;	/* Internet address */
#define	ip_nethost ip_u.ip_u_a		/* Composite network/host address */
#define	ip_host	ip_u.ip_u_b.ip_u_b2	/* Host on Imp */
#define	ip_net	ip_u.ip_u_b.ip_u_b1	/* Network */
#define	ip_imp	ip_u.ip_u_w.ip_u_w2	/* Imp */
#define	ip_impno ip_u.ip_u_b.ip_u_b4	/* Imp number */
#define	ip_lh	ip_u.ip_u_b.ip_u_b3	/* Logical host */
} ipa;

/* Typed socket addresses for all supported address families */

typedef	struct sockaddr {	/* Generic socket address */
	short	sa_type;
	char	sa_data[22];
} saddr;

typedef struct xsockaddr {	/* XNS socket address */
	short	sa_type;	/* AF_NS */
	xna	sa_xna;
} xsaddr;

/* Congruent to the nasty WNJ-style stuff in "un.h" */
typedef struct lsockaddr {	/* Intramachine (local) socket address */
	short	sa_type;	/* AF_INTRA */
	char	sa_name[2];	/* null-terminated string (can be longer) */
} lsaddr;

/* Congruent to the nasty stuff in "in.h" */
typedef struct isockaddr {	/* DARPA Internet socket address */
	short	sa_type;	/* AF_INET */
	ipa	sa_ipa;
} isaddr;

typedef struct eaddr {		/* Raw Ethernet socket address */
	short	sa_type;	/* AF_ETHER */
	a48	sa_ena;
} esaddr;

typedef struct proaddr {	/* Raw Pronet socket address */
	short	sa_type;	/* AF_PRONET */
	char	sa_pna;
} psaddr;

typedef struct kiaaddr {  /* AF_INTRA address w/i the kernel */
	u16	ksa_type;		/* AF_INTRA */
	u16	ksa_pad1;
	struct	str_t	* ksa_str;	/* ptr. to funny name structure */
} kiaaddr;

typedef union sa_u {		/* union of all types of socket addresses */
	saddr	au_saddr;	/* generic */
	xsaddr	au_xsaddr;	/* AF_NS: Xerox network */
	lsaddr	au_lsaddr;	/* AF_INTRA: local to this machine */
	isaddr	au_isaddr;	/* AF_INET: DARPA Internet */
	esaddr	au_esaddr;	/* AF_ETHER: Ethernet address */
	psaddr	au_psaddr;	/* AF_PRONET: Pronet address */
} sa_u;

/* Security structures for all address families */

typedef	struct secure {		/* Generic security structure */
	char	sec_data[16];
} secure;

/* Route key structure */
typedef struct rte {
	saddr	rte_net;	/* network number (partial address) */
	saddr	rte_gwy;	/* gateway host (full host address) */
	secure	rte_secure;	/* security structure */
} rte;

/* Socket option value structure */
typedef struct socketopt {
	int	so_optlen;	/* length of optdata area */
	char *	so_optdata;	/* pointer to data */
} socketopt;

/* Internal structure for socket addresses.  Congruent to Berkeley 'saddr'
 * structure from 'a_type' down.
 */
typedef	struct {
	u16	a_len;
	u16	a_pad1;
	union {
		sa_u	a_sa_u;
		kiaaddr	a_kiaaddr;
	} a_u;
} so_addr;

/* short-hand definitions to get into so_addr */
#define a_saddr	a_u.a_sa_u.au_saddr
#define a_type	a_saddr.sa_type
#define	a_data	a_saddr.sa_data
#define a_xsaddr a_u.a_sa_u.au_xsaddr
#define a_lsaddr a_u.a_sa_u.au_lsaddr
#define a_isaddr a_u.a_sa_u.au_isaddr
#define a_esaddr a_u.a_sa_u.au_esaddr
#define a_psaddr a_u.a_sa_u.au_psaddr
#define a_xna	a_xsaddr.sa_xna
#define a_ipa	a_isaddr.sa_ipa
#define a_ena	a_esaddr.sa_ena
#define a_pna	a_psaddr.sa_pna
#define a_intra	a_u.a_kiaaddr.ksa_str

#endif	_SOCKET_

/*
 *	@(#)socket.h	11.2 (NRC)
 */
