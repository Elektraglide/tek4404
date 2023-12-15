/*  Copyright (C) 1/28/86 by Network Research Corporation
 *  All rights reserved.  No part of this software may be disclosed or distributed in any form or by any means without the prior written consent of Network Research Corporation.
 */
/*  Module: h       File: /nrc/rel/h/s.menu.h  */
/*  Version 11.1 as of 86/01/28 at 09:33:54  */
#ifndef	_MENU_
#define	_MENU_

typedef	struct menu_item {
	char			* field;
	char			* title;
	int			len;
	short			flags;
	char			* defalt;
	char			* value;
	struct menu_item	* help;
} menu_item;

#define	M_REQ		1	/* value required 			*/
#define	M_NUM		2	/* numerics only			*/
#define M_NOSKIP	4	/* no line skip after			*/
#define M_DSPACE	8	/* extra line skip after		*/
#define	M_NOCHANGE	16	/* display current value but don't	*/
				/* accept input				*/
#define M_NOSHOW	32	/* leave current value and don't prompt */
#define M_ALLOC		64	/* the value field has been allocated	*/

#define m_note(text)	{"_note_",text,0,0,nil,nil,(menu_item *)0}
#define m_notens(text)	{"_note_",text,0,M_NOSKIP,nil,nil,(menu_item *)0}
#define m_noteds(text)	{"_note_",text,0,M_DSPACE,nil,nil,(menu_item *)0}

import	void	menu_init(), menu_exit();

/*
 *	@(#)menu.h	11.1 (NRC)
 */

#endif	_MENU_
