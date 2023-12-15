/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.intra.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:41  */

#ifndef	_INTRA_
#define	_INTRA_

#ifndef	_STD_
#include <net/std.h>
#endif

/* protocol identifiers */
#define	INTRA_DGRAM	0	/* intramachine datagram */
#define	INTRA_STREAM	1	/* intramachine byte stream */

/* packet sizes */

#define	INTRA_PSIZE	4096
#define	INTRA_CPSIZE	4096

/* intramachine header (datagrams) */
typedef struct ih {
	u16		 ih_length;
	u16		 ih_packet_type;
} ih;

typedef struct str_t { /* intramachine name storage */
	struct	str_t	* st_next;  	/* ptr. to next in list */
	struct	str_t	* st_prev;	/* ptr. to prev in list */
	u16		 st_refcnt;	/* # of times referenced */
	u16		 st_len;	/* maps over so_addr */
	u16		 st_type;
	char		 st_name[2];
} str_t;

#endif	_INTRA_

/*
 *	@(#)intra.h	11.1 (NRC)
 */
