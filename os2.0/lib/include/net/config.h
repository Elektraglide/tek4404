/*  Copyright (C) 5/3/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h/config       File: /nrc/rel/h/config/s.tek4404.h  */
/*  Version 11.5 as of 86/05/03 at 09:35:51  */

#ifndef	_CONFIG_
#define	_CONFIG_

#define	UNIX		/* The OS family - UNIX, VERSADOS, VMS, MSDOS */
#define	TEK4404	1	/* for machine specific code */

#include <net/tek.h>

/* MS_PER_TICK == how many milliseconds between clock service calls */
#define	MS_PER_TICK	10	/* Tek ticks are hundreths of seconds */

/* Compiler description: the choices here are a function of word size
 * and brain damage.
 */
#define	INT_32		/* integers are 32 bits */

#ifndef NULL
#define NULL	0
#endif

#define	mem_addr	char *	/* what a memory address looks like */
#define	nil		0	/* to get the compiler off our back */

#ifndef KERNEL
#define NOFILE		32
#endif

#define TCP_ONLY

/* Priorities */
#define	NET_PRIORITY	-5	/* for networks sleeps, cf. kernal/priorities */
#define	NTTY_PRIORITY	-10	/* for getty sleeps on nttys */

#define	TETA_DEVICE			/* TEK */

#define	ETHER_PROTOCOL		/* Raw and Link Layer Ethernet access */

#ifdef	NEED_CONFIG_INFO
#include <time.h>

#define	XDG_SQ_MAX	4	/* default send queue max */
#define	XDG_RQ_MAX	4	/* default recv queue max */

#define	XSP_SQ_MAX	4	/* default send queue max */
#define	XSP_RQ_MAX	4	/* default recv queue max */
#define	XSP_TRY_MAX	6	/* number of retries before giving up */

#ifdef	X_F_VTP
#define	MAX_NTTYS	5		/* default number of network ttys */
#define	NTTY_BUFSIZE	20		/* default ntty buffer size */
#endif	X_F_VTP

	/* Intramachine protocol family (AF_INTRA) */
#define	IA_SQ_MAX	10	/* default send queue max */
#define	IA_RQ_MAX	10	/* default recv queue max */

	/* DARPA Internet protocol family (AF_INET) */
#define UDP_SQ_MAX	2	/* default send queue max */
#define UDP_RQ_MAX	8	/* default recv queue max */

#define TCP_SQ_MAX	2	/* default send queue max */
#define TCP_RQ_MAX	2048	/* bytes */
#define	TCP_TRY_MAX	6

#ifdef	ETHER_PROTOCOL
#define	EN_RQ_MAX	10	/* default recv queue max, send unlimited */
#endif

#define	SOCK_PREFIX	"/dev/fusion/so" /* string prefixing socket names */
#define	TRAILING_DIGITS	3		/* how many digits after prefix */
#define	MAX_SOCKETS	20		/* how many sockets to allocate */
#define	FUSION_DB_NAME	"/etc/fusion.db" /* standard Fusion indirection db */
#define	MAX_BACKLOG	5		/* allowed listening backlog */
#define	NET_TICK	1000		/* milliseconds between net clocks */
#define	STATIC_HEAP			/* heap is compiled in */
#define	HEAP_SIZE	80000		/* see the machine specfic h files */

#endif	NEED_CONFIG_INFO

#endif	_CONFIG_

/*
 *	@(#)tek4404.h	11.5 (NRC)
 */
