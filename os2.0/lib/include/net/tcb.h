/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.tcb.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:46  */

#ifndef	_TCB_
#define	_TCB_

/* Definitions and Macros for dealing with the `tcb' structure */

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>
#endif

/* flags for tc.tc_flags */
#define	F_T_ONESHOT	0x0001		/* pfi to be called once only */
#define	F_T_ALLOCATED	0x0002		/* this tcb was heap allocated */
#define F_T_CRITICAL	0x0004

#define	t_ms_update(p,ms)	((p)->tc_original = ms)

/* Timer control block structure */
typedef struct tcb {
	q	tc_q;
	u16	tc_flags;
	u16	tc_pad1;
	u32	tc_original;
	i32	tc_remaining;
	char	* tc_arg;
	pfi_t	tc_pfi;
} tcb;

#endif	_TCB_

/*
 *	@(#)tcb.h	11.1 (NRC)
 */
