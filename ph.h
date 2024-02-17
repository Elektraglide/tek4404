/* tek4404 executable header */

#ifdef __clang__
#pragma pack(push, 1)
#endif
/* probably 64 bytes, big endian */
typedef struct
{
  unsigned char magic[2];    /* exe: 0x04, 0x00,  relocatable:  0x05, 0x00 */
  int textsize;
  int datasize;
  int bsssize;

  int relocsize;
  int xferaddress;
  int textstart;
  int datastart;
  char minpage;
  char maxpage;
  char stacksize;
  char initialstack;
  int symbolsize;

  short commentsize;
  short rcssize;

  short flags;    /* 0x8000 = no mc68881   0x0040 = core dump, 0x0020 = not clear BSS */
  
  int unknown2[3];
  
  int data2;      /* eg 0x00020000 */
  int data3;      /* eg 0x00040000 */
} PH;

typedef struct {

  short kind;
  int offset;
  short segment;

  short len;
  
} symbolheader;

#define SEGABS 8
#define SEGTEXT 9
#define SEGDATA 10
#define SEGBSS 11

typedef struct {

	int offset;
	short kind;
	short len;
	
} relocheader;

#ifdef __clang__
#pragma pack(pop)
#endif
