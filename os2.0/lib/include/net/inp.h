/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.inp.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:25  */

/* Intelligent node processor .
 * Structures and constants common to the host and node resident code.
 */

#ifndef _INP_
#define _INP_

#define	MBOX_IDLE	0
#define	HNC_MAX		127

/* Node to Host Commands.
 * NHC_SIGNAL requests that signal (software interrupt) 'mb_p1' be delivered
 *  to the process which owns 'mb_sid'.  This will only be given if
 *  the process requested the service via an async() library call.
 * NHC_PRINTF requests the host to print out the null terminated ascii
 *  message pointed at by 'mb_p1' on the console.
 * NHC_PANIC indicates that the node processor is dying.  A null terminated
 *  message telling why is pointed to by 'mb_p1'.
 */
#define	NHC_SIGNAL	(HNC_MAX + 1)	/* deliver signal 'mb_p1' */
#define	NHC_PRINTF	(HNC_MAX + 2)	/* print this message on console */
#define	NHC_PANIC	(HNC_MAX + 3)	/* I'm giving up the ghost */
#define	NHC_NTSIGNAL	(HNC_MAX + 4)	/* please call nt_signal for me */
#define	NHC_NTABORT	(HNC_MAX + 5)	/* call nt_abort */
#define	NHC_INIT_DONE	(HNC_MAX + 6)	/* initialization response */
#define	NHC_DONE	(HNC_MAX + 7)	/* node processor is done */
#define	NHC_ATTACH	(HNC_MAX + 8)	/* attach status */
#define	NHC_WB_BUFF	(HNC_MAX + 9)	/* buffer for write behinds */

/* Host to Node Commands.  They are used to initiate host ==> node
 * (network) transactions.  HNC_IOCTL is just a pass through from the user.
 * The 'cmnd' and  'ptr' from the user's ioctl(fd, cmnd, ptr) should be
 * passed through in 'mb_p1' and 'mb_src' respectively.
 */
#define	HNC_OPEN	1	/* open up a socket */
#define	HNC_CLOSE	2	/* close the socket indicated in mb_sid */
#define	HNC_READ	3	/* read data into 'mb_dest' */
#define	HNC_WRITE	4	/* write the indicated data */
#define	HNC_IOCTL	5	/* escape mechanism for 'attach', 'bind' ... */
#define	HNC_ABORT	6	/* about that requested action, ... forget it*/
#define HNC_PROBE	7	/* are you still there ?? */

/* A word on direction: host mailboxes are filled by the np and are emptied
 * by the host, mailboxes are filled by the host and emptied by the np.
 * NOTE: mbox.mb_cmnd must align with init.i_version
 * NOTE: mbox.mb_status must align with init.i_u32_pattern
 */
typedef struct mbox {
	u16	mb_cmnd;	/* what we want done */
	u16	mb_hostid;	/* 0 means unallocated/unused */
	u32	mb_status;	/* what happened */
	u32	mb_sid;		/* socket id */
	u32	mb_tid;		/* transaction id */
	u32	mb_p0;		/* misc. parameter/result */
	u32	mb_p1;		/* misc. parameter/result */
	u32	mb_src;		/* where the el_arr is located on the host */
} mbox;

/* First message sent to the node. Note that the version number must be
 * between 0 and 255 since it demonstrates the order of 16 bit entities.
 * By aligning the fields mentioned below we allow common code in
 * interrupt handlers and termination code.
 * NOTE: mbox.mb_cmnd must align with init.i_version
 * NOTE: mbox.mb_status must align with init.i_u32_pattern
 */
typedef struct {
	u16	i_version;	/* between 0-255, demonstrates 16 bit order */
	u16	i_pad1;
	u32	i_u32_pattern;	/* used to determine 32 bit flips */
	u32	i_printfp;	/* address of printf buffer on host */
	u16	i_first_sig;	/* first valid signal */
	u16	i_last_sig;	/* last good one */
} init;

#endif _INP_

/*
 *	@(#)inp.h	10.1 (NRC)
 */
