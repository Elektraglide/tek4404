/*  Copyright (C) 4/23/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.netdev.h  */
/*  Version 11.2 as of 86/04/23 at 17:17:48  */

#ifndef	_NETDEV_
#define	_NETDEV_

/* Definitions and Macros for dealing with the `netdev' structure */

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif
#ifndef	_DSU_
#include <net/dsu.h>		/* device specific union */
#endif	_DSU_

/* flags for ndp->nd_flags */
#define	F_N_INITIALIZED	0x0001	/* has been initialized */
#define F_N_ONLINE	0x0002	/* device is on line */
#define	F_N_NONEXISTENT	0x0004	/* no response during probe */
#define	F_N_HDUPLEX	0x0008	/* board can't hear itself */
#define F_N_HGWY	0x0010	/* half-gateway (XLL) */

#define	MAX_DEV_NAME	16

typedef struct st_blk {	/* Ethernet Based */
	u32	sb_rcnt;	/* Receive packet count */
	u32	sb_xcnt;	/* Transmit packet count */
	u32	sb_recnt;	/* Receive error count */
	u32	sb_xecnt;	/* Transmit error count */
	u32	sb_jam;		/* Jam retry count */
} st_blk;

/* net device switch */
typedef struct netdev {
	char	nd_name[MAX_DEV_NAME];	/* device name */
	u16	nd_flags;       /* Flags */
	u16	nd_devid;       /* Minor device id, ie a board # */
	u16	nd_ix;		/* 'ndp_tbl' index (back ptr) */
	u16	nd_xflags;	/* Filler field */
	u32	nd_p0;		/* device specific init values */
	u32	nd_p1;
	u32	nd_p2;
	u32	nd_p3;
	so_addr	nd_lladdr;	/* link layer address of board */
	saddr	nd_addrs[NAF];	/* host address per AF */
	st_blk	nd_stat;	/* statistics */
	pfi_t	nd_init;	/* Initialization entry point */
	pfi_t	nd_updown;	/* turn device on/off */
	st	nd_send;	/* Send message entry point */
	pfi_t	nd_start;	/* Start the actual transfer */
	pfi_t	nd_ioctl;	/* Ioctl entry point */
	pfi_t	nd_zzzpfi;	/* relocation terminating zero field */
	u32	nd_brdcast;	/* # of outstanding unwanted broadcasts */
	struct	so_t * nd_sop;	/* Socket backptr. for RAW stuff */
	struct	netdev *nd_next;/* Link to next similar device */
	mq	nd_mq;		/* transmit message queue */
	struct frent *nd_re;	/* router entry list (see "fr.h") */
	char	* nd_ppad1;
	dsu	nd_dsu;		/* Device board specific union */
} netdev;

#endif	_NETDEV_

/*
 *	@(#)netdev.h	11.2 (NRC)
 */
