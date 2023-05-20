/* tek4404 executable header */

/* probably 64 bytes */
typedef struct
{
  short magic;    /* 0x0400 */
  int textsize;
  int datasize;
  int bsssize;

  int relocsize;
  int xferaddress;
  int textstart;
  int datastart;
  short minpage;
  short maxpage;
  short stacksize;  
  short symbolsize;
  short commentsize;
  
  int rcssize;
  int namesize;
 
} PH;
