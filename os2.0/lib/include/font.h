/*
 *  The information in this file is used to construct a 4400-Series
 *  font definition.  A font definition file is a collection of the following
 *  structures, in the order listed:
 *
 *	struct FontHeader	/* basic info about the font
 *	char *comment		/* optional copyright information
 *	struct FontMaps 	/* description of xtable
 *	short *xtable		/* table of x positions in bitmap
 *	struct FontInfo		/* optional detailed information about font
 *	char BitMap[inc,height]	/* BitBlt compatible image representation
 *
 *	The font file must contain FontHeader, FontMaps, and BitMap.  If a
 * 	comment is included, it should immediately follow the FontHeader.
 *
 *	Additional structures for more detailed description of character
 *	widths, offsets, and ligatures may be added in the future.
 */

#define FONT_MAGIC	0x466f6e74	/* spells "Font" in ASCII */

/* AB:  long is not required to be 4 bytes as this assumes; 8 bytes on OSX */
#define long int

struct FontHeader
    {				/* the header of a font file */
    long   magic;		/* number to identify this as a font */
    char   name[32];		/* name of this font family */
    char   face[32];		/* style of this font (ie bold, italic, etc) */

    short  type;		/* indicates ASCII vs Icon vs Greek, etc */
    short  compatibility;	/* indicates consistency with other format */

    short  ptsize;		/* indicates intended image size */
    short  resolution;		/* assumed resolution for ptsize (pixels/inch)*/

    short mul;			/* mul/div provides scaling between */
    short div;			/* "distances" (used below) and screen pixels */

    char *comment;		/* 1) pointer to copyright information */
    struct FontMap *maps;	/* 1) pointer to info about xtable */
    struct FontInfo *info;	/* 1) pointer to misc hints (optional) */
    short *xtable;		/* 1) ptr to table of chr locations in bitmap */
    long spare_ptr[4];		/* spare pointers for possible future use */

    char  *bitmap;		/* 1) 2) ptr to bitmap */
    short  width;		/* width (in pixels) of font bitmap */
    short  height;		/* height (in scan lines) of font bitmap */
    short  offsetw;		/* horizontal offset - normally 0 */
    short  offseth;		/* vertical offset - normally maps->baseline */
    short  inc;			/* bytes per scan line of bitmap (even) */
    short  depth;		/* number of bits per pixel in the bitmap */

    short  rotation;		/* 3) font orientation - degrees cw from norm */
    short  undef_char;		/* char to be printed for any undefined char */

    short  boldinfo;		/* info for making bold, 0 if not bold-able */
    short  italicinfo;		/* info for making italic, 0 if not italic-able */

    long spare[4];		/* Just in case we forgot something */
    };

/* AB:  hack to compile on modern systems */
#undef long

/*
 *  1)  These pointers are set to file relative values within the file
 *	structure, they must be replaced with absolute pointer values when
 *	the font structure is read into memory.
 *  2)  The BitMap is an array of characters, the size of which (in bytes) is
 *	FontHeader.inc * FontHeader.height. 
 *  3)  Currently only 0 is supported.  90, 180 and 270 may be added in the
 *	future.
 */


/* Defines for FontHeader.type - to be added to as needed */
#define TypeUnknown	0
#define TypeASCII	1
#define TypeIcon	2
#define TypeGreek	3
#define TypeKanji	4

/*
 *  Defines for FontHeader.compatibility - to be added to as needed.
 *  These provide "hints" as to conventions of style, ligature use, etc
 */
#define CompUnknown	0
#define CompTekMonoSpaced	1
#define CompTekProportional	2
#define CompXeroxSmalltalk	3
#define CompLocal	32
#define CompMagnolia	33
#define CompMac		34
#define CompAdobe	35
#define CompSymbolics	36
#define CompTEX		37
#define CompBerkeley	38


/*
 *  The struct FontMaps indicates what characters are defined in the font
 *  bitmap.  The xtable array, which immediately follows, indicates where
 *  each character starts within the bitmap.  A "BitBlt" operation
 *  is performed starting at the specified x location, and goes up to,
 *  but not including, the x location of the next entry in the xtable. 
 *  NOTE: This means that there must be 1 extra entry in the xtable
 *  (last - first + 1), where the last entry defines the end of the last
 *  defined character.  If (xtable[foo+1] - xtable[foo] = 0) then ???
 */
struct FontMap
    {
    short white;	/* 1) pixel padding to add between chars */
    short maxw;		/* width (in pixels) of the widest char */
    short line;		/* scan lines per text line */
    short baseline;	/* distance from baseline to top of bitmap */
    short fixed;	/* 2) xtable increment for printable chars */
    short first;	/* code of first character in the xtable array */
    short last;		/* code of last character in the xtable array */
    };
/*
 *  1)  A negative FontMap.white may be used to subtract pixels between chars.
 *  2)  A zero value indicates a proportionally-spaced font.
 */


/*
 *  The FontInfo structure provides more detailed information about the
 *  specified font characters.  All entries may be scaled by mul/div to 
 *  allow values to be specified with higher resolution than the bitmap
 *  (in printer resolution for example).
 */
struct FontInfo  /* optional - used for positioning diacritical marks etc */
    {
    short maxw;			/* width (distance) of the widest char */
    short line;			/* distance per text line */
    short baseline;		/* distance from baseline to top of bitmap */
    short cap_height;		/* distance from baseline to top of "H" */
    short low_height;		/* distance from baseline to top of "x" */
    short ascender_height;	/* distance from baseline to top of "h" */
    short descender_height;	/* distance from baseline to bottom of "g" */
    short underline_posn;	/* distance from baseline to top of underline */
    short underline_weight;	/* thickness (distance) of underline */
    short min_lead;		/* minimum leading distance between lines */
    short max_lead;		/* maximum leading distance between lines */
    };

/* the default font */
#define DefaultFontName "/fonts/PellucidaSans-Serif12.font"
extern struct FontHeader *DefaultFont;

