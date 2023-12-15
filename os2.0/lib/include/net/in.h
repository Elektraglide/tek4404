/*  Module: h       File: /nrc/rel/h/s.in.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:35  */

/*	@(#)in.h 1.2	2/6/84 16:21:21
 */

/*	in.h	4.13	82/06/13	*/

#ifndef	_IN_
#define	_IN_

#include <net/std.h>
#include <net/flip.h>

#ifdef B42_COMPATIBLE
#define IPPR_	IPPROTO_
#define IPO_	IPPORT_	
#define IMPLNK_ IMPLINK_
#define INCLS	IN_CLASS
#define	sockaddr_in in_sockaddr	/* for BOGUS implementations */
#endif

/* Constants and structures defined by the internet system,
   Per RFC 790, September 1981. */

/* Protocols */

#define	IPPR_ICMP		1		/* control message protocol */
#define	IPPR_GGP		2		/* gateway^2 (deprecated) */
#define	IPPR_TCP		6		/* tcp */
#define	IPPR_PUP		12		/* pup */
#define	IPPR_UDP		17		/* user datagram protocol */

#define	IPPR_RAW		255		/* raw IP packet */
#define	IPPR_MAX		256

/* Port/socket numbers: network standard functions */

#define	IPO_ECHO		7
#define	IPO_DISCARD		9
#define	IPO_SYSTAT		11
#define	IPO_DAYTIME		13
#define	IPO_NETSTAT		15
#define	IPO_FTP		21
#define	IPO_TELNET		23
#define	IPO_SMTP		25
#define	IPO_TIMESERVER	37
#define	IPO_NAMESERVER	42
#define	IPO_WHOIS		43
#define	IPO_MTP		57

/* Port/socket numbers: host specific functions */

#define	IPO_TFTP		69
#define	IPO_RJE		77
#define	IPO_FINGER		79
#define	IPO_TTYLINK		87
#define	IPO_SUPDUP		95

/* UNIX TCP sockets */

#define	IPO_EXECSERVER	512
#define	IPO_LOGINSERVER	513
#define	IPO_CMDSERVER	514

/* UNIX UDP sockets */

#define	IPO_BIFFUDP		512
#define	IPO_WHOSERVER	513

/* Ports < IPO_RESERVED are reserved for
   privileged processes (e.g. root). */

#define	IPO_RESERVED		1024

/* Link numbers */

#define	IMPLNK_IP		155
#define	IMPLNK_LOWEXPER	156
#define	IMPLNK_HIGHEXPER	158

/* Internet address (old style... should be updated) */

struct in_addr {
	union {
		struct { char s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { u16 s_w1,s_w2; } S_un_w;
		long S_addr;
	} S_un;
};
#define	s_addr	S_un.S_addr	/* can be used for most tcp & ip code */
#ifndef WATCHOUT
#define	s_host	S_un.S_un_b.s_b2	/* host on imp */
#define	s_net	S_un.S_un_b.s_b1	/* network */
#define	s_imp	S_un.S_un_w.s_w2	/* imp */
#define	s_impno	S_un.S_un_b.s_b4	/* imp # */
#define	s_lh	S_un.S_un_b.s_b3	/* logical host */
#endif

/* Macros for dealing with Class A/B/C network
   numbers.  High 3 bits of uppermost byte indicates
   how to interpret the remainder of the 32-bit
   Internet address.  The macros may be used in time
   time critical sections of code, while subroutine
   versions also exist use in other places. */

#ifdef notdef
#if vax
#define	INCLSA	0x00000080
#define	INCLSA_NET	0x000000ff	/* 8 bits of net # */
#define	INCLSA_LNA	0xffffff00
#define	INCLSB	0x00000040
#define	INCLSB_NET	0x0000ffff	/* 16 bits of net # */
#define	INCLSB_LNA	0xffff0000
#define	INCLSC_NET	0x00ffffff	/* 24 bits of net # */
#define	INCLSC_LNA	0xff000000
#endif
#ifdef pdp11
#define INCLSA       0x00800000L
#define INCLSA_NET   0x00ff0000L     /* 8 bits of net # */
#define INCLSA_LNA   0xff00ffffL
#define INCLSB       0x00400000L
#define INCLSB_NET   0xffff0000L     /* 16 bits of net # */
#define INCLSB_LNA   0x0000ffffL
#define INCLSC_NET   0xffff00ffL     /* 24 bits of net # */
#define INCLSC_LNA   0x0000ff00L
#endif
#endif
#ifndef WATCHOUT 	/* 68k (?) */
#define INCLSA       0x80000000L
#define INCLSA_NET   0xff000000L     /* 8 bits of net # */
#define INCLSA_LNA   0x00ffffffL
#define INCLSB       0x40000000L
#define INCLSB_NET   0xffff0000L     /* 16 bits of net # */
#define INCLSB_LNA   0x0000ffffL
#define INCLSC_NET   0xffffff00L     /* 24 bits of net # */
#define INCLSC_LNA   0x000000ffL
#endif

#define	IN_NETOF(in) \
	(((in).s_addr&INCLSA) == 0 ? (in).s_addr&INCLSA_NET : \
		((in).s_addr&INCLSB) == 0 ? (in).s_addr&INCLSB_NET : \
			(in).s_addr&INCLSC_NET)
#define	IN_LNAOF(in) \
	(((in).s_addr&INCLSA) == 0 ? (in).s_addr&INCLSA_LNA : \
		((in).s_addr&INCLSB) == 0 ? (in).s_addr&INCLSB_LNA : \
			(in).s_addr&INCLSC_LNA)

#define	INADDR_ANY	0x00000000L

/* Socket address, internet style. */

struct in_sockaddr {
	i16	sin_family;
	u16	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};


/* define the host/net order transformations specified in byteorder(3N)
	these are simply macros of the standard FUSION flip'ers */

#define	htonl		_flip32
#define htons		_flip16
#define ntohl		_flip32
#define ntohs		_flip16

#endif	_IN_

/*
 *	@(#)in.h	11.1 (NRC)
 */
