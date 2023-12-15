/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.courier.h  */
/*  Version 11.1 as of 86/01/28 at 09:32:57  */

#ifndef _COURIER_
#define _COURIER_

/* courier types and definitions */

#define	BRIDGE_PROGRAM	3
#define	BRIDGE_VERSION	2

#ifdef boolean
#undef boolean
#endif boolean

#define	boolean		a16	/* values 0..1 */
#define	cardinal	a16	/* values 0..65535 */
#define	longcardinal	a32	/* values 0..4294967295 */
#define	integer		a16	/* values -32768..32767 */
#define	longinteger	a32	/* values -2147483648..2147483647 */
#define	unspecified	a16	/* values 0..65535 */

typedef	struct version_t {
	cardinal	lowest;
	cardinal	highest;
} version;

typedef	struct string_t {
	cardinal	length;		/* number of bytes */
	char		bytes[2];	/* variable size */
} string;

/* defines for procedure field */
#define	CREATE		2
#define	DELETE		3
#define	SET_PARAM	9
#define	READ_PARAM	10
#define	SET_READ_PARAM	11

typedef	struct call_t {
	a16		choice;
	unspecified	transaction_id;
	longcardinal	programNumber;
	cardinal	versionNumber;
	cardinal	procValue;
} CallMessageBody;
 
typedef	struct reject_t {
	a16		choice;
	unspecified	transaction_id;
	struct {
		cardinal	rd_choice;
		cardinal	ImplementedVersionNumbers;
	} rejectionDetails;
} RejectMessageBody;
 
typedef	struct return_t {
	a16		choice;
	unspecified	transaction_id;
	unspecified	procResults[20];
} ReturnMessageBody;
 
typedef	struct abort_t {
	a16		choice;
	unspecified	transaction_id;
	cardinal	errorValue;
} AbortMessageBody;
 
/* Choices */
#define	CALLV	0
#define	REJECTV	1
#define	RETURNV	2
#define	ABORTV	3

typedef union message_t {
	a16			mu_choice;
	CallMessageBody		mu_call;
	RejectMessageBody	mu_reject;
	ReturnMessageBody	mu_return;
	AbortMessageBody	mu_abort;
} Message;

#endif	_COURIER_

/*
 *	@(#)courier.h	11.1 (NRC)
 */
