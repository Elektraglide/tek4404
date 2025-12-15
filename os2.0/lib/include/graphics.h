/*
 * graphics.h -- definitions for Tektronix 4400-Series graphics library
 */

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* structures needed for bitblt and other routines */
struct POINT {
    short x, y;		/* x and y coordinates */
};
struct RECT {
    short x, y;		/* coordinates of upper left corner */
    short w, h;		/* width and height */
};
struct QUADRECT {	/* structure used for returning 0 to 4 RECTs */
    short next;		/* value of 0 to 4, inclusive, is number of RECTs */
    struct RECT region[4];	
};
struct FORM {
    short *addr;		/* memory address of first bit of bitmap */
    short w, h;			/* width and height of form */
    short offsetw, offseth;	/* used for cursors to mark "hotspot" */
    short inc;		/* byte increment from one line to next, must be even */
};
struct BBCOM {
    struct FORM  *srcform;	/* defines source form, NULL if not needed */
    struct FORM  *destform;	/* defines dest form */
    struct POINT srcpoint;	/* source coord, 0,0 is top left */
    struct RECT  destrect;	/* rectangle for use in dest bitmap */
    struct RECT  cliprect;	/* clipping rectangle */
    struct FORM  *halftoneform;	/* form for halftoning, NULL if not needed */
    short  rule;		/* combination rule (defined below) */
};

/* Bitblt combination rules */
#define bbZeros 	0	/* = zeros */
#define bbZero  	0  	/* = zeros */
#define bbSandD 	1  	/* = source and dest */
#define bbSandDn	2  	/* = source and dest' */
#define bbS		3 	/* = source */
#define bbSnandD	4	/* = source' and dest */
#define bbD		5  	/* = dest */
#define bbSxorD		6 	/* = source xor dest */
#define bbSorD		7  	/* = source or dest */
#define bbnSorD		8  	/* = (source or dest)' */
#define bbnSxorD	9  	/* = (source xor dest)' */
#define bbDn		10 	/* = dest' */
#define bbnD		10 	/* = dest' */
#define bbSorDn		11 	/* = source or dest' */
#define bbSn		12 	/* = source' */
#define bbnS		12 	/* = source' */
#define bbSnorD		13 	/* = source' or dest */
#define bbnSandD	14 	/* = (source and dest)' */
#define bbOnes		15 	/* = ones */

/* defines to help with mouse buttons */
#define M_LEFT	 4
#define M_MIDDLE 2
#define M_RIGHT	 1
#define M_ANY	 7

/* structures and defines for menus */
#define MENU_LINE	0x0001
#define MENU_NOSELECT	0x0002
#define MENU_LEFT	0x0004
#define MENU_RIGHT	0x0008
struct Mbox {
    short y, h;		/* vertical location and height of one menu item */
    short set;		/* if FALSE then item is not selectable */
};
struct MENU {
    struct BBCOM menubb;	/* includes pointers to necessary forms */
    short count,previtem;	/* number of items and last item selected */
    struct Mbox *box;		/* array of item information */
};

/* structure and bit definitions for save/restore state */
struct DISPSTATE {
    long statebits;		/* bits defined below */
    struct POINT viewp;		/* upper left corner of viewport */
    struct POINT ulmouseb;	/* upper left corner of mouse bounds */
    struct POINT lrmouseb;	/* lower right corner of mouse bounds */
    short curarray[16];		/* the bits for the cursor */
    char  keycode;		/* kb encoding (0=event,1=ansi) */
    char  b1_reserved;		/* reserved for future use */
    short lineincr;		/* byte increment between lines of screen */
    short dispwidth;		/* width of virtual display bitmap */
    short dispheight;		/* height of virtual display bitmap */
    short viewwidth;		/* width of visible viewport */
    short viewheight;		/* height of visible viewport */
    short cursorxoffset;	/* x offset of cursor hotpoint */
    short cursoryoffset;	/* y offset of cursor hotpoint */
    long  l1_reserved[2];	/* reserved for future use */
};

#define DS_DISPON	0x0001	/* 1=display enabled, 0=disabled */
#define DS_SCRSAVE	0x0002	/* 1=screen saver enabled, 0=disabled */
#define DS_VIDEO	0x0004	/* 1=video normal, 0=video inverse */
#define DS_TERMEM	0x0008	/* 1=terminal emulator enabled, 0=disabled */
#define DB_CAPSLOCK	0x0010	/* 1=caps lock led on, 0=off */
/* 0x0020 -- reserved */
/* 0x0040 -- reserved */
/* 0x0080 -- reserved */
#define DS_CURSOR	0x0100	/* 1=cursor enabled, 0=disabled */
#define DS_TRACK	0x0200	/* 1=cursor tracks mouse, 0=no tracking */
#define DS_PANCUR	0x0400	/* 1=cursor panning enabled, 0=disabled */
#define DS_PANDISK	0x0800	/* 1=joydisk panning enabled, 0=disabled */
/* 0x1000 -- reserved */
/* 0x2000 -- reserved */
/* 0x4000 -- reserved */
/* 0x8000 -- reserved */
#define DS_KBEVENTS	0x10000	/* 1=keyboard generates event codes, 0=not */
/* 0x20000 through 0x80000000 are reserved */

/* Event value structure and type definitions */
union EVENTUNION {	/* events are either a struct or a specific value */
#ifdef __GNUC__
    struct {
    unsigned int dummy : 16;	/* AB gcc lays out fields left to right.. */
	unsigned int etype : 4;		/* type code, defined below */
	unsigned int eparam : 12;	/* parameter associated with type */
    } estruct;
#else
    struct {
	unsigned int eparam : 12;	/* parameter associated with type */
	unsigned int etype : 4;		/* type code, defined below */
    } estruct;
#endif
    unsigned long evalue;		/* some types have parameters with */
};					/* more than 12 bits, so they use */
					/* the next event codes, too. */
    /* etype definitions */
#define E_DELTATIME	0
#define E_XMOUSE	1
#define E_YMOUSE	2
#define E_PRESS		3
#define E_RELEASE	4
#define E_ABSTIME	5

/* and forward declarations which might be necessary */
struct FORM *InitGraphics();
struct FORM *FormCreate();
struct FORM *FormRead();
struct FORM *FormFromUser();
struct MENU *MenuCreate();
struct MENU *MenuCreateX();
struct MENU *IconMenuCreate();
struct MENU *IconMenuCreateX();
unsigned long EGetNext();
unsigned long EGetTime();

/* the standard cursor forms */
extern struct FORM OriginCursor;
extern struct FORM CornerCursor;
extern struct FORM WaitCursor;
extern struct FORM NormalCursor;
extern struct FORM CrosshairCursor;

/* the standard halftone mask forms */
extern struct FORM BlackMask;
extern struct FORM DarkGrayMask;
extern struct FORM GrayMask;
extern struct FORM LightGrayMask;
extern struct FORM VeryLightGrayMask;
extern struct FORM WhiteMask;

/* the graphics error codes */
#define GERROR	512		/* the generic graphics error, start of list */
#define	GEBCUR	GERROR+1	/* invalid cursor form */
#define GENULL	GERROR+2	/* invalid (NULL) structure pointer */
#define GEINIT	GERROR+3	/* graphics not initialized */
#define GESCRN	GERROR+4	/* specified coords not on virtual screen */
#define GEPARAM	GERROR+5	/* invalid parameter in structure */
#define GEFILE	GERROR+6	/* invalid file type or file i/o error */
#define GEALLOC	GERROR+7	/* memory allocation failure */

/* and finally some externs into display size storage */
extern short ScrWidth;		/* width of the physical bitmap */
extern short ScrHeight;		/* height of the physical bitmap */
extern short ViewWidth;		/* visible viewport width */
extern short ViewHeight;	/* visible viewport height */
