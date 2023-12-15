/*  Module: h       File: /nrc/rel/h/s.telnet.h  */
/*  Version 11.2 as of 86/03/28 at 07:18:25  */

/*	telnet.h	4.4	82/03/16	*/
#include <net/std.h>
#ifndef	_TELNET_
#define _TELNET_

#ifdef B42_COMPATIBLE
#define T_	TELOPT_
#endif
/*
 * Definitions for the TELNET protocol.
 */
#define	IAC	255		/* interpret as command: */
#define	DONT	254		/* you are not to use option */
#define	DO	253		/* please, you use option */
#define	WONT	252		/* I won't use option */
#define	WILL	251		/* I will use option */
#define	SB	250		/* interpret as subnegotiation */
#define	GA	249		/* you may reverse the line */
#define	EL	248		/* erase the current line */
#define	EC	247		/* erase the current character */
#define	AYT	246		/* are you there */
#define	AO	245		/* abort output--but let prog finish */
#define	IP	244		/* interrupt process--permanently */
#define	BREAK	243		/* break */
#define	DM	242		/* data mark--for connect. cleaning */
#define	NOP	241		/* nop */
#define	SE	240		/* end sub negotiation */

#define SYNCH	242		/* for telfunc calls */

/* telnet options */

#define T_BINARY	0	/* 8-bit data path */
#define T_ECHO		1	/* echo */
#define	T_RCP		2	/* prepare to reconnect */
#define	T_SGA		3	/* suppress go ahead */
#define	T_NAMS		4	/* approximate message size */
#define	T_STATUS	5	/* give status */
#define	T_TM		6	/* timing mark */
#define	T_RCTE		7	/* remote controlled transmission and echo */
#define T_NAOL 		8	/* negotiate about output line width */
#define T_NAOP 		9	/* negotiate about output page size */
#define T_NAOCRD	10	/* negotiate about CR disposition */
#define T_NAOHTS	11	/* negotiate about horizontal tabstops */
#define T_NAOHTD	12	/* negotiate about horizontal tab disposition */
#define T_NAOFFD	13	/* negotiate about formfeed disposition */
#define T_NAOVTS	14	/* negotiate about vertical tab stops */
#define T_NAOVTD	15	/* negotiate about vertical tab disposition */
#define T_NAOLFD	16	/* negotiate about output LF disposition */
#define T_XASCII	17	/* extended ascic character set */
#define	T_LOGOUT	18	/* force logout */
#define	T_BM		19	/* byte macro */
#define	T_DET		20	/* data entry terminal */
#define	T_SUPDUP	21	/* supdup protocol */
#define T_EXOPL		255	/* extended-options-list */

/* telnet det options */

#define DET_EDIT	1	/* edit facility */
#define DET_ERASE	2	/* erase facility */
#define DET_TRANSMIT	3	/* transmit facility */
#define DET_FORMAT	4	/* format facility */
#define DET_MVCUR	5	/* move cursor */
#define DET_UP		8	/* up cursor */
#define DET_DOWN	9	/* down cursor */
#define DET_LEFT	10	/* left cursor */
#define DET_RIGHT	11	/* right cursor */
#define DET_HOME	12	/* home cursor */
#define DET_LNIN	13	/* line insert */
#define DET_LNDEL	14	/* line delete */
#define DET_CHRIN	15	/* character insert */
#define DET_CHRDEL	16	/* character delete */
#define DET_RDCUR	17	/* read cursor */
#define DET_CURPOS	18	/* cursor position */
#define DET_REVTAB	19	/* reverse tab */
#define DET_XMTSCR	20	/* transmit screen */
#define DET_XUN		21	/* transmit unprotected */
#define DET_XLN		22	/* transmit line */
#define DET_XFLD	23	/* transmit field */
#define DET_XROS	24	/* transmit rest of screen */
#define DET_XROL	25	/* transmit rest of line */
#define DET_XMOD	27	/* transmit modified */
#define DET_DATAXMT	28	/* data transmit */
#define DET_ESCR	29	/* erase screen */
#define DET_ELN		30	/* erase line */
#define DET_EFLD	31	/* erase field */
#define DET_EROS	32	/* erase rest of screen */
#define DET_EUN		35	/* erase unprotected */
#define DET_FMTDATA	36	/* format data */
#define DET_REPEAT	37	/* repeat */
#define DET_SUPPRO	38	/* suppress protection */
#define DET_FLDSEP	39	/* field separator */
#define DET_FN		40
#define DET_ERROR	41	/* error */

#ifdef TELCMDS
char *telcmds[] = {
	"SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
	"EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC",
};
#endif

#ifdef TELOPTS
char *telopts[] = {
	"BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
	"STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
	"NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
	"NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
	"DATA ENTRY TERMINAL", "SUPDUP"
};
#endif

#endif	_TELNET_

/*
 *	@(#)telnet.h	11.2 (NRC)
 */
