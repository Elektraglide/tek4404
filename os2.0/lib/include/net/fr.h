/*  Copyright (C) 10/27/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.fr.h  */
/*  Version 10.1 as of 85/10/27 at 12:45:07  */

/* FUSION router structures (see also 'netdev' structure in "netdev.h") */

#ifndef _FR_
#define	_FR_

#ifndef	_STD_
#include <net/std.h>
#endif
#ifndef	_SOCKET_
#include <net/socket.h>
#endif
#ifndef	_Q_
#include <net/q.h>
#endif

/* Router union is defined conditionally on the manifest 'FR_PROCESS',
 * which indicates that a FUSION router process is present; this process
 * will contain all information which is not used by 'fr_route' to target
 * link-layers on outbound data (such information as time-to-live, origin
 * of information, etc.); in the case that the router is within the kernel,
 * all this information is in the router union.
 */

#ifndef	FR_PROCESS

typedef struct fru {
	u16	ru_uflags;	/* see below */
	u16	ru_ttl;		/* time to live */
	u32	ru_era;		/* router entry age */
} fru;
/* values of 'ru_uflags' */
/* NONE YET! */

#endif	FR_PROCESS

/* Union of 'netdev' pointer and a 'u32' index (for use below) */
typedef	union	ndpu {
	struct	netdev	* dpu_p;
		u32	dpu_i;
} ndpu;

/* Union of 'frent' pointer and a 'u32' index (for use below) */
typedef union	frpu {
	struct	frent	* rpu_p;
		u32	rpu_i;
} frpu;

/* Router entry within the kernel; pointers are defined as unions of
 * the pointer (for use within the kernel), and an integer (for offsets
 * outside the kernel).  Because of this dual indexing, all entries
 * are required to be (at least for the purposes of a router process)
 * in a contiguous array; this abstraction is supported by the ioctl's
 * which interface to such a process.  There are two linkages through
 * this table (outside of the router union): linkage by network device
 * (head is in 'netdev' with a backpointer herein) and by address type
 * (head is in 're_ah' with implicit backpointer via address type).
 * A dummy entry is preallocated at the head of each linkage to allow
 * a process to do all manipulations of the list within the router
 * entry table; thus, no special logic is required to manipulate
 * pointers within structures outside of the router entry table.
 * For this external interface, the 'netdev' table is also viewed
 * as an array.
 */
typedef struct frent {
	u16	re_flags;	/* see below */
	u16	re_hops;	/* hops to get there */
	u16	re_rate;	/* output rate governor */
	u16	re_maxpkt;	/* maximum packet size */
	u32	re_delay;	/* estimated round trip in msec */
	ndpu	re_ndpU;	/* device to use */
	frpu	re_ndcU;	/* chain by (same) network device */
	frpu	re_nacU;	/* chain by (same) address type */
	rte	re_rte;		/* route key (see above) */
#ifndef	FR_PROCESS
	fru	re_ru;		/* router union */
#endif	FR_PROCESS
} frent;

/* manifest abbreviations */
#define re_net	re_rte.rte_net
#define re_gwy	re_rte.rte_gwy
#define re_secure	re_rte.rte_secure
#define	re_ndp	re_ndpU.dpu_p
#define re_ndi	re_ndpU.dpu_i
#define re_ndcp	re_ndcU.rpu_p
#define re_ndci	re_ndcU.rpu_i
#define re_nacp	re_nacU.rpu_p
#define re_naci	re_nacU.rpu_i
#ifndef	FR_PROCESS
#define	re_uflags	re_ru.ru_uflags
#define re_era	re_ru.ru_era
#define re_ttl	re_ru.ru_ttl
#endif	FR_PROCESS
/* values of 're_flags' */
#define F_R_IN_USE	0x0001	/* non-empty one */
#define F_R_DELETED	0x0002	/* on its way out */
#define	F_R_VALID	0x0004	/* entry is valid for 'fr_send' */
#define F_R_REFED	0x0008	/* referenced recently */
#define	F_R_DIRECTLY_CONNECTED	0x0010	/* Sicilian ancestry */
#define F_R_ROUTER	0x0020	/* 'ask and ye shall receive ...' */
#define F_R_DUMMY	0x0040	/* placeholder for process interface */
#define	F_R_CHECKSUM	0x0080	/* route requires checksums, possibly noisy */
#define F_R_PERMANENT	(F_R_DIRECTLY_CONNECTED | F_R_DUMMY | F_R_HARD_WIRED)
#define F_R_EXT_FLAGS	(F_R_HARD_WIRED)	/* outside allowances */
/* limits of 're_rate' */
#define RE_FASTEST	1	/* fastest (no delays) */
#define RE_SLOWEST	8192	/* slowest (max delays) */

/* Router table scanning definitions; these allow protocols to perform
 * fancier things such as limited broadcast, multicast, etc. without
 * having to know all the nitty-gritty.
 */
typedef struct frsb {	/* scan control structure */
	u8	rsb_mode;	/* see below */
	u8	rsb_how;	/* see below */
	u16	rsb_pad1;
	u16	rsb_mask;	/* scan mask */
	u16	rsb_flags;	/* masked flag match criterion */
	frent	* rsb_re;	/* currency on list */
	char	* rsb_ppad1;
} frsb;
/* scanning modes */
#define RS_BY_ADDRESS	1	/* scan by address family */
#define RS_BY_DEVICE	2	/* scan by device */
/* scanning protection ('how's) */
#define RS_EXCL	1	/* scan w/ exclusive (r/w) use */
#define RS_SAFE	2	/* scan w/ safe (r/o) use */

/* Router error block; handles errors generated on abortive attempts to
 * modify the router tables in an unnatural way (if FR_SANITY is defined).
 * In the case of FR_PROCESS, the 'ioctl' will return 'EINVAL'
 * whereupon the values may be 'rdiddle'd via the structure
 * 'fr_errblk'.  In the absence of FR_PROCESS, this structure is
 * directly accessible within the kernel.
 */
typedef struct frerrb {
	u16	rerr_type;	/* error type (see below) */
	u16	rerr_pad1;
	u32	rerr_arg[2];	/* error argument(s) */
} frerrb;

/* Error block type values along with contents of argument locations;
 * error type and arguments are found in 'fr_errblk'; argument content codes
 * given in parentheses are:
 *	af# -- address family #, 
 *	nd# -- network device #,
 *	re# -- router table entry #.
 */
#define FE_AFDUMMY	1	/* (af#): no dummy entry for AF */
#define FE_AFFREE	2	/* (af#,re#): free entry in AF chain */
#define FE_AFAF		3	/* (af#,re#): bad AF in AF chain */
#define FE_AFLIST	4	/* (af#): corrupt AF chain */
#define FE_AFUNREACH	5	/* (re#): entry unreachable by AF */
#define FE_DEVDUMMY	6	/* (nd#): no dummy entry for dev */
#define FE_DEVFREE	7	/* (nd#,re#): free entry in dev chain */
#define FE_DEVDEV	8	/* (nd#,re#): bad dev # in dev chain */
#define FE_DEVLIST	9	/* (nd#): corrupt dev chain */
#define FE_DEVUNREACH	10	/* (re#): entry unreachable by dev */

/* Structure mapping link-layer packet type to/from address family */
typedef struct aftmap {
	u32	tm_type;	/* packet's type */
	u16	tm_af;		/* address family */
	i16	tm_arp_needed;	/* ARP needed for this AF on this LLAF */
} aftmap;

/* Structure of headers for address family (see 'ahp_tbl' in "ncaf.c") */
typedef	struct afh {	/* NOTE: indexed by address family */
	i16	ah_valid;	/* good and usable one */
	u16	ah_alen;	/* length of these addresses */
	pfi_t	ah_init;	/* initialize address family */
	st	ah_up;		/* inbound packet processor */
	pfb_t	ah_hmatch;	/* net host address matching function */
	pfb_t	ah_nmatch;	/* net address matching function */
	pfb_t	ah_smatch;	/* net security matching function */
	pfb_t	ah_ask;		/* router request function */
	pfi_t	ah_dink;	/* dink-before-send function */
	pfi_t	ah_haddr;	/* disect saddr (for ARP) */
	st	ah_send[NLLAF];	/* send function per llaf */
	frent	* ah_re;	/* list of router entries */
	char	* ah_ppad1;
} afh;
#define fbnil	((pfb_t)0)
#define finil	((pfi_t)0)

/* Structure of headers for link layer address families (see 'llahp_tbl'
 * in "ncaf.c").
 */
typedef	struct llafh {	/* NOTE: indexed by link layer address family */
	u16	lah_alen;	/* length of these addresses */
	u16	lah_hsize;	/* link layer header size */
	u16	lah_maxpkt;	/* maximum packet size */
	u16	lah_arp_type;	/* ARP type */
	saddr	lah_broadcast;	/* broadcast address for this LLAF */
	aftmap	* lah_map;	/* map AF type to/from LLAF type */
	char	* lah_ppad1;
} llafh;

/* Queued packet processing structure */
typedef struct qtb {  /* queue type block */
	mq	qtb_mq;		/* head of queued functions */
	u16	qtb_ins;	/* count of active parties herein */
	u16	qtb_waits;	/* count of sleepers */
} qtb;
import	afh	* ahp_tbl[];
import	qtb	fr_eqtb, fr_sqtb;
import	int	fr_in(), fr_multi(), fr_unsafe();
import	st	fr_qin(), fr_o(), fr_r(), fr_s();
#define fr_excl(f)	fr_in((f),&fr_eqtb)
#define	fr_qexcl(f,m)	fr_qin(f,m,&fr_eqtb)
#define	fr_qsafe(f,m)	fr_qin(f,m,&fr_sqtb)
#define fr_safe(f)	fr_in((f),&fr_sqtb)
/* dispatch message along a predefined route (MUST be in safe context) */
#define fr_out(m)	fr_o(m,ahp_tbl[(u16)(m)->m_dest.a_type])
/* route message according to routing tables */
#define fr_route(m)	fr_qsafe((st)fr_r,m)
/* dispatch message to a predefined device */
#define fr_send(m)	fr_s(m,ahp_tbl[(u16)(m)->m_dest.a_type])

#ifdef RINADS
/* Router initial installation structure; allows devices to be initialized
 * with addresses and network number from patched values (see 'rinads'
 * in "ncaf.c").
 */
typedef	struct rinad {
	char	rin_name[MAX_DEV_NAME];
	saddr	rin_addr;	/* device's address w/ AF */
} rinad;
#endif	RINADS

/* Included down here because if WANT_ROUTER_PROCESS is defined, the
 * structure 'frent' must be also before we can to this inclusion
 */
#ifndef _FRCTLM_
#include <net/frctlm.h>
#endif

#endif	_FR_

/*
 *	@(#)fr.h	10.1 (NRC)
 */
