/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.tc.h  */
/*  Version 10.1 as of 85/10/27 at 12:46:25  */

#ifndef	_TC_
#define	_TC_

#define	TC_OK		0

#define	E_TC_INVALID	-1
#define	E_TC_NOSPACE	-2
#define	E_TC_NOFILE	-3
#define	E_TC_EMPTY	-4
#define	E_TC_NOENT	-5

import	char	* tc_string(), * tc_number(), * tc_boolean();
import	int	tc_get();
#endif	_TC_

/*
 *	@(#)tc.h	10.1 (NRC)
 */
