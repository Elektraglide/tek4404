/*  Copyright (C) 4/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.adbnrc.h  */
/*  Version 11.3 as of 86/04/28 at 09:48:43  */

#ifndef	_ADBNRC_
#define	_ADBNRC_

#define N_CURRENT_DB_VERSION	"3.1.0"

/* lists of field names used within the standard NRC 'adb' databases;
 * allows recompilation for different human languages, since these
 * names are known to the system administrator
 */
#ifndef	N_NAME
#define ENGLISH  /* default is (Gringo) English */
#endif

#ifdef	ENGLISH
/* generic names used in various files */
#define N_ADDR		"addr"	/* host portal address w/ security */
#define	N_CMND		"cmnd"	/* command to use */
#define N_CODE		"code"	/* short numeric handle / value */
#define N_COMMENT	"comment"	/* non-machine processable comments */
#define N_DELAY		"delay"	/* time delays (in msec) */
#define N_FLAGS		"flags"	/* general binary flags */
#define N_HOPS		"hops"	/* routing hops */
#define N_IMAGE		"image"	/* download image for intelligent boards */
#define N_LFIG		"lfig"	/* link layer config handle */
#define N_MFIG		"mfig"	/* machine config handle */
#define N_NAME		"name"	/* proper name */
#define	N_OPTIONS	"options"
#define	N_P0		"p0"	/* long general parameter */
#define	N_P1		"p1"	/* " */
#define	N_P2		"p2"	/* " */
#define	N_P3		"p3"	/* " */
#define N__P0		"_p0"	/* secondary long general parameter */
#define	N__P1		"_p1"	/* " */
#define	N__P2		"_p2"	/* " */
#define	N__P3		"_p3"	/* " */
#define N_PATH		"path"	/* file name value */
#define	N_PROTOCOL	"protocol" /* protocol type, see socket.h */
#define	N_PORT		"port"	/* port # to use (socket) */
#define N_RTE		"route"	/* host via route w/ security */
#define	N_SERVICE	"service" /* service name */
#define N_STATE		"state"	/* device state value */
#define N_TYPE		"type"	/* variable type indicator */
#define N_VALUE		"value"	/* arbitrary value */

/* sizes of items (in chars incl. terminator) */
#define	NN_ADDR		(27+33)
#define NN_CODE		NN_SHORT
#define NN_COMMENT	128
#define NN_DELAY	NN_LONG
#define	NN_FLAGS	NN_CODE
#define NN_GADDR	(NN_NAME+2+NN_ADDR)
#define NN_HOPS		NN_CODE
#define NN_IMAGE	80
#define NN_LFIG		6
#define NN_LONG		(2*NN_SHORT)
#define NN_MFIG		24
#define NN_NAME		32
#define NN_P		NN_LONG
#define NN_PATH		128
#define NN_RTE		(27+27+33)
#define NN_SHORT	10
#define NN_STATE	NN_CODE
#define NN_TYPE		4
#define NN_VALUE	128	/* defined to be the largest */

/* handles used in the FUSION database
 * the name of this file is defined in "nc.c" in the kernel source and
 * is retrieved via the info socket for user processes
 */
#define NF_AFDB		"af_db"
#define NF_ARPDB	"arp_db"
#define NF_BINPREFIX	"bin_prefix"
#define NF_CONSOLE	"console"
#define NF_CURTTY	"cur_tty"
#define NF_DEFLOGIN	"default_login"
#define NF_DEFTTTYPE	"default_termtype"
#define NF_DST_ADJUST	"dst_adjustment"
#define NF_ERRNODB	"errno_db"
#define NF_KCONFIGDB	"kconfig_db"
#define NF_LFIGDB	"lfig_db"
#define NF_LIBPREFIX	"lib_prefix"
#define NF_M_WEST_GMT	"minutes_west_gmt"
#define NF_MACHINEDB	"machine_db"
#define NF_MAXNTTYS	"max_nttys"	/* old version */
#define NF_MAXSOCKETS	"max_sockets"	/* old version */
#define NF_MFIGDB	"mfig_db"
#define NF_MISCPREFIX	"misc_prefix"
#define NF_MSGDB	"msg_db"
#define NF_MYNAME	"my_name"
#define NF_NETDB	"net_db"
#define NF_OS		"os"
#define NF_SERVERDB	"server_db"
#define NF_SRVDB	"service_db" /* for compatibility */
#define	NF_SERVICEDB	"service_db"
#define NF_TTYFILE	"ttys_file"
#define NF_TTYNPREFIX	"ttyn_prefix"
#define NF_TTTYPEFILE	"ttytype_file"
#define NF_SRVSOCK	"server_socket"	/* should be from service database */
#define NF_TIMESOCK	"time_socket"	/* should be from service database */
#define NF_SPOOLSOCK	"spooler_socket"	/* should be from service database */
#define NF_VERSION	"version"

/* handles used in the 'arp_db' file
 * the name of this file is defined in the FUSION database
 */
#define NARP_LADDR	"laddr"
#define NARP_PADDR	"paddr"

/* handles used in the 'net_db' file
 * the name of this file is defined in the FUSION database
 */
#define	NE_EXISTS	"exists"
#define NNE_EXISTS	NN_CODE

/* handles used in the 'machine_db' file
 * the name of this file is defined in the FUSION database
 */
#define NM_SITE		"site"	/* site's pen-name */
/* NOTE: contains some number of AF domains; see 'af_db' */

/* handles used in the 'msg_db'
 * the name of this file is defined in the FUSION database
 */
#define NM_FMT		"fmt"	/* printing format string */

/* handles used in the 'net_db' file
 * the name of this file is defined in the FUSION database
 */
/* NOTE: contains some number of AF domains; see 'af_db' */

#endif	ENGLISH

/* definitions of types in the kernel config database */
#define FT_BOOLEAN	1	/* "b" */
#define FT_I16		2	/* "i" */
#define FT_I32		3	/* "I" or "il" */
#define FT_STRING	4	/* "s" */
#define FT_U16		5	/* "u" */
#define FT_U32		6	/* "U" or "ul" */

import	char	* fnget_str(), * nget_str();
import	void	check_db_version();

#endif	_ADBNRC_

/*
 *	@(#)adbnrc.h	11.3 (NRC)
 */
