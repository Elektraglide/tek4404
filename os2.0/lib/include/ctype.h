#define ctype_h

extern  char    _chcodes[];      /* in chcodes.a */

#define _CONTROL       0x01
#define _UPPER         0x02
#define _LOWER         0x04
#define _DIGIT         0x08
#define _WHITE         0x10
#define _PUNCT         0x20
#define _HEXDIG        0x40

#define isascii(c)      (((unsigned)(c))<=0x007F)
#define isalpha(c)      (_chcodes[c]&(_UPPER|_LOWER))
#define isupper(c)      (_chcodes[c]&_UPPER)
#define islower(c)      (_chcodes[c]&_LOWER)
#define isdigit(c)      (_chcodes[c]&_DIGIT)
#define isxdigit(c)     (_chcodes[c]&_HEXDIG)
#define isspace(c)      (_chcodes[c]&_WHITE)
#define ispunct(c)      (_chcodes[c]&_PUNCT)
#define isalnum(c)      (_chcodes[c]&(_UPPER|_LOWER|_DIGIT))
#define isprint(c)      (_chcodes[c]&(_PUNCT|_UPPER|_LOWER|_DIGIT))
#define isgraph(c)      (_isgraph(c))
#define iscntrl(c)      (_chcodes[c]&_CONTROL)
#define _toupper(c)     ((c)-('a'-'A'))
#define _tolower(c)     ((c)-('A'-'a'))
#define toascii(c)      ((c)&0x007F)
