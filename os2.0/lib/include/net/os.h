/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.os.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:57  */

/* os dependent structure (see so_t in so.h) */

#ifndef _OS_
#define	_OS_

#ifdef INP

#include <proc.h>

typedef struct os {
	struct	proc	* os_procp;
} os;

#endif  INP

#ifdef UNIX
#ifdef MASSCOMP
#include <param.h>
#include <pte.h>
#endif

#ifdef CALLAN5
#include <param.h>
#endif

#ifdef TEK4404
#include <task.h>

typedef struct os {
	struct	userbl	* os_up;
	struct	task	* os_taskp;
	int		os_tstid;
} os;

#else

#include <proc.h>

typedef struct os {
	struct	proc	* os_procp;
	int		os_pid;
	int		os_pad1;
} os;
#endif TEK4404

#endif	UNIX

#ifdef	vms
#include <acbdef.h>

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>	/* for <tcb.h> */
#endif
#ifndef	_TCB_
#include <net/tcb.h>
#endif

typedef struct os {
	u32	os_pid;
	int	os_sig;
	int	os_pad1;
	tcb	os_tcb;
	ACB	os_acb;
} os;
#endif	vms

#endif	_OS_

/*
 *	@(#)os.h	10.1 (NRC)
 */
