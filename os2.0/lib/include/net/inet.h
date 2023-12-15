/*  Module: h/b42ish       File: ./s.inet.h  */
/*  Version 1.2 as of 85/05/10 at 02:02:50  */

#ifndef	_INET_
#define	_INET_

/*	inet.h	4.1	83/05/28	*/

#include <net/std.h>
/*
 * External definitions for
 * functions in inet(3N)
 */
char	* inet_ntoa();
struct in_addr	*inet_addr(), *inet_makeaddr();
u32	inet_network();

#endif	_INET_

/*
 *	@(#)inet.h	1.2 (NRC)
 */
