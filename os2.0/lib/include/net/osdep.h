/*  Copyright (C) 5/9/85 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: ./s.osdep.h  */
/*  Version 1.15 as of 85/05/09 at 19:20:45  */

#ifndef	_OSDEP_
#define	_OSDEP_

#include <net/ftype.h>

/* NOTE: watch your step!  Need to avoid redefining this guy w/i the kernel! */
#ifdef	KERNEL
#define	bzero				md_bzero
#define	cheql				md_cheql
#define	strcmp				md_strcmp
#define net_ioctl(fd, elp)		so_ioctl(kdev(fd), el_ioc(elp), elp, 0)
#endif	KERNEL

#define	NET_ERR				80
#define	INET_ERR			-1
#define	GOOD_EXIT			0
#define	BAD_EXIT			1

/* Unix O/S Family **********************************************************/
#ifdef	UNIX

#define	INFO_SOCKET			"/dev/fusion/so000"
#define SOIOCBASE			('s' << 8)
#ifndef net_ioctl
#define	net_ioctl(fd, elp)		ioctl(fd, el_ioc(elp), (char *)elp)
#endif	net_ioctl

#ifdef	KERNEL
#include <net/nerrno.h>

#define	NET_SIG				(NSIG - 1)
#define	NO_PRIORITY			-10000	/* any invalid priority value*/
#define	NET_TIMER			NET_PRIORITY
#define	is_kdev(dev)			((major(dev) & 0xFF) == 0xFF)
#define	kdev(dev)			((u16)(makedev(0xFF, minor(dev))))
#define os_gsignal			gsignal
#define	os_normal(priority)		splx(priority)
#define	os_printf			printf
#define	os_wakeup(event)		wakeup(event)
#define	os_critical			spl6
#define	ioc_base(cmnd)			((u16)(cmnd) & 0xff00)

#ifdef	INP
#undef	os_wakeup
#define	os_wakeup(event)		os_swakeup(event, 0)
#endif	INP

#endif	KERNEL

#ifdef	V7
#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)

#ifdef KERNEL

#undef	os_critical
#undef	os_gsignal
#define	os_move(src, dst, cnt)		blt(dst, src, cnt)
#define	os_critical			spl5
#define os_gsignal			signal
#define	os_ssleep()			save(u.u_qsav)

#else KERNEL

/* memory block operations from 4.2 */
#define	bcopy(b1,b2,l)			(void)memcpy(b2,b1,l)
#define bcmp(b1,b2,l)			memcmp(b1,b2,l)
#define bzero(b,l)			(void)memset(b,0,l)

#define gettimeofday			gettod
#define settimeofday			settod

#endif	KERNEL
#endif	V7

#ifdef	SYS5
#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)
/* string functions for older programs (thank you Watson) */
#define index				strchr
#define rindex				strrchr

#ifdef KERNEL

#ifndef	minor
#include <sys/sysmacros.h>
#endif	minor
#undef	os_gsignal
#define	os_gsignal			signal
#ifdef VAX
#define	os_move(src, dst, cnt)		bcopy(src, dst, cnt)
#else
#define	os_move(src, dst, cnt)		blt(dst, src, cnt)
#endif
#ifdef	UNISOFT5
#define	os_ssleep()			save(u.u_qsav)
#else	UNISOFT5
#define	os_ssleep()			setjmp(u.u_qsav)
#endif	UNISOFT5
#ifdef	MOTO_5
#undef	os_move
#define	copyin(src, dest, cnt)		mmuread(src, dest, cnt)
#define	copyout(src, dest, cnt)		mmuwrite(dest, src, cnt)
#endif	MOTO_5

#else	KERNEL

#define gettimeofday			gettod
#define settimeofday			settod

#endif	KERNEL
#endif	SYS5

#ifdef	SYS3
#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)

#ifdef	KERNEL

#ifndef minor
#include <param.h>
#endif	minor
#ifndef	MASSCOMP
#undef	os_gsignal
#define	os_gsignal			signal
#endif	MASSCOMP
#define	os_ssleep()			setjmp(u.u_qsav)

#else	KERNEL

#define gettimeofday			gettod
#define settimeofday			settod

#endif	KERNEL
#endif	SYS3

#ifdef	B41
#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)

#ifdef	KERNEL
#define	os_ssleep()			setjmp(u.u_qsav)
#endif	KERNEL
#endif	B41

#ifdef	B42
#ifndef	_IOCTL_
#include <sys/ioctl.h>
#endif	_IOCTL_

#define	our_direct			direct
#define	od_name				d_name
#define	od_namlen			d_namlen
#define	SOIOCGNTTY			(IOC_VOID | (SOIOCBASE-2))
#define	SOIOCSNTTY			(IOC_VOID | (SOIOCBASE-1))

#ifdef KERNEL

#define	EL_ACCESSIBLE
#undef	os_printf
#define	os_printf			uprintf
#define	socket				Fsocket
#define	bind				Fbind
#define	connect				Fconnect
#define	recv				Frecv
#define	recvfrom			Frecvfrom
#define	send				Fsend
#define	sendto				Fsendto
#define	socketpair			Fsocketpair
#undef	bzero
#define	md_bzero			to stop compilation of md_bzero
#define	md_cheql			!bcmp
#define	os_ssleep()			setjmp(&u.u_qsave)

#else	KERNEL

#undef	net_ioctl
#define	net_ioctl(fd, elp)		ioctl(fd,IOC_IN|((el_size(elp)&IOCPARM_MASK)<<16) | el_ioc(elp),(char *)elp)

#endif	KERNEL
#endif	B42

#ifdef	VENIX
#undef	SIGIO
#undef	NET_SIG
#define	SIGIO				SIGEMT
#define	NET_SIG				SIGIO

#ifdef	KERNEL

#undef	os_critical
#undef	os_gsignal
#ifdef	INP
#define	os_move				sdcp
#undef	use_critical
#undef	critical
#undef	normal
#define	use_critical	/**/
#define	critical	/**/
#define	normal		/**/
#else	INP
#ifndef VENIX11
#define	os_move				vx_move
#define	os_critical			spl5
#else
#include <lc.h>
#ifndef XFUSION
#define	os_move				sdcp
#endif
#define	os_critical			spl4
#endif VENIX11
#endif	!INP
#define os_gsignal			signal

#else	KERNEL

#define gettimeofday			gettod
#define settimeofday			settod

#endif	KERNEL
#endif	VENIX

#endif	UNIX
/* End of Unix O/S Family ****************************************************/

#ifdef TEK4404	
#define	SIGIO				(NSIG-1)
#define	SIGURG				(NSIG-2)
#ifndef time_t
#include <sys/types.h>
#endif
#undef net_ioctl
#define	net_ioctl(fd, elp)		_net_ioctl(fd, elp)

/* memory block operations from 4.2 */
#define	bcopy(b1,b2,l)			(void)memcpy(b2,b1,l)
#define bcmp(b1,b2,l)			memcmp(b1,b2,l)
#define bzero(b,l)			(void)memset(b,0,l)

#define gettimeofday			gettod	
#define settimeofday			settod	

#undef	SOIOCBASE
#define	SOIOCBASE			((unsigned)32757)
#undef	SOIOCATMARK
#define	SOIOCATMARK			(32754) /* SOIOCBASE - 3 */
#define	ENIOCBASE			100      /* Ethernet specific ioctls */

/* add some handy errors missing on tek */
#define	EPERM				NET_ERR-1
#define	ENXIO				NET_ERR-2
#define ENODEV				NET_ERR-3
#define ENFILE				NET_ERR-4
#define EFBIG				NET_ERR-5
#define EROFS				NET_ERR-6


#endif TEK4404

#endif	_OSDEP_

/*
 *	@(#)osdep.h	1.15 (NRC)
 */
