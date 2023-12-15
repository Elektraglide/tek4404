/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.kludge.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:49  */

#ifndef	_KLUDGE_
#define	_KLUDGE_

/* Definitely nasty work arounds for name conflicts between FUSION
   and the insane real world. */

#ifdef	B42
#define tcp_trace tcptrace
#endif	B42

#endif	_KLUDGE_

/*
 *	@(#)kludge.h	11.1 (NRC)
 */
