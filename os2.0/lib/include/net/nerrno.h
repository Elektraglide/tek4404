/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.nerrno.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:59  */

#ifndef	_NERRNO_
#define _NERRNO_

#ifndef EINVAL
#include <errno.h>
#endif

#ifdef	B42
#define	_OK_
#endif

#ifdef MASSCOMP
#define	_OK_
#endif

/* internal err/status values */
#define	IDITCHPACKET		(INET_ERR - 1)
#define IECHOPACKET		(INET_ERR - 2)
#define IRETRYLATER		(INET_ERR - 3)
#define IOTHER			(INET_ERR - 4)

#ifndef _OK_
#define	EWOULDBLOCK		(NET_ERR + 0)
#define	EINPROGRESS		(NET_ERR + 1)
#define	EALREADY		(NET_ERR + 2)
#define	ENOTSOCK		(NET_ERR + 3)
#define	EDESTADDRREQ		(NET_ERR + 4)
#define	EMSGSIZE		(NET_ERR + 5)
#define	EPROTOTYPE		(NET_ERR + 6)
#define	ENOPROTOOPT		(NET_ERR + 7)
#define	EPROTONOSUPPORT		(NET_ERR + 8)
#define	ESOCKTNOSUPPORT		(NET_ERR + 9)
#define	EOPNOTSUPP		(NET_ERR + 10)
#define	EAFNOSUPPORT		(NET_ERR + 11)
#define	EADDRINUSE		(NET_ERR + 12)
#define	EADDRNOTAVAIL		(NET_ERR + 13)
#define	ENETDOWN		(NET_ERR + 14)
#define	ENETUNREACH		(NET_ERR + 15)
#define	ENETRESET		(NET_ERR + 16)
#define	ECONNABORTED		(NET_ERR + 17)
#define	ECONNRESET		(NET_ERR + 18)
#define	ENOBUFS			(NET_ERR + 19)
#define	EISCONN			(NET_ERR + 20)
#define	ENOTCONN		(NET_ERR + 21)
#define	ESHUTDOWN		(NET_ERR + 22)
#define	ETIMEDOUT		(NET_ERR + 23)
#define	ECONNREFUSED		(NET_ERR + 24)
#define	EPFNOSUPPORT		(NET_ERR + 25)
#define EHOSTDOWN		(NET_ERR + 26)
#define	EHOSTUNREACH		(NET_ERR + 27)

#endif	_OK_

#ifdef _OK_
#undef	_OK_
#endif

#endif	_NERRNO_

/*
 *	@(#)nerrno.h	11.1 (NRC)
 */
