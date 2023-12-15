/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.ftbl.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:15  */

#ifndef	_FTBL_
#define	_FTBL_

#ifndef	_STD_
#include <net/std.h>
#endif

typedef struct ftbl {
	u32	ft_first;	/* first command value accepted */
	u32	ft_last;	/* last  command value accepted */
	pfi_t	ft_spanner;	/* called if non nil and ft_tbl misses */
	pfi_t	* ft_tbl;	/* function called is ft_tbl[cmnd-ft_first] */
} ftbl;

#endif	_FTBL_

/*
 *	@(#)ftbl.h	10.1 (NRC)
 */
