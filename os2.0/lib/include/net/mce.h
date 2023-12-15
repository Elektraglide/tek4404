/*  Copyright (C) 5/9/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: ./s.mce.h  */
/*  Version 1.2 as of 85/05/09 at 19:18:40  */

#ifndef	_MCE_
#define	_MCE_

/* Defines and structures common for node and host */
#define	NI_HMMAX	1
#define	NI_MMAX		1

/* mce dependent initialization mailbox */
typedef	struct initmbox {
	u16	in_h_mcnt;	/* host mailbox cnt, 0 means np chooses */
	u16	in_n_mcnt;	/* np mailbox cnt, 0 means np chooses */
	u32	in_h_maddr;	/* host mailbox addr */
	u32	in_n_maddr;	/* mailbox addr */
	u32	in_pm_base;	/* phys memory base for NP */
} initmbox;

typedef struct thes {
	u32	th_go;			/* where the NP_GO command goes */
	u32	th_pstart;		/* address where program start goes */
	initmbox th_mceinit;		/* initial negotiation mailbox */
	init	th_init;		/* general initialization structure */
	mbox	th_h_mbarr[NI_HMMAX];	/* host mailbox array */
	mbox	th_n_mbarr[NI_MMAX];	/* mailbox array */
} thes;

#endif	_MCE_

/*
 *	@(#)mce.h	1.2 (NRC)
 */
