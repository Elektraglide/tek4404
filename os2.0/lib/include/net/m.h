/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.m.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:36  */

#ifndef	_M_
#define	_M_

/* Definitions and Macros for dealing with the `Message' structure */

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_Q_
#include <net/q.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif

/* flags for m.m_flags */
#define	F_M_DELAY	0x0001		/* delay call on board start */
#define	F_M_CLR_Q_FLAGS	0x0002		/* clear q_flags upon q insertion */
#define	F_M_EVEN	0x0004		/* data must be even size, compensate */
#define F_M_ORIGIN	0x0008		/* packet is from this host */
#define F_M_ONCE_ROUTED	0x0010		/* packet went through router once */
#define	F_M_ZAP		0x0040		/* to be unlocked soon */
#define F_M_STAY_LOCKED	0x0080		/* tell s.m. not to unlock when done */
#define	F_M_MSM		0x0100		/* currently being driven by s.m. */
#define	F_M_CRITICAL	0x0200		/* this msg is CRITICAL now */

#define	m_dsize(mp)		((mp)->m_tp - (mp)->m_hp)
#define	m_empty(mp)		((mp)->m_tp == (mp)->m_hp)
#define	m_hptr(mp,type)		((type *)((mp)->m_hp -= sizeof(type)))
#define	m_htrim(mp,size)	((mp)->m_phdr = (mp)->m_hp,(mp)->m_hp += ((mp)->m_phlen = size))
#define	m_ptr(mp,type)		((type *)((mp)->m_hp))
#define	m_release(mp)		bC2((mp)->m_q.q_flags,F_Q_PRESERVE,F_Q_LOCKED)
#define	m_tptr(mp,type)		((type *)((mp)->m_tp))
#define	m_trim(mp,size)		((mp)->m_hp += size)
#define	m_ttrim(mp,size)	(m_dsize(mp) >= (size) ? (((mp)->m_tp -= size),0):ERR)
#define	mq_init(mqp)		((mqp)->mq_hmp = nil,(mqp)->mq_tmp = nil)
#define	mq_head(mqp)		((mqp)->mq_hmp)
#define	in_mq(mp)		((mp)->m_next)
#define	is_origin(mp)		bT1((mp)->m_flags,F_M_ORIGIN)
#define	is_remote(mp)		bF1((mp)->m_flags,F_M_ORIGIN)

/* start a work list */
#define	msm(mp,job)	((mp)->m_job=(job),sm_msm(mp))
/* kill an mp */
#define smkill(mp)	((mp)->m_flags|=F_M_ZAP,msm((mp),(mp)->m_dispfn))

#ifdef	MSDOS
/* Lousy Lattice Compiler (LLC) hates zero casts */
#define s_offset(s,f,p)	((int)&((s *)(p))->f - (int)(p))
#else	MSDOS
#define	s_offset(s,f,p)	((int)&(((s *)0)->f))
#endif	MSDOS
#define s_ptr(s,f,p)	((s *)((int)(p) - s_offset(s,f,p)))

/* state machine type definitions */
#ifdef	M_HISTORY
import	void	smfhist(), smhprint(), smhzap(), smuhist();
#else	M_HISTORY
#define smfhist(m,f)
#define smhprint(m)
#define smhzap(m)
#define smuhist(m,u)
#endif	M_HISTORY

/* internal message structure */
typedef	struct mm { /* mini-message */
	q		mm_q;		/* for send queue linkages (q, gq) */
	struct gq	* mm_gqp;	/* pointer to queue head */
	struct m	* mm_next;	/* for message queue linkages (mq) */
	q		mm_wq;		/* message work q */
	st		mm_job;		/* first state of this work message */
	st		mm_dispfn;	/* function to put it away */
	u16		mm_flags;
	i16		mm_err;		/* error status */
	u32		mm_p0;		/* general parameter use */
	u32		mm_p1;
	u32		mm_p2;
	u32		mm_p3;
	u16		mm_savpri;	/* old priority if we're critical */
	struct so_t	* mm_sop;	/* socket pointer */
	union {			/* side-car parameter */
		struct netdev	* mms_ndp;	/* device pointer */
		struct frent	* mms_rep;	/* router entry pointer */
	} mm_sidekick;
#ifdef	M_HISTORY
#define MAXHIST	24	/* Must be multiple of 4 to maintain alignment */
	u16		mm_zhist;	/* lost history counter */
	u16		mm_histi;	/* next available history index */
	u8		mm_histt[MAXHIST];	/* history type array */
	union {				/* remembered state history array */
		st	mmh_fn;		/* state function called */
		u32	mmh_u32;	/* stuff stored by function */
	} mmh[MAXHIST];
#endif	M_HISTORY
} mm;

#define mm_ndp	mm_sidekick.mms_ndp
#define mm_rep	mm_sidekick.mms_rep
#ifdef	M_HISTORY
#define mm_fn(i)	mmh[i].mmh_fn
#define mm_u32(i)	mmh[i].mmh_u32
#define mm_hist		mm_u32
#endif	M_HISTORY

typedef struct m {  /* a real message */
	mm		m_mm;		/* mini-message at the beginning */
	so_addr		m_dest;		/* only one allowed */
	so_addr		m_src;		/* other source specified */
	char		* m_phdr;	/* protocol header */
	char		* m_hp;		/* head pointer to trimmed data */
	char		* m_tp;		/* tail pointer to trimmed data */
	char		* m_cp;		/* place holder for protocols */
	char		* m_secp;	/* security field pointer */
	char		* m_ppad1;
	u32		m_type;		/* used by protocol level layers */
	u32		m_delay;	/* estimated delay to packet arrival */
	u32		m_phlen;	/* protocol header length */
	u16		m_pflags;	/* protocol flags */
	u16		m_pad1;
} m;

/* abbreviations to allow direct naming of mini-message structures */
#define m_q	m_mm.mm_q
#define m_gqp	m_mm.mm_gqp
#define m_next	m_mm.mm_next
#define	m_wq	m_mm.mm_wq
#define	m_job	m_mm.mm_job
#define m_flags	m_mm.mm_flags
#define m_p0	m_mm.mm_p0
#define m_p1	m_mm.mm_p1
#define m_p2	m_mm.mm_p2
#define m_p3	m_mm.mm_p3
#define m_sop	m_mm.mm_sop
#define m_ndp	m_mm.mm_ndp
#define m_rep	m_mm.mm_rep
#define m_err	m_mm.mm_err
#define	m_savpri m_mm.mm_savpri
#define m_dispfn	m_mm.mm_dispfn
#ifdef	M_HISTORY
#define m_zhist		m_mm.mm_zhist
#define m_histi		m_mm.mm_histi
#define m_histt(i)	m_mm.mm_histt[i]
#define m_fn(i)		m_mm.mm_fn(i)
#define m_u32(i)	m_mm.mm_u32(i)
#define m_hist		m_u32
#endif	M_HISTORY

#endif	_M_

/*
 *	@(#)m.h	10.1 (NRC)
 */
