/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.diddle.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:02  */

#ifndef _DIDDLE_
#define	_DIDDLE_

#ifndef	_STD_
#include <net/std.h>
#endif

import	int	ldiddle();
import	void	frdiddle(), fwdiddle();

/* type information */
#define	F_D_BOOLEAN		0x100
#define	F_D_I16			0x200
#define	F_D_I32			0x400
#define	F_D_U16			0x800
#define	F_D_U32			0x1000
#define	F_D_STRING		0x2000
#define	F_D_OTHER		0x4000

/* diddle function return values */
#define	DID_DO_DEFAULT		-1	/* positive values are reserved to */
#define	DID_DONE		0	/* err values */

#endif	_DIDDLE_

/*
 *	@(#)diddle.h	11.1 (NRC)
 */
