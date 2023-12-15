/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h/osdep       File: /nrc/rel/h/osdep/s.tek.h  */
/*  Version 10.1 as of 85/10/27 at 12:56:03  */

#ifndef	_TEK_
#define	_TEK_

/*TEK4404: tek added */
#define INFO_SOCKET    "/dev/fusion/so000"
/* */

#include <net/ftype.h>

#define	NET_ERR				80
#define	INET_ERR			(-1)
#define	GOOD_EXIT			0
#define	BAD_EXIT			1

#define	SOIOCBASE			((unsigned)32757)
#define	SOIOCATMARK			32754 /* SOIOCBASE - 3 */
#define	ENIOCBASE			100      /* Ethernet specific ioctls */

/* add some handy errors missing on tek */
#define	EPERM				(NET_ERR-1)
#define	ENXIO				(NET_ERR-2)
#define ENODEV				(NET_ERR-3)
#define ENFILE				(NET_ERR-4)
#define EFBIG				(NET_ERR-5)
#define EROFS				(NET_ERR-6)

#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)

#ifndef time_t
#include <sys/types.h>
#endif

#endif	_TEK_

/*
 *	@(#)tek.h	10.1 (NRC)
 */
