#include <stdio.h>
#include <errno.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct bmpheader {

  short bfType;
  int bfSize;
  short  bfReserved1;
  short  bfReserved2;
  int bfOffBits;

  int biSize;
  int  biWidth;
  int  biHeight;
  short  biPlanes;
  short  biBitCount;
  int biCompression;
  int biSizeImage;
  int  biXPelsPerMeter;
  int  biYPelsPerMeter;
  int biClrUsed;
  int biClrImportant;

  int palette[2];
};

#define STRIDE 1024/8


int to_LE16(num)
int num;
{

    return (num>>8) | (num<<8);
}

int to_LE32(val)
int val;
{

    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

int main(argc, argv)
int argc;
char *argv[];
{
struct bmpheader header;
FILE *fp;
FILE *output;
char *fb;
int i,x,y;
int width = 640;
int height = 480;

  output = fopen(argv[1], "w");

  /* write an image header */		
  header.bfType = to_LE16(0x4d42);
  header.bfSize = to_LE32(14+40+2*4+(width/8)*height);
  header. bfReserved1 = 0;
  header. bfReserved2 = 0;
  header.bfOffBits = to_LE32(14 + 40 );

  header.biSize = to_LE32(40);
  header.biWidth = to_LE32(width);
  header.biHeight = to_LE32(-height);	/* flipped */
  header.biPlanes = to_LE16(1);
  header.biBitCount = to_LE16(1);
  header.biCompression = 0;
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0;
  header.biYPelsPerMeter = 0;

  header.biClrUsed = to_LE32(2);
  header.biClrImportant = 0;
  /* must use palette */
  header.palette[0] = 0xffffffff;	/* white bg */
  header.palette[1] = 0x000000ff;	/* black fg */
  fwrite(&header, sizeof(header), 1, output);

  fb = phys(1);
  for (y=0; y<height; y++)
  {
    fwrite(fb, width/8, 1, output);
    fb += STRIDE;
  }
  phys(-1);
  
  fclose(output);

  exit(0);
}
