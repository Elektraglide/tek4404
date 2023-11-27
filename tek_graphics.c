// HACK to avoid missing include
typedef void siginfo_t;

#include <stdio.h>
#include <stdlib.h>
#include <font.h>
#include <graphics.h>

#include <OpenGL/gl.h>
#include <SDL2/SDL.h>

SDL_Window *window;
SDL_Surface *framebuffer;
SDL_Renderer *renderer;
SDL_Rect vport;
SDL_Texture *font8x16;
SDL_Texture *font8x16_normal,*font8x16_invert;
#define FONTWIDTH 8
#define FONTBASELINE 10
#define FONTSPACING 12

struct FORM Screen;
union EVENTUNION eventqueue[32];
int eread,ewrite;
int eventsenable,keyboardenable;
int mbuttons,mousex,mousey;
int pandisc;

#pragma pack(push, 1)

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
#pragma pack(pop)

void dumpBMP(char *filename, int width, int height, unsigned char *data)
{
  struct bmpheader header;
  
  FILE *output = fopen(filename, "w");

  /* write an image header */		
  header.bfType = (0x4d42);
  header.bfSize = (14+40+2*4+(width/8)*height);
  header. bfReserved1 = 0;
  header. bfReserved2 = 0;
  header.bfOffBits = (14 + 40 );

  header.biSize = (40);
  header.biWidth = (width);
  header.biHeight = (-height);	/* flipped */
  header.biPlanes = (1);
  header.biBitCount = (1);
  header.biCompression = 0;
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0;
  header.biYPelsPerMeter = 0;

  header.biClrUsed = (2);
  header.biClrImportant = 0;
  /* must use palette */
  header.palette[0] = 0xffffffff;	/* white bg */
  header.palette[1] = 0xff000000;	/* black fg */
  fwrite(&header, sizeof(header), 1, output);

  fwrite(data, width/8, height, output);
  
  fclose(output);
}

void RenderString(char *line, int n, int ox, int oy)
{
  unsigned char *src, *dst;
  int i,j,k,rows;
  int ch;
  
  static unsigned char *bitmap = NULL;
  static unsigned char charset[14 * 4096];
  if (!bitmap)
  {
    bitmap = (unsigned char *)calloc(1024, 1024/8);
    FILE *fp = fopen("charMagnoliaFixed_7.bmp","r");
    struct bmpheader header;
    fread(&header, sizeof(header),1,fp);
    fseek(fp, header.bfOffBits, SEEK_SET);
    fread(charset, 14, 4096, fp);
    fclose(fp);
    
    // byte-per-pixel to bit-per-pixel
    dst = charset;
    for (i=0; i<4096 * 14; i+=8*4)
    {
        unsigned char eightbit = 0;
        for (j=0; j<8; j++)
        {
          eightbit <<= 1;
          eightbit |= charset[i+(j*4+0)] ? 0 : 1;
        }
        *dst++ = eightbit;
    }
  }

    /* assumes font is fixed 8px and font bitmap stride=1024 */
    src = (unsigned char *)charset;
    dst = (unsigned char *)bitmap + ((oy-14)<<7) + (ox>>3);
    for(k=0; k<n; k++)
    {
       ch = line[k];

      j = 0;
      rows = 14;
      while(rows-- > 0)
      {
       dst[k+j] = src[ch+j];
       j += 128;
      }
    }
    
    dumpBMP("screen.bmp", 1024, 1024, bitmap);
}

// need to call this for window to flip
void SDLrefreshwin()
{
  SDL_UpdateWindowSurface(window);
}

void updatewin(SDL_Rect *r)
{
  SDL_Rect dst;

  // viewport relative
  dst.x = r->x - vport.x;
  dst.y = r->y - vport.y;
  dst.w = r->w;
  dst.h = r->h;
  
  SDL_BlitSurface(framebuffer, r, SDL_GetWindowSurface(window), &dst);
}

short ScrWidth,ScrHeight,ViewWidth,ViewHeight;
struct FORM BlackMask;
struct FORM DarkGrayMask;
struct FORM GrayMask;
struct FORM LightGrayMask;
struct FORM VeryLightGrayMask;
struct FORM WhiteMask;

#define min(A,B)  ((A) < (B) ? (A) : (B))
#define max(A,B)  ((A) > (B) ? (A) : (B))

struct FileFontHeader
{				/* the header of a font file */
    int   magic;		/* number to identify this as a font */
    char   name[32];		/* name of this font family */
    char   face[32];		/* style of this font (ie bold, italic, etc) */

    short  type;		/* indicates ASCII vs Icon vs Greek, etc */
    short  compatibility;	/* indicates consistency with other format */

    short  ptsize;		/* indicates intended image size */
    short  resolution;		/* assumed resolution for ptsize (pixels/inch)*/

    short mul;			/* mul/div provides scaling between */
    short div;			/* "distances" (used below) and screen pixels */

    unsigned int comment;		/* 1) pointer to copyright information */
    unsigned int  maps;	/* 1) pointer to info about xtable */
    unsigned int info;	/* 1) pointer to misc hints (optional) */
    unsigned int xtable;		/* 1) ptr to table of chr locations in bitmap */
    unsigned int spare_ptr[4];		/* spare pointers for possible future use */

    unsigned int bitmap;		/* 1) 2) ptr to bitmap */
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

    int spare[4];		/* Just in case we forgot something */
};

void readtekfont(char *filename)
{
  int i,j,k;
  char dumpname[150];
  
  // read font
  FILE *ff = fopen(filename, "r");
  struct FileFontHeader *fdata;
  fseek(ff, 0, SEEK_END);
  i = (int)ftell(ff);
  fdata = (struct FileFontHeader *)malloc(i);
  fseek(ff, 0, SEEK_SET);
  fread(fdata, i, 1, ff);
  fclose(ff);
  
  // dump font
  sprintf(dumpname, "tekfont_%s_%d.pnm", fdata->name, ntohs(fdata->ptsize));
  FILE *fp = fopen(dumpname, "w");
  fprintf(fp, "P1 %d %d %d\n", ntohs(fdata->width), ntohs(fdata->height), 1);
  fflush(fp);

  int maps = ntohl(fdata->maps);
  int info = ntohl(fdata->info);
  int xtable = ntohl(fdata->xtable);
  int bitmap = ntohl(fdata->bitmap);
  printf("bitmap %d %d at offset:0x%x\n",ntohs(fdata->width), ntohs(fdata->height), bitmap);
  printf("%d ptsize\n",ntohs(fdata->ptsize));

  // how many characters defined
  struct FontMap *fm = (struct FontMap *)((char *)fdata + maps);
  printf("%d maxw\n",ntohs(fm->maxw));
  printf("%d line\n",ntohs(fm->line));
  printf("%d baseline\n",ntohs(fm->baseline));
  printf("%d fixed\n",ntohs(fm->fixed));
  printf("%d chars [%d %d]\n",ntohs(fm->last) - ntohs(fm->first) + 1, ntohs(fm->first), ntohs(fm->last));
  printf("calculated chars %d\n", ntohs(fdata->width) / ntohs(fm->maxw));
  
  char *bits = ((char *)fdata) + bitmap;  // + (ntohs(fdata->width) / ntohs(fm->maxw));
  for (j=0; j<ntohs(fdata->height); j++)
  {
    for (i=0; i<ntohs(fdata->width) / 8; i++)
    {
      unsigned char eightbits = bits[j * ntohs(fdata->inc) + i];
      for(k=0; k<8; k++)
      {
        fprintf(fp, "%d ", (eightbits & 0x80) ? 1 : 0);
        eightbits <<= 1;
      }
      
      if ((i % 8) == 7)
        fprintf(fp, "\n");
    }
  }
  fclose(fp);
 
  free(fdata);
}


void SaveDisplayState(struct DISPSTATE *state)
{
}

void RestoreDisplayState(struct DISPSTATE *state)
{
}

int BbcomDefault(struct BBCOM *bbcom)
{
  memset(bbcom, 0, sizeof(struct BBCOM));
  bbcom->destform = &Screen;
  bbcom->destrect.w = ScrWidth;
  bbcom->destrect.h = ScrHeight;
  bbcom->cliprect.w = ScrWidth;
  bbcom->cliprect.h = ScrHeight;
  bbcom->rule = bbSorD;

  return 0;
}

void ClearScreen()
{

  SDL_SetRenderDrawColor(renderer, 255,255,255,255);
  SDL_RenderClear( renderer);
  updatewin(&vport);
}

int ReleaseCursor()
{

  return 0;
}

int ProtectCursor(struct RECT *rl,struct RECT *r2)
{

  return 0;
}

int CharWidth(char ch, struct FontHeader *font)
{
  return FONTWIDTH;
}

int CharDraw(char ch, struct POINT *loc)
{
  SDL_Rect dst = {loc->x,loc->y - FONTBASELINE,FONTWIDTH,FONTSPACING};
  SDL_Rect src = {0,0,FONTWIDTH,FONTSPACING};

  src.x = ch * FONTWIDTH;
  src.y = 0;
  SDL_RenderCopy(renderer, font8x16, &src, &dst);

  // dirty area
  updatewin(&dst);

  return 0;
}

int CharDrawX(char ch, struct POINT *loc, struct BBCOM *bbcom, struct FontHeader *font)
{
  SDL_Rect dst = {loc->x,loc->y - FONTBASELINE,FONTWIDTH,FONTSPACING};
  SDL_Rect src = {0,0,FONTWIDTH,FONTSPACING};
  SDL_Rect cr;
  
  cr.x = bbcom->cliprect.x;
  cr.y = bbcom->cliprect.y;
  cr.w = bbcom->cliprect.w;
  cr.h = bbcom->cliprect.h;
  SDL_RenderSetClipRect(renderer, &cr);

  // any !source needs to use inverted fontmap
  font8x16 = (bbcom->rule == bbnS || bbcom->rule == bbSnorD) ? font8x16_invert : font8x16_normal;

  // any blending?
  SDL_SetTextureBlendMode(font8x16, (bbcom->rule == bbSorD) ? SDL_BLENDMODE_MOD : SDL_BLENDMODE_NONE);
  
  src.x = ch * FONTWIDTH;
  src.y = 0;
  SDL_RenderCopy(renderer, font8x16, &src, &dst);

  // dirty area
  updatewin(&dst);

  return 0;
}


int StringWidth(char *string,struct FontHeader *font)
{
  int w = 0;
  char ch;
  
  while ((ch = *string++) != '\0')
  {
    w += CharWidth(ch, font);
  }
  
  return w;
}

int StringDraw(char *ch, struct POINT *loc)
{
  char c;
  SDL_Rect dst = {loc->x,loc->y - FONTBASELINE,FONTWIDTH,FONTSPACING};
  SDL_Rect src = {0,0,FONTWIDTH,FONTSPACING};
  
  while((c = *ch++) != '\0')
  {
    src.x = c * FONTWIDTH;
    src.y = 0;
    SDL_RenderCopy(renderer, font8x16, &src, &dst);
    dst.x += 8;
  }
  
  // dirty area
  dst.w = dst.x - loc->x;
  dst.h = dst.y - loc->y + FONTSPACING;
  dst.x = loc->x;
  dst.y = loc->y - FONTBASELINE;

  updatewin(&dst);
  
  return 0;
}

int StringDrawX(char *ch, struct POINT *loc, struct BBCOM *bbcom, struct FontHeader *font)
{
  char c;
  SDL_Rect dst = {loc->x,loc->y - FONTBASELINE,FONTWIDTH,FONTSPACING};   // we use topleft not bottomleft
  SDL_Rect src = {0,0,FONTWIDTH,FONTSPACING};
  SDL_Rect cr;
  
  cr.x = bbcom->cliprect.x;
  cr.y = bbcom->cliprect.y;
  cr.w = bbcom->cliprect.w;
  cr.h = bbcom->cliprect.h;
  SDL_RenderSetClipRect(renderer, &cr);

  // any !source needs to use inverted fontmap
  font8x16 = (bbcom->rule == bbnS || bbcom->rule == bbSnorD) ? font8x16_invert : font8x16_normal;

  // any blending?
  SDL_SetTextureBlendMode(font8x16, (bbcom->rule == bbSorD) ? SDL_BLENDMODE_MOD : SDL_BLENDMODE_NONE);


  SDL_BlendMode bm = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR,SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDOPERATION_MINIMUM,
  SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD);
  SDL_SetTextureBlendMode(font8x16, bm);
  

  while((c = *ch++) != '\0')
  {
    src.x = c * FONTWIDTH;
    src.y = 0;
    SDL_RenderCopy(renderer, font8x16, &src, &dst);
    dst.x += FONTWIDTH;
  }

  // dirty area
  dst.w = dst.x - loc->x;
  dst.h = FONTSPACING;
  dst.x = loc->x;
  dst.y = loc->y - FONTBASELINE;

  updatewin(&dst);
  
  return 0;
}

int BitBlt(struct BBCOM *bbcom)
{
  // just does 
  SDL_Rect dst,src,cr;

  dst.x = bbcom->destrect.x;
  dst.y = bbcom->destrect.y;
  dst.w = bbcom->destrect.w;
  dst.h = bbcom->destrect.h;

  src.x = bbcom->srcpoint.x;
  src.y = bbcom->srcpoint.y;
  src.w = bbcom->destrect.w;
  src.h = bbcom->destrect.h;

  cr.x = bbcom->cliprect.x;
  cr.y = bbcom->cliprect.y;
  cr.w = bbcom->cliprect.w;
  cr.h = bbcom->cliprect.h;
  SDL_RenderSetClipRect(renderer, &cr);
//SDL_SetSurfaceColorMod( framebuffer, 255,0,0);

  SDL_LowerBlit(framebuffer, &src, framebuffer, &dst);

SDL_SetSurfaceColorMod( framebuffer, 255,255,255);

  updatewin(&dst);

}

int SetKBCode(int val)
{
  keyboardenable = (val == 0);
  return 0;
}

int EGetCount()
{
  return (ewrite - eread + 32) % 32;
}

int specialkey(SDL_Scancode key, int updown)
{
  switch(key)
  {
  case SDL_SCANCODE_UP:
    if(updown) pandisc &= ~1; else pandisc |= 1;
	  break;
  case SDL_SCANCODE_DOWN:
    if(updown) pandisc &= ~2; else pandisc |= 2;
	  break;
  case SDL_SCANCODE_LEFT:
    if(updown) pandisc &= ~4; else pandisc |= 4;
	  break;
  case SDL_SCANCODE_RIGHT:
    if(updown) pandisc &= ~8; else pandisc |= 8;
	  break;
  default:
    return keyboardenable;
    break;
  }
  
  return 0;
}

unsigned long EGetNext()
{
  unsigned long rc;
  
  /* process pandisc control */
  if (pandisc & 1)
    if (vport.y >= 4) vport.y -= 16;
  if (pandisc & 2)
    if (vport.y < ScrHeight-ViewHeight) vport.y += 16;
  if (pandisc & 4)
    if (vport.x >= 4) vport.x -= 16;
  if (pandisc & 8)
    if (vport.x < ScrWidth-ViewWidth) vport.x += 16;
  if (pandisc)
    updatewin(&vport);

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (eventsenable)
	  switch (event.type)
	  {
  	  case SDL_WINDOWEVENT:
  	  break;
  	  case SDL_QUIT:
        exit(0);
	  	  break;

#if 0
      case SDL_TEXTINPUT:
        if (keyboardenable)
        {
          eventqueue[ewrite].estruct.etype = E_PRESS;
          eventqueue[ewrite].estruct.eparam = event.text.text[0];
          ewrite = (ewrite + 1) & 31;
        }
	  	  break;
#endif

  	  case SDL_KEYDOWN:
        if (specialkey(event.key.keysym.scancode, 0))
        {
          if (event.key.keysym.scancode == SDL_SCANCODE_RETURN)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = '\r';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = 0x08;
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_TAB)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = '\t';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = 0x1b;
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = ' ';
            ewrite = (ewrite + 1) & 31;
          }
          else

          if (event.key.keysym.scancode == SDL_SCANCODE_COMMA)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '<' : ',';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_PERIOD)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '>' : '.';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_SLASH)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '?' : '/';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_SEMICOLON)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? ':' : ';';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_APOSTROPHE)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '\"' : '\'';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '{' : '[';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '}' : ']';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_BACKSLASH)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '|' : '\\';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_MINUS)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '_' : '-';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_EQUALS)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '+' : '=';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_NONUSBACKSLASH)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = (event.key.keysym.mod & KMOD_SHIFT) ? '~' : '\`';
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode == SDL_SCANCODE_0)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = '0';
            if (event.key.keysym.mod & KMOD_SHIFT)
            {
              eventqueue[ewrite].estruct.eparam = ')';
            }
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode >= SDL_SCANCODE_1 && event.key.keysym.scancode <= SDL_SCANCODE_9)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = '1' + event.key.keysym.scancode - SDL_SCANCODE_1;
            if (event.key.keysym.mod & KMOD_SHIFT)
            {
              static char shifted[] = "!@#$%^&*(";
              int n = event.key.keysym.scancode - SDL_SCANCODE_1;
              eventqueue[ewrite].estruct.eparam = shifted[n];
            }
            ewrite = (ewrite + 1) & 31;
          }
          else
          if (event.key.keysym.scancode >= SDL_SCANCODE_A && event.key.keysym.scancode <= SDL_SCANCODE_Z)
          {
            eventqueue[ewrite].estruct.etype = E_PRESS;
            eventqueue[ewrite].estruct.eparam = 'a' + event.key.keysym.scancode - SDL_SCANCODE_A;
            if ((event.key.keysym.mod & KMOD_CAPS) || (event.key.keysym.mod & KMOD_SHIFT))
            {
              eventqueue[ewrite].estruct.eparam -= 32;
            }
            if (event.key.keysym.mod & KMOD_CTRL)
            {
              eventqueue[ewrite].estruct.eparam -= 'a'-1;
            }
            ewrite = (ewrite + 1) & 31;
          }
        }
	  	  break;

  	  case SDL_KEYUP:
        if (specialkey(event.key.keysym.scancode, 1))
        {

        }
	  	  break;

  	  case SDL_MOUSEMOTION:
        // create a single dummy event
        mousex = event.motion.x;
        mousey = event.motion.y;
        break;
        
  	  case SDL_MOUSEBUTTONDOWN:
  	  {
        int tek4404buttoncode;
        if (event.button.button == SDL_BUTTON_LEFT)
          tek4404buttoncode = 4;
        if (event.button.button == SDL_BUTTON_MIDDLE)
          tek4404buttoncode = 2;
        if (event.button.button == SDL_BUTTON_RIGHT)
          tek4404buttoncode = 1;
        mbuttons |= tek4404buttoncode;

        // what code are we meant to return for this?
        //eventqueue[ewrite].estruct.etype = E_PRESS;
        //eventqueue[ewrite].estruct.eparam = tek4404buttoncode;
        //ewrite = (ewrite + 1) & 31;

	  	  break;
  	  }
  	  case SDL_MOUSEBUTTONUP:
  	  {
        int tek4404buttoncode;
        if (event.button.button == SDL_BUTTON_LEFT)
          tek4404buttoncode = 4;
        if (event.button.button == SDL_BUTTON_MIDDLE)
          tek4404buttoncode = 2;
        if (event.button.button == SDL_BUTTON_RIGHT)
          tek4404buttoncode = 1;
        mbuttons &= ~tek4404buttoncode;
        
        eventqueue[ewrite].estruct.etype = E_RELEASE;
        eventqueue[ewrite].estruct.eparam = tek4404buttoncode;
        ewrite = (ewrite + 1) & 31;
	  	  break;
  	  }
  	  case SDL_MOUSEWHEEL:
	  	  break;

  	  default:
	  	  break;
	  }
  }

  rc = 0;
  if (EGetCount() > 0)
  {
    rc = eventqueue[eread].evalue;
    eread = (eread + 1) & 31;
  }
  return rc;
}

unsigned long EGetTime()
{
  
  return SDL_GetTicks();
}

int ESetSignal()
{
  fprintf(stderr,"ESetSignal: unimplemented\n");
  return 0;
}

int ESetAlarm(unsigned long time)
{
  fprintf(stderr,"ESetAlarm: unimplemented\n");
  return 0;
}

int EventDisable()
{
  eventsenable = 0;
  return 0;
}

int EventEnable()
{
  eventsenable = 1;
  return 0;
}

int FontClose(struct FontHeader *font)
{

  return 0;
}

struct FontHeader * FontOpen(char *filename)
{
  static struct FontMap map;
  static struct FontHeader header;

  map.fixed = FONTWIDTH;
  map.maxw = FONTWIDTH;
  map.line = FONTSPACING;
  map.baseline = FONTBASELINE;
  header.maps = &map;

  strcpy(header.name, "built-in");
  header.ptsize = FONTSPACING;
  header.resolution = FONTBASELINE;
  header.bitmap = Screen.addr;
  return &header;
}

static void refreshmousestate()
{
  /* refresh mouse state */
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
      if (event.type == SDL_MOUSEMOTION)
      {
        mousex = event.motion.x;
        mousey = event.motion.y;
      }
      else
      if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        int tek4404buttoncode;
        if (event.button.button == SDL_BUTTON_LEFT)
          tek4404buttoncode = 4;
        if (event.button.button == SDL_BUTTON_MIDDLE)
          tek4404buttoncode = 2;
        if (event.button.button == SDL_BUTTON_RIGHT)
          tek4404buttoncode = 1;
        mbuttons |= tek4404buttoncode;
      }
      else
      if (event.type == SDL_MOUSEBUTTONUP)
      {
        int tek4404buttoncode;
        if (event.button.button == SDL_BUTTON_LEFT)
          tek4404buttoncode = 4;
        if (event.button.button == SDL_BUTTON_MIDDLE)
          tek4404buttoncode = 2;
        if (event.button.button == SDL_BUTTON_RIGHT)
          tek4404buttoncode = 1;
        mbuttons &= ~tek4404buttoncode;        
      }
  }
}


int GetButtons()
{
  refreshmousestate();
  return mbuttons;
}

int GetMBounds(struct POINT *ulpoint,struct POINT *lrpoint)
{
  ulpoint->x = vport.x;
  ulpoint->y = vport.y;
  lrpoint->x = vport.x + vport.w;
  lrpoint->y = vport.y + vport.h;

  return 0;
}

int GetMPosition(struct POINT *point)
{
  refreshmousestate();
  point->x = mousex + vport.x;
  point->y = mousey + vport.y;
  return 0;
}

int GetRealMachineType()
{

  return 0x44040000;
}

int GetViewport(struct POINT *point)
{
  point->x = vport.x;
  point->y = vport.y;
  return 0;
}

int SetViewport(struct POINT *point)
{
  vport.x = point->x;
  vport.y = point->y;
  return 0;
}

int PointDistance(struct POINT *point1, struct POINT *point2)
{

  return 0;
}

int PointFromUser(struct POINT *point)
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
      if (event.type == SDL_MOUSEMOTION)
      {
        mousex = event.motion.x;
        mousey = event.motion.y;
      }
      else
      if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        GetMPosition(point);
        return 0;
      }
      else
      if (event.type == SDL_MOUSEBUTTONUP)
      {
        GetMPosition(point);
        return 0;
      }
  }
  
  return 0;
}

int PointMax(struct POINT *pointl,struct POINT *point2,struct POINT *point3)
{
  point3->x = max(pointl->x, point2->x);
  point3->y = max(pointl->y, point2->y);
  return 0;
}

int PointMidpoint(struct POINT *point1,struct POINT *point2,struct POINT *point3)
{
  point3->x = (point1->x + point2->x) >> 1;
  point3->y = (point1->y + point2->y) >> 1;
  return 0;
}

int PointMin(struct POINT *pointl,struct POINT *point2,struct POINT *point3)
{
  point3->x = min(pointl->x, point2->x);
  point3->y = min(pointl->y, point2->y);
  return 0;
}

int PointsToRect(struct POINT *point1,struct POINT *point2, struct RECT *rect)
{
  PointMin(point1, point2, (struct POINT *)&rect->x);
  PointMax(point1, point2, (struct POINT *)&rect->w);
  rect->w -= rect->x;
  rect->h -= rect->y;
  return 0;
}

void PointToRC(int *row,int *col, struct POINT *point)
{
  *col = point->x / FONTWIDTH;
  *row = point->y / FONTSPACING; // NB
}

void RCToRect(struct RECT *rect, int row, int col)
{
  rect->x = col * FONTWIDTH;
  rect->y = row * FONTSPACING;  // NB
  rect->w = FONTWIDTH;
  rect->h = FONTSPACING;
}

static void halftone2color(struct FORM *halftoneform)
{
  SDL_SetRenderDrawColor(renderer, 255,255,255,255);
  if (halftoneform == &BlackMask)
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
  if (halftoneform == &DarkGrayMask)
    SDL_SetRenderDrawColor(renderer, 64,64,64,255);
  if (halftoneform == &GrayMask)
    SDL_SetRenderDrawColor(renderer, 128,128,128,255);
  if (halftoneform == &LightGrayMask)
    SDL_SetRenderDrawColor(renderer, 192,192,192,255);
  if (halftoneform == &VeryLightGrayMask)
    SDL_SetRenderDrawColor(renderer, 220,220,220,255);
  if (halftoneform == &WhiteMask)
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
}

int RectBoxDraw(struct RECT *rect, int width)
{
  SDL_Rect r;
  
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;
  
  SDL_SetRenderDrawColor(renderer, 255,255,255,255);
  for (int i=0; i<width; i++)
  {
    SDL_RenderDrawRect(renderer,&r);
    r.x -= 1;
    r.y -= 1;
    r.w += 2;
    r.h += 2;
  }

  updatewin(&r);

  return 0;
}

int RectBoxDrawX(struct RECT *rect, int width, struct BBCOM *bbcom)
{
  SDL_Rect r,cr;
  
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;
  
  cr.x = bbcom->cliprect.x;
  cr.y = bbcom->cliprect.y;
  cr.w = bbcom->cliprect.w;
  cr.h = bbcom->cliprect.h;
  SDL_RenderSetClipRect(renderer, &cr);

  halftone2color(bbcom->halftoneform);
  for (int i=0; i<width; i++)
  {
    SDL_RenderDrawRect(renderer,&r);
    r.x -= 1;
    r.y -= 1;
    r.w += 2;
    r.h += 2;
  }
  updatewin(&r);

  return 0;
}

int RectContainsPoint(struct RECT *rect, struct POINT *point)
{
  // NB docs are wrong
  if (point->x > rect->x && point->x < rect->x + rect->w &&
      point->y > rect->y && point->y < rect->y + rect->h)
    return 0;
    
  return 1;
}
    
int RectContainsRect(struct RECT *rectl, struct RECT *rect2)
{

  return 0;
}

int RectDraw(struct RECT *rect)
{
  SDL_Rect r;
  
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;
  
  SDL_RenderSetClipRect(renderer, NULL);

  SDL_SetRenderDrawColor(renderer, 255,255,255,255);
  SDL_RenderFillRect(renderer, &r);
  updatewin(&r);

  return 0;
}

int RectDrawX(struct RECT *rect, struct BBCOM *bbcom)
{
  SDL_Rect r,cr;
  
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;

  cr.x = bbcom->cliprect.x;
  cr.y = bbcom->cliprect.y;
  cr.w = bbcom->cliprect.w;
  cr.h = bbcom->cliprect.h;
  SDL_RenderSetClipRect(renderer, &cr);

  halftone2color(bbcom->halftoneform);
  SDL_RenderFillRect(renderer, &r);
  updatewin(&r);

  return 0;
}

int RectDebug(struct RECT *rect, int r1, int g, int b)
{
  SDL_Rect r;
  
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;

  SDL_RenderSetClipRect(renderer, NULL);
  SDL_SetRenderDrawColor(renderer, r1,g,b,255);
  SDL_RenderFillRect(renderer, &r);
  updatewin(&r);
}

int RectFromUser(struct RECT *rect)
{

  return 0;
}

int RectFromUserX(struct POINT *minsize, struct FORM *mask, struct RECT *rect)
{

  return 0;
}

int RectAreasDiffering(struct RECT *rect1,struct RECT *rect2, struct QUADRECT *quadrect)
{
  quadrect->next = 0;

  return 0;
}

int RectAreasOutside(struct RECT *rect1,struct RECT *rect2, struct QUADRECT *quadrect)
{
  int i = 0;
  struct RECT r1 = *rect1;
  
  if (r1.x < rect2->x)
  {
    quadrect->region[i].x = r1.x;
    quadrect->region[i].y = r1.y;
    quadrect->region[i].w = min(r1.w, rect2->x - r1.x);
    quadrect->region[i].h = r1.h;
    i++;
    
    r1.w -= rect2->x - r1.x;
    r1.x = rect2->x;
  }

  if (r1.y < rect2->y)
  {
    quadrect->region[i].x = r1.x;
    quadrect->region[i].y = r1.y;
    quadrect->region[i].w = r1.w;
    quadrect->region[i].h = min(r1.h, rect2->y - r1.y);
    i++;
    
    r1.h -= rect2->y - r1.y;
    r1.y = rect2->y;
  }

  if (r1.x+r1.w > rect2->x+rect2->w)
  {
    quadrect->region[i].x = max(r1.x, rect2->x+rect2->w);
    quadrect->region[i].y = r1.y;
    quadrect->region[i].w = (r1.x+r1.w) - quadrect->region[i].x;
    quadrect->region[i].h = r1.h;
    i++;

    r1.w -= (r1.x+r1.w) - (rect2->x+rect2->w);
  }

  if (r1.y+r1.h > rect2->y+rect2->h)
  {
    quadrect->region[i].x = r1.x;
    quadrect->region[i].y = max(r1.y , rect2->y + rect2->h);
    quadrect->region[i].w = r1.w;
    quadrect->region[i].h = (r1.y+r1.h) - quadrect->region[i].y;
    i++;

    r1.h -= (r1.y+r1.y) - (rect2->y+rect2->h);
  }

  quadrect->next = i;
  return 0;
}

int RectIntersect(struct RECT *rect1,struct RECT *rect2,struct RECT *rect3)
{
  rect3->x = max(rect1->x, rect2->x);
  rect3->y = max(rect1->y, rect2->y);
  rect3->w = min(rect1->x+rect1->w, rect2->x+rect2->w) - rect3->x;
  rect3->h = min(rect1->y+rect1->h, rect2->y+rect2->h) - rect3->y;
  
  return 0;
}

int RectIntersects(struct RECT *rect1, struct RECT *rect2)
{
  if (rect1->x < rect2->x+rect2->w &&
      rect2->x < rect1->x+rect1->w &&
      rect1->y < rect2->y+rect2->h &&
      rect2->y < rect1->y+rect1->h)
    return 1;

  return 0;
}

int RectMerge(struct RECT *rect1,struct RECT *rect2,struct RECT *rect3)
{
  struct POINT lr;
  
  rect3->x = min(rect1->x, rect2->x);
  rect3->y = min(rect1->y, rect2->y);
  lr.x = max(rect1->x + rect1->w, rect2->x + rect2->w );
  lr.y = max(rect1->y + rect1->h, rect2->y + rect2->h );
  rect3->w = lr.x - rect3->x;
  rect3->h = lr.y - rect3->y;
  
  return 0;
}

void ExitGraphics()
{

}

struct FORM *InitGraphics(int mode)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
	  printf("Couldn't initialize SDL: %s\n", SDL_GetError());
	  exit(1);
  }

  ScrWidth = 1024;
  ScrHeight = 1024;
  ViewWidth = 640;
  ViewHeight = 480;

  window = SDL_CreateWindow("Tek4404", 10, 10, ViewWidth, ViewHeight, SDL_WINDOW_SHOWN);
  if (!window)
  {
	  printf("Failed to open %d x %d window: %s\n", ViewWidth, ViewHeight, SDL_GetError());
	  exit(2);
  }
 
  /* CPU pixel buffer */
  framebuffer = SDL_CreateRGBSurfaceWithFormat(0, ScrWidth, ScrHeight, 32, SDL_PIXELFORMAT_BGR888);

  renderer = SDL_CreateSoftwareRenderer(framebuffer);
//  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );

  vport.x = 0;
  vport.y = 0;
  vport.w = ViewWidth;
  vport.h = ViewHeight;
  
  SDL_ShowWindow(window);
  
  // events
  eread = 0;
  ewrite = 0;
  keyboardenable = 0;
  
  Screen.addr = framebuffer->pixels;
  Screen.w = ScrWidth;
  Screen.h = ScrHeight;
  Screen.offsetw = 0;
  Screen.offseth = 0;
  Screen.inc = ScrWidth / 8;
  
  // fonts 8x16
  
  //readtekfont("/Users/adambillyard/projects/tek4404/development/mirror2.0_net2.1/fonts/MagnoliaFixed7.font");

  SDL_Surface *bitmap = SDL_LoadBMP("charMagnoliaFixed_7.bmp");
  font8x16_normal =  SDL_CreateTextureFromSurface(renderer, bitmap);
  SDL_SetTextureColorMod(font8x16_normal, 255,255,255);
  SDL_SetTextureBlendMode(font8x16_normal, SDL_BLENDMODE_NONE);
  SDL_FreeSurface( bitmap );

  bitmap = SDL_LoadBMP("charMagnoliaFixed_7_invert.bmp");
  font8x16_invert =  SDL_CreateTextureFromSurface(renderer, bitmap);
  SDL_SetTextureColorMod(font8x16_invert, 255,255,255);
  SDL_SetTextureBlendMode(font8x16_invert, SDL_BLENDMODE_NONE);
  SDL_FreeSurface( bitmap );

  font8x16 = font8x16_normal;

  SDL_UpdateWindowSurface(window);
  
  if (mode)
  {
    // clear screen etc
    ClearScreen();
  }

  return &Screen;
}
