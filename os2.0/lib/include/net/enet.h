/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.enet.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:08  */

#ifndef _ENET_
#define	_ENET_

#ifndef _STD_
#include <net/std.h>
#endif

#define	ENET_RAW	0	/* protocol handle */

#define	EN_MAX_DATA	1500
#define	EN_MIN_DATA	46
#define	EN_MIN_PACKET	(EN_MIN_DATA + EN_HDR_SIZE)
#define	EN_MAX_PACKET	(EN_MAX_DATA + EN_HDR_SIZE)
#define	EN_HDR_SIZE	14
#define	EN_JAM_MAX	16
#define	EN_BACKOFF_MAX	10	/* maximum backoff exponent */

typedef struct {
	a48	enh_dest;
	a48	enh_source;
	a16	enh_type;
} enh;

typedef struct en {
	a48	en_dest;
	a48	en_source;
	a16	en_type;
	char	en_data[EN_MAX_DATA];
	char	en_checksum[4];
} en_t;

/* Defined Ethernet types */

#define	EN_XNS		03000	/* XNS Packet */
#define	EN_IP		04000	/* Darpa Internet Protocol Packet */
#define	EN_ARP		04006	/* Address Resolution Protocol Packet */

#endif	_ENET_

/*
 *	@(#)enet.h	11.1 (NRC)
 */
