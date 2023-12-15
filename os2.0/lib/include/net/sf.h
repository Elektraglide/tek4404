/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.sf.h  */
/*  Version 10.1 as of 85/10/27 at 12:46:13  */

#ifndef _SF_
#define	_SF_

#define	sf_nil	(-1L)

#ifndef	PAGE_SIZE
#define	PAGE_SIZE	(1024L)
#endif

#define	sf_ilgetc(sfp) (((sfp)->s_top <= (sfp)->s_buf) ? sf_lgetc((sfp)): (*--(sfp)->s_bot = *--(sfp)->s_top))
#define	sf_irgetc(sfp) (((sfp)->s_bot >= (sfp)->s_eob) ? sf_rgetc((sfp)) : (*(sfp)->s_top++ = *(sfp)->s_bot++))

typedef	struct sf {
	char *	s_bot;		/* bottom side of split in buffer, */
				/* points to the current character */
	char *	s_top;		/* top side of split in buffer */
	char *	s_buf;		/* in core buffer */
	char *	s_eob;		/* end of in core buffer */
	int	s_o_fd;		/* original file descriptor */
	int	s_t_fd;		/* fd for top temp file */
	int	s_b_fd;		/* fd for bottom temp file */
	int	s_pad1;

	long	s_t_o_p;	/* physical bytes in top original file */
	long	s_t_o_l;	/* logical bytes in top original file */
	long	s_t_t_p;	/* physical bytes in top temp file */
	long	s_t_t_l;	/* logical bytes in top temp file */

	long	s_b_o_p;	/* physical bytes in bottom original file */
	long	s_b_o_l;	/* logical bytes in bottom original file */
	long	s_b_t_p;	/* physical bytes in bottom temp file */
	long	s_b_t_l;	/* logical bytes in bottom temp file */

	long	s_utsize;	/* unmodified top size */
	long	s_ubsize;	/* unmodified bottom size */
	long	s_usize;	/* unmodified file size (size before mods) */
	int	s_modified;
	int	s_pad2;
} sf;

#define	is_modified(sfp)	((sfp)->s_modified)
#define	reset_modified(sfp)	((sfp)->s_modified = false)

export	boolean	sf_modified(), sf_bind(), sf_eof();
export	int	sf_read(), sf_write(), sf_insert();
export	long	sf_tell(), sf_loc(), sf_seek(), sf_search();
export	sf	* sf_creat(), * sf_open();
export	unsigned sf_lgetc(), sf_rgetc();
export	void	sf_close(), sf_delete();

#endif _SF_

/*
 *	@(#)sf.h	10.1 (NRC)
 */
