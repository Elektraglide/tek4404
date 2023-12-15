/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.pr.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:59  */

#ifndef	_PR_
#define	_PR_

/* Definitions and Macros for dealing with the `pr_t' structure */

#ifndef	_STD_
#include <net/std.h>
#endif

/* flags for pr_t.pr_flags */
#define	F_PR_NEEDS_CONNECT	0x0001

/* protocol switch */
typedef struct pr_t {
	u16	pr_af;		/* address format */
	u16	pr_protocol;
	u16	pr_type;
	u16	pr_flags;
	u16	pr_cpsize;
	u16	pr_psize;
	pfi_t	pr_init;
	pfi_t	pr_accept;
	pfi_t	pr_attach;
	pfi_t	pr_bind;
	pfi_t	pr_connect;
	pfi_t	pr_ioctl;
	pfi_t	pr_getopt;
	pfi_t	pr_listen;
	pfi_t	pr_inaddr;
	pfi_t	pr_outaddr;
	pfi_t	pr_pair;
	pfi_t	pr_recv;
	pfi_t	pr_send;
	pfi_t	pr_setopt;
	pfi_t	pr_shutdown;
	st	pr_up;
	pfi_t	pr_close;
	st	pr_err;
	pfi_t	pr_dink;
	pfi_t	pr_reject;
	pfi_t	pr_zzzpfi;
} pr_t;

#endif	_PR_

/*
 *	@(#)pr.h	10.1 (NRC)
 */
