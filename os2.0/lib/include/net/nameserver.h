/*
 * Nameserver packet format
 *
 * $Header: nameserver.h,v 20.0 85/03/11 06:43:25 steveg Stable $
 * $Locker:  $
 * 
 * Copyright (c) 1984, Tektronix Inc.
 * All Rights Reserved
 *
 */

#ifndef MAXHOSTNAMESIZE
#include <sys/max.h>
#endif

#define NS_VERSION	3		/* version # nameserver format */

#define MAXALIASES	7		/* max # hostname aliases */

#define NSR_ERROR	0		/* error in request */
#define	NSR_ANSWER	1		/* answer to request */
#define	NSR_GETNAME	2		/* Gethostbyname() */
#define	NSR_GETADDR	3		/* Gethostbyaddr() */
#define NSR_STATUS	4		/* Dump hostname/addresss list */
#define NSR_DELNAME	5		/* delete all entries for name */
#define NSR_DELADDR	6		/* delete all entries for address */

struct ns_req {
	u_short		nr_version;			/* version of */
	u_short		nr_type;			/* type of request */
	struct in_addr	nr_addr;			/* address of host */
	char		nr_host[MAXHOSTNAMESIZE];	/* name of host */
	char		nr_aliases[MAXALIASES][MAXHOSTNAMESIZE]; /* aliases */
};

/* Communications is via a intraMachine domain stream socket. */
#define	NAME_SOCK	"/tmp/nameServer"
