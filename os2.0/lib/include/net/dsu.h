/*  Copyright (C) 10/29/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: ./s.dsu.h  */
/*  Version 10.4 as of 85/10/29 at 08:47:23  */

#ifndef	_DSU_
#define	_DSU_

#ifndef	_STD_
#include <net/std.h>
#endif

/* Network device specific structures and their unions */

#ifdef	I586_DEVICE
/* Intel 82586 controller specifics (for intelligent boards) */
#ifndef	_I586REG_
#include <i586reg.h>
#endif

typedef struct i586 {
	fd_rbd	* i586_frdp;
	fd_rbd	* i586_lastp;
	fd_rbd	* i586_fillp;
	fd_rbd	* i586_curp;
	scb	i586_scb;
	itcb	i586_tcb;
} i586;
#else
#define	i586	i32
#endif

#ifdef	XEEA_DEVICE
/* Specifics for Excelan EXOS Ethernet controller series */
typedef struct	xeea {
	u16	xeea_mlbx;	/* Excelan type mailbox */
	i16	xeea_post_cnt;	/* number of receive buffers available */
} xeea;
#else
#define	xeea	i32
#endif

#ifdef MEIA_DEVICE
/* Multibus Ethernet Interlan NI3010A controller specifics */
typedef struct	meia {
	char	meia_ie_reg;	/* copy of interrupt enable register setting */
	char	meia_c_reg;	/* copy of last issued command	*/
	u16	meia_pad1;
	struct m	* meia_mp;	/* where packet was dma_ed into the heap */
	struct en	* meia_dma_enp; /* for devices with bus/board byte flips */
} meia;
#else
#define	meia	i32
#endif

#ifdef MEIB_DEVICE
/* Multibus Ethernet Interlan NI3210 controller specifics */
typedef struct	meib {
	struct dma_t	* mu_dma_queue;
	struct dma_t	* mu_dma_free_queue;
	struct cmd_t	* mu_cmd_queue;
	struct cmd_t	* mu_cmd_free_queue;
	int		mu_rcvdmabuf; /* next buffer to dma from */
	int		mu_rcviobuf; /* buffer we expect to receive next */
	u16		mu_status; /* driver state */
	u16		mu_pad1;
} meib;
#else
#define	meib	i32
#endif

#ifdef META_DEVICE
/* Multibus Ethernet 3Com 3C400 controller specifics */
typedef	struct	meta {
	i16	meta_retries;
	u16	meta_csr;
} meta;
#else
#define	meta	i32
#endif

#ifdef QEVA_DEVICE
/* Q-bus Ethernet VMS controller specifics */
/* NOTE! - Looks exactly like UEVA_DEVICE */
/* This device definition must preceed the UEVA_DEVICE definition */
#ifndef _VMS_DEVICE
#define _VMS_DEVICE
#endif
#endif

#ifdef PEIA_DEVICE
/* PCbus Ethernet Interlan NI5010 controller specifics */
typedef struct peia {
	u16	peia_offset;	/* starting offset of current send */
	u16	peia_pad1;
} peia;
#else
#define	peia	i32
#endif

#ifdef PETA_DEVICE
/* PCbus Ethernet 3Com IE controller specifics */
typedef struct peta {
	u16	peta_offset;	/* starting offset of current send */
	char	peta_rmode;	/* recv mode */
	char	peta_pad1;
} peta;
#else
#define	peta	i32
#endif

#ifdef PPPA_DEVICE
typedef	struct	pppa {
	boolean		pppa_first_dma;
	struct m	* pppa_rcv_mp;
} pppa;
#else
#define	pppa	i32
#endif

#ifdef UEDA_DEVICE
/* Unibus Ethernet DEC controller specifics */
typedef struct ueda {
	struct	ring_d	* ueda_rrdp;		/* base of recv ring desc. */
	struct	ring_d	* ueda_c_rrdp;		/* current recv ring desc. */
	struct	ring_d	* ueda_e_rrdp;		/* end of recv ring desc. */
	struct	ring_d	* ueda_trdp;		/* base of trans ring desc. */
	struct	ring_d	* ueda_c_trdp;		/* current trans ring desc. */
	struct	ring_d	* ueda_e_trdp;		/* end of trans ring desc. */
	struct	pcb	* ueda_pcbp;		/* ptr to port command block */
	char		* ueda_ppad1;
	u16		ueda_flags;
	u16		ueda_pad1;
} ueda;
#else
#define	ueda	i32
#endif

#ifdef UEIA_DEVICE
/* Unibus Ethernet Interlan NI1010A controller specifics */
typedef struct	ueia {
	struct m	* ueia_mp;	/* where packet was dma_ed into the heap */
	char	* ueia_ppad1;
	u16	ueia_flags;
	u16	ueia_sr;		/* value from reading status register */
	char	ueia_cr;		/* copy of last issued command */
	char	ueia_pad1[3];
} ueia;
#else
#define	ueia	i32
#endif

#ifdef UETA_DEVICE
/* Unibus Ethernet 3Com 3C300 controller specifics */
typedef	struct	ueta {
	u16	ueta_rcsr;	/* for writing receive csr */
	u16	ueta_tcsr;	/* for writing transmit csr */
	char	* ueta_buf;	/* board buffer address */
	char	* ueta_ppad1;
	i16	ueta_retries;	/* transmission retries */
	i16	ueta_pad1;
} ueta;
#else
#define	ueta	i32
#endif

#ifdef UEVA_DEVICE
/* Unibus Ethernet VMS controller specifics */
/* NOTE! - must follow definition for QEVA_DEVICE! */
#ifndef	_VMS_DEVICE
#define _VMS_DEVICE
#endif
#endif	UEVA_DEVICE

#ifdef	_VMS_DEVICE
/*  *********          ANY CHANGES IN THIS STRUCTURE             *********** */
/*  ********* MUST ALSO BE MADE TO DSU DEFINITION IN UEVAMAR.MAR *********** */

typedef struct ueva {
	a32		ueva_ucb0;	/* address of prototype UCB to clone */
	a32		ueva_ffi;	/* head of fast interface block list */
	u16		ueva_flags;	/* flags */
#define	UEVA_RUN	1
	u16		ueva_mlen;	/* length of the m struct */
	struct	m	* ueva_rmp;	/* available receive message */
	struct	m	* ueva_tmp;	/* current transmit message */
	char		* ueva_tcxb;	/* pointer to transmit CXB */
	char		ueva_devnam[4];	/* device name build area */
	a48		* ueva_phap;	/* physical address pointer */
} ueva;
#else
#define ueva	i32
#endif	_VMS_DEVICE

#ifdef PLL_DEVICE
typedef struct pll_d {
	struct	so_t	* pd_sop;	/* backptr. to process socket */
	char	* pd_ppad1;
} pll_d;
#else
#define	pll_d	i32
#endif

#ifdef MPPA_DEVICE
#define	PROTEON
#endif
#ifdef UPPA_DEVICE
#define	PROTEON
#endif

#ifdef PROTEON
typedef struct proteon {
	struct m	* pp_mp;
	struct tcb	* pp_tcbp;
} proteon;
#else
#define	proteon	i32
#endif

/* network device specific union */
typedef	union	{
	i586		i586_u;
	meia		meia_u;
	meib		meib_u;
	meta		meta_u;
	peia		peia_u;
	peta		peta_u;
	pppa		pppa_u;
	ueda		ueda_u;
	ueia		ueia_u;
	ueta		ueta_u;
	xeea		xeea_u;
	pll_d		pll_u;
	proteon		pp_u;
} dsu;

#endif	_DSU_

/*
 *	@(#)dsu.h	10.4 (NRC)
 */
