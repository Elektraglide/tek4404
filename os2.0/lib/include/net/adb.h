/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.adb.h  */
/*  Version 11.1 as of 86/01/28 at 09:32:43  */

#ifndef _ADB_
#define	_ADB_

/* include <stdio.h> for definition of `FILE' */
#ifndef stdio_h
#include <stdio.h>
#endif

#ifndef	_STD_
#include <net/std.h>
#endif

typedef struct adb {
	FILE	* ad_fp;	/* fp into the data base */
	char	* ad_fields;	/* double null terminated field list */
	char	* ad_buf;	/* beg of record buffer */
	char	* ad_bufend;	/* end of record buffer */
	char	* ad_recend;	/* end of current record */
	pfi_t	ad_cmp;		/* optional key/value comparison function */
} adb;

adb	* adb_open ();		/* adp = adb_open(db_name) */
char	* adb_search();		/* cp1 = adb_search(adp, field_name, value) */
char	* adb_extract();	/* cp1 = adb_extract(adp, field_name, buf) */
char	* adb_next();		/* cp1 = adb_next(adp) */
boolean adb_upd();		/* adb_upd(db_name, srch_field, srch_value,
						rep_field, rep_value)	*/

#endif	_ADB_

/*
 *	@(#)adb.h	11.1 (NRC)
 */
