/* tek4404 executable header */

#pragma pack(push, 1)

/* probably 64 bytes, big endian */
typedef struct
{
  unsigned char magic[2];    /* 0x04, 0x00 */
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

  short flags;    // 0x8000 = no mc68881   0x0040 = core dump, 0x0020 = not clear BSS
  
  int unknown2[3];
  
  int data2;      // eg 0x00020000
  int data3;      // eg 0x00040000
} PH;

#pragma pack(pop)

