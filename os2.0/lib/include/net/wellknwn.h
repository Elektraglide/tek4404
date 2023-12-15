/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.wellknwn.h  */
/*  Version 11.1 as of 86/01/28 at 09:35:05  */

/*  Wellknown addresses and sockets  */

#ifndef _WELLKNWN_ 
#define	_WELLKNWN_

/* Fusion intramachine stuff (AF_INTRA) */

#define NRC_RC_ROUTER_CONTROL	"$nrc/rtrctl"
#define NRC_RK_ROUTER_KERNEL	"$nrc/rtrkrnl"
#define NRC_SERVER_CONTROL	"$nrc/serverctl"	/* server likes to talk to itself */

/* Xerox Network Systems stuff (AF_NS) */

/* network numbers */
#define XN_UNKNOWN	{0, 0, 0, 0}		/* Unknown network */

/* host addresses */
#define XH_UNKNOWN	{0, 0, 0, 0, 0, 0}	/* Unknown host */
#define XH_BROADCAST	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} /* All hosts */

#define	XS_I_ECHO	2			/* Echo socket */
#define XS_I_COURIER	5			/* Courier server socket */

/* sockets */
#define XS_UNKNOWN	{0, 0}			/* Unknown socket */
#define	XS_ROUTE_INFO	{0, 1}			/* Routing info socket */
#define	XS_ECHO		{0, XS_I_ECHO}		/* Echo socket */
#define	XS_ERR_ROUTE	{0, 3}			/* Routing error socket */
#define	XS_COURIER	{0, XS_I_COURIER}	/* Courier server socket */

#endif	_WELLKNWN_

/*
 *	@(#)wellknwn.h	11.1 (NRC)
 */
