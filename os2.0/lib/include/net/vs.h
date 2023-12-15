/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.vs.h  */
/*  Version 11.1 as of 86/01/28 at 09:35:00  */

#ifndef	_VS_
#define	_VS_

#define	V_BUF_SIZE		512	/* output buffering area */
#define	V_HIDDEN		65534	/* depth beyond which all is lost */

/* Terminal hardware description bits */
#define	F_V_I_LN	0x1		/* insert line */
#define	F_V_D_LN	0x2		/* delete line */
#define	F_V_I_CH	0x4		/* insert character */
#define	F_V_D_CH	0x8		/* delete character */
#define	F_V_C_EL	0x10		/* clear to end of line */
#define	F_V_C_EW	0x20		/* clear to end of window */
#define	F_V_C_SCR	0x40		/* clear screen */
#define	F_V_MM		0x80		/* memory mapped */
#define	F_V_S_UP	0x100		/* scrolls up on '\n' at bottom */
#define	F_V_WINDOWS	0x200		/* hardware windowing supported */
#define	F_V_HO_WINDOWS	0x400		/* supports horizontal windowing */
#define	F_V_COL_WRAP	0x800		/* cursor wraps on rightmost column */

#define	F_V_MASK	0xF000

/* Viewport specific flags */
#define	F_V_POSTED	0x1000		/* viewport is posted */
#define	F_V_MARKED	0x2000		/* viewport node has been marked for 
					** later use */

/* Macros for readability */
#define	has_i_ln(vp)		(vp->v_flags & F_V_I_LN)
#define	has_d_ln(vp)		(vp->v_flags & F_V_D_LN)
#define	has_i_ch(vp)		(vp->v_flags & F_V_I_CH)
#define	has_d_ch(vp)		(vp->v_flags & F_V_D_CH)
#define	has_c_el(vp)		(vp->v_flags & F_V_C_EL)
#define	has_c_ew(vp)		(vp->v_flags & F_V_C_EW)
#define	has_c_scr(vp)		(vp->v_flags & F_V_C_SCR)
#define	has_mm(vp)		(vp->v_flags & F_V_MM)
#define	has_s_up(vp)		(vp->v_flags & F_V_S_UP)
#define	has_windows(vp)		(vp->v_flags & F_V_WINDOWS)
#define	has_ho_windows(vp)	(vp->v_flags & F_V_HO_WINS)
#define	is_posted(vp)		(vp->v_flags & F_V_POSTED)
#define	is_marked(vp)		(vp->v_flags & F_V_MARKED)


/* Single character buffered terminal output.  Overflows into t_iputc(). */
#define	t_putc(tp, ch)	(tp->t_bufp < tp->t_bufe ? (*tp->t_bufp++ = ch) : t_iputc(tp, ch))

#define	a_ch	a_chars[0]
#define	a_att	a_chars[1]

typedef union {
	i16	a_i;
	char	a_chars[2];
} ac;

typedef struct td_s {
	struct vs *	t_vp;
	char *	t_term_type;
	u16	t_flags;
	int	t_fd;
	i16	t_row;
	i16	t_col;
	i16	t_rows;
	i16	t_cols;
	i16	t_v_row_off;
	i16	t_v_col_off;
	i16	t_ms_per_pad;
	pfi_t	t_fmv;
	pfi_t	t_fpaint;
	pfi_t	t_finit;
	ac   **	t_scr;
	u16  **	t_darr;
	u16  **	t_vdarr;
	char	t_pad_char;
	char	t_att;
	char *	t_bufb;
	char *	t_bufe;
	char *	t_bufp;
	char *	t_l;
	char *	t_u;
	char *	t_d;
	char *	t_c_el;
	char *	t_c_ew;
	char *	t_c_scr;
	char *	t_d_ch;
	char *	t_d_ln;
	char *	t_i_ch;
	char *	t_i_ln;
	char *	t_mv;
} td;

typedef struct vs {
	ac **		v_scr;
	td *		v_tp;
	u16		v_flags;
	struct vs *	v_dad;
	struct vs *	v_bro;
	struct vs *	v_son;
	u16		v_depth;
	i16		v_row;
	i16		v_col;
	i16		v_rows;
	i16		v_cols;
	i16		v_row_off;
	i16		v_col_off;
	char		v_att;
} vs;

import	ac **	new_scr(), * t_scr_ptr();
import	char	t_iputc(), * v_error();
import	void	v_set_err();
import	vs *	v_create(), * v_open(), * v_push();

import	char	* v_errp;

#endif	_VS_

/*
 *	@(#)vs.h	11.1 (NRC)
 */
