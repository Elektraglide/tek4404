/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.netdb.h  */
/*  Version 11.1 as of 86/01/28 at 09:34:02  */

#ifndef	_NETDB_
#define _NETDB_

#ifndef	_STD_
#include <net/std.h>
#endif

/* Structures returned by network data base library.  All addresses
   are supplied in host order, and returned in network order (suitable
   for use in system calls). */

typedef struct	hostent {
	char	* h_name;	/* official name of host */
	char	** h_aliases;	/* alias list */
	u16	h_addrtype;	/* host address type */
	u16	h_length;	/* length of address */
	char	* h_addr;	/* address */
} hostent_t;

/* Assumption here is that a network number fits in 32 bits -- A poor one! */

typedef struct	netent {
	char	* n_name;	/* official name of net */
	char	** n_aliases;	/* alias list */
	u16	n_addrtype;	/* net address type */
	u32	n_net;		/* network # */
} netent_t;

typedef struct	servent {
	char	* s_name;	/* official service name */
	char	** s_aliases;	/* alias list */
	u16	s_port;		/* port # */
	char	* s_proto;	/* protocol to use */
} servent_t;

typedef struct	protoent {
	char	* p_name;	/* official protocol name */
	char	** p_aliases;	/* alias list */
	u16	p_proto;	/* protocol # */
} protoent_t;

/* These names have been shortened to ensure (?) portability onto
   brain-dead systems which have ridiculous name length limitations */

struct hostent	* ghbyname(), * ghbyaddr(), * ghent();
struct netent	* gnbyname(), * gnbyaddr(), * gnent();
struct servent	* gsbyname(), * gsbyport(), * gsent();
struct protoent	* gpbyname(), * gpbynumber(), * gpent();

struct hostent	* gethostbyname();
#define gethostbyaddr		ghbyaddr
#define gethostent		ghent
#define gethostname		ghname
#define	getnetbyname		gnbyname
#define getnetbyaddr		gnbyaddr
#define getnetent		gnent
#define	getservbyname		gsbyname
#define getservbyport		gsbyport
#define getservent		gsent
#define	getprotobyname		gpbyname
#define getprotobynumber	gpbynumber
#define getprotoent		gpent
#define sethostname		shname

#endif	_NETDB_

/*
 *	@(#)netdb.h	11.1 (NRC)
 */
