/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.rem.h  */
/*  Version 10.1 as of 85/10/27 at 12:46:09  */

#ifndef	_REM_
#define	_REM_

/* Data stream types in sequence packets for login & descendant processing */

#define REM_DATA	0	/* regular character data in packet */
#define	REM_ECHO	1       /* kindly fling it back to my shell */
#define	REM_IOCTL	2	/* remote ioctl, data follows */
#define	REM_RLOGIN	3	/* new rlogin signaling old one */
#define	REM_CLOSE	254	/* socket being closed by protocol */
#define	REM_END		255	/* lost second part of socket close */

#endif	_REM_

/*
 *	@(#)rem.h	10.1 (NRC)
 */
