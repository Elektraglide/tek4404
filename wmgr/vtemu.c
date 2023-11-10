/*
  vtemu.c
  winprocmgr

  Created by Adam Billyard on 05/08/2023.
 TODO: 4-bit attrib[]
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vtemu.h"

#define UNICR  0x0d
#define UNILF  0x0a

extern int write();

extern void movedisplaylines();
extern void cleardisplaylines();

/* clear region of both character and attribute buffer */
int clearregion(vt, off, len)
VTemu *vt;
int off;
int len;
{
  if (off+len <= MAXTERMCOLS*MAXTERMROWS)
  {
    memset(vt->buffer+off, 0, len);
    memset(vt->attrib+off, 0, len);
  }
  else
  {
    fprintf(stderr, "clearregion(%d, %d)\n", off,len);
  }

  return 0;	
}

/* move region of both character and attribute buffer */
int moveregion(vt, dst, src, len)
VTemu *vt;
int dst;
int src;
int len;
{

  if (dst < src)
  {
    /* assumes a low-to-high copy direction */
    memcpy(vt->buffer+dst, vt->buffer+src, len);
    memcpy(vt->attrib+dst, vt->attrib+src, len);
  }
  else
  {
    while(len-- > 0)
    {
      vt->buffer[dst+len] = vt->buffer[src+len];
      vt->attrib[dst+len] = vt->attrib[src+len];
    }
  }

  return(0);

}

void VTreset(vt)
VTemu *vt;
{
  vt->state = 0;
  vt->wrapping = 0;
  vt->hidecursor = 0;
  vt->focusblur = 0;
  vt->style = 0;
  vt->cx = vt->sx = 0;
  vt->cy = vt->sy = 0;
  vt->dirtylines = 0;
  vt->margintop = 0;
  vt->marginbot = vt->rows - 1;
  
  clearregion(vt, 0, MAXTERMROWS*MAXTERMCOLS);
  memset(vt->linelengths, 0, MAXTERMROWS);
  vt->dirtylines |= (1 << vt->rows) - 1;
}

void VTmovelines(vt, dst, src, n)
VTemu *vt;
int dst,src,n;
{
  int i;

  if (dst < 0 || dst + n > 25)
  {
    fprintf(stderr,"VTmovelines(%d %d)\n",dst,n);
    return;
  }

    /* move the existing pixels */
    movedisplaylines(vt, dst, src, n);   

    /* dst lines are now clean */
    for(i=0; i<n; i++)
    {
        vt->dirtylines &= ~(1 << (dst+i));
    }
    
    /* move the underlying buffer */
    moveregion(vt, RC2OFF(dst, 0),RC2OFF(src, 0), MAXTERMCOLS*n);

    /* outside dst region is dirty */
    if (dst < src)
    {
      memcpy(vt->linelengths+dst, vt->linelengths+src, n);
    
      /* need redrawing */
      for(i=dst+n; i<src+n; i++)
      {
        vt->dirtylines |= (1 << i);
        vt->linelengths[i] = 0;
      }
    }
    else
    {
      while(n--)
      {
        vt->linelengths[dst+n] = vt->linelengths[src+n];
      }
    
      for(i=src; i<dst; i++)
      {
        vt->dirtylines |= (1 << i);
        vt->linelengths[i] = 0;
      }
    }
}

void VTclearlines(vt, dst, n)
VTemu *vt;
int dst,n;
{
  int i;

  if (dst < 0 || dst + n > 25)
  {
    fprintf(stderr,"VTclearlines(%d %d)\n",dst,n);
    return;
  }

    cleardisplaylines(vt, dst,n);
    
    clearregion(vt, RC2OFF(dst,0), MAXTERMCOLS*n);

    /* dst lines are now clean */
    for(i=0; i<n; i++)
    {
      vt->dirtylines &= ~(1 << (dst+i));
      vt->linelengths[i] = 0;
    }
}

void VTnewline(vt, fastscroll)
VTemu *vt;
int fastscroll;
{
  int numlines = fastscroll ? 4 : 1;
  if (numlines >= (vt->marginbot - vt->margintop))
    numlines = 1;

    numlines = 1;

  /* scroll only region inside margintop/marginbot */
  vt->dirtylines |= (1<<vt->cy);
  vt->cx = 0;
  vt->cy++;
  if (vt->cy > vt->marginbot)
  {
    vt->cy -= numlines;
    /* scroll up */
    VTmovelines(vt, vt->margintop, vt->margintop+numlines, vt->marginbot - vt->margintop + 1 - numlines);
    VTclearlines(vt,vt->marginbot+1-numlines,numlines);
  }
  
  vt->dirtylines |= (1<<vt->cy);
}

int asciinum(msg, defval)
char *msg;
int defval;
{
  int c = 0;
  int i = 0;
  for(c=0; c<4; c++)
  {
    if (isdigit(msg[c]))
    {
      i *= 10;
      i += msg[c] - '0';
    }
    else
    {
      break;
    }    
  }
  return i ? i : defval;
  
}

int asciinum2(msg, defval)
char *msg;
int defval;
{
  int c = 0;

  for(c=0; c<8; c++)
  {
    if (msg[c] == ';')
      return asciinum(msg+c+1, defval);
    if (msg[c] >= 0x40)
      break;	
  }

  return defval;
}


void reply(fdout, c)
int fdout;
char c;
{
   write(fdout, &c, 1);
}

void VTfocus(vt, fdout)
VTemu *vt;
int fdout;
{
    if (vt->focusblur)
    {
      reply(fdout, 0x1b);
      reply(fdout,'[');
      reply(fdout,'I');
    }
}

void VTblur(vt, fdout)
VTemu *vt;
int fdout;
{
    if (vt->focusblur)
    {
      reply(fdout, 0x1b);
      reply(fdout,'[');
      reply(fdout,'O');
    }
}

void VTsetstyle(vt, i)
VTemu *vt;
int i;
{
    switch(i)
    {
      case 0:
        vt->style = 0;
        break;
        
      /* BOLD */
      case 1:
        vt->style |= vtBOLD;
        break;
      case 21:
        vt->style &= ~vtBOLD;
        break;

      /* INVERSE */
      case 7:
        vt->style |= vtINVERTED;
        break;
      case 27:
        vt->style &= ~vtINVERTED;
        break;

      case 30:
      case 37:
        break;

      case 40:
      case 47:
        break;

      default:
        break;
    }
}

int VToutput(vt, msg, n, fdout)
VTemu *vt;
char *msg;
int n;
int fdout;
{
  char c;
  int i,j;

  while (n--)
  {
    c = *msg++;

    if (vt->state > 0)
    {
      vt->escseq[vt->state] = c;
      vt->state++;
      /* CSI Escape */
      if (vt->escseq[1] == '[')
      {
        /* is it final byte */
        if (c >= 0x40 && c <= 0x7e  && vt->state > 2)
        {
          vt->escseq[vt->state] = '\0';
          
          switch(c)
          {
            case 'A':
            vt->cy -= asciinum(vt->escseq+2, 1);
            break;
            case 'B':
            vt->cy += asciinum(vt->escseq+2, 1);
            break;
            case 'C':
            vt->cx += asciinum(vt->escseq+2, 1);
            break;
            case 'D':
            vt->cx -= asciinum(vt->escseq+2, 1);
            break;
            case 'E':
            vt->cy += asciinum(vt->escseq+2, 1);
            vt->cx = 0;
            break;
            case 'F':
            vt->cy -= asciinum(vt->escseq+2, 1);
            vt->cx = 0;
            break;
            case 'G':
            vt->cx = asciinum(vt->escseq+2, 1) - 1;
            break;
            case 'H':
            vt->cy = asciinum(vt->escseq+2, 1) - 1;
            vt->cx = asciinum2(vt->escseq+2, 1) - 1;
            break;
            case 'J':
            i = asciinum(vt->escseq+2, 0);
            if (i == 1)
            {
              clearregion(vt, 0, RC2OFF(vt->cy,vt->cx));
              vt->dirtylines |= (1<<vt->cy);
            }
            else
            if (i == 2)
            {
              VTclearlines(vt, 0, vt->rows);
            }
            else
            {
              i = MAXTERMCOLS * MAXTERMROWS - RC2OFF(vt->cy,vt->cx);
              clearregion(vt, RC2OFF(vt->cy,vt->cx), i);
              for (i=vt->cy; i<vt->rows; i++)
              {
                vt->dirtylines |= (1 << i);
                vt->linelengths[i] = vt->cols - 1;
              }
              vt->linelengths[vt->cy] = vt->cx;
            }
            vt->cx = 0;
            vt->cy = 0;
            break;
            case 'K':
            i = asciinum(vt->escseq+2, 0);
            if (i == 1)
            {
              clearregion(vt, RC2OFF(vt->cy,0), vt->cx);
              vt->dirtylines |= (1<<vt->cy);
            }
            else
            if (i == 2)
            {
              VTclearlines(vt, vt->cy, 1);
            }
            else
            if (i == 3)
            {
              VTclearlines(vt, vt->cy, 1);
            }
            else
            {
              clearregion(vt, RC2OFF(vt->cy,vt->cx), vt->cols-vt->cx);
              vt->dirtylines |= (1<<vt->cy);
              vt->linelengths[vt->cy] = vt->cols - 1;
            }
            break;

            case 'L':
            i = asciinum(vt->escseq+2, 1);
            VTmovelines(vt, vt->cy+i, vt->cy, i);
            VTclearlines(vt, vt->cy, i);
            printf("case L:\n");
            break;
            
            case 'M':
            i = asciinum(vt->escseq+2, 1);
            VTmovelines(vt, vt->cy, vt->cy+i, i);
            VTclearlines(vt, vt->cy+i, vt->rows-vt->cy-i);
            printf("case M:\n");
            break;

            case 'P':
            i = asciinum(vt->escseq+2, 1);
            j = vt->cols - vt->cx - i;
            moveregion(vt, RC2OFF(vt->cy, vt->cx), RC2OFF(vt->cy, vt->cx+i),j);
            vt->dirtylines |= (1<<vt->cy);
            break;

            case 'S':
            i = asciinum(vt->escseq+2, 1);
            VTmovelines(vt, 0, i, i);
            VTclearlines(vt, vt->rows-i, i);
            break;
            case 'T':
            i = asciinum(vt->escseq+2, 1);
            VTmovelines(vt, i, 0, i);
            VTclearlines(vt, 0, i);
            break;

            case 'I':
            vt->cx += 8 * asciinum(vt->escseq+2, 1);
            vt->cx &= -8;
            break;
            case 'Z':
            vt->cx -= 8 * asciinum(vt->escseq+2, 1);
            vt->cx &= -8;
            break;

            case 'c':
            /* I am a VT100 */
            reply(fdout, 0x1b);
            reply(fdout,'[');
            reply(fdout,'?');
            reply(fdout,'1');
            reply(fdout,';');
            reply(fdout,'2');
            reply(fdout,'c');
            break;

            case 'd':
            vt->cy = asciinum(vt->escseq+2, 1) - 1;
            break;

            case 'f':
            vt->cy = asciinum(vt->escseq+2, 1) - 1;
            vt->cx = asciinum2(vt->escseq+2, 1) - 1;
            break;

            case 'm':
            /* display attrib */
            VTsetstyle(vt, asciinum(vt->escseq+2, 0));
            if (strchr(vt->escseq+2, ';'))
              VTsetstyle(vt, asciinum2(vt->escseq+2, 0));
            /* printf("%.*s   style = %d\n", vt->state, vt->escseq, vt->style); */
            break;

            case 'n':
            /* status report */
            i = asciinum(vt->escseq+2, 1);
            reply(fdout, 0x1b);
            reply(fdout, '[');
            if (i == 5)
              reply(fdout, 'c');
            if (i == 6)
            {
            reply(fdout, '0' + vt->cy + 1);
            reply(fdout, ';');
            reply(fdout, '0' + vt->cx + 1);
            reply(fdout, 'R');
            }
            break;

            case 'r':
            vt->margintop = asciinum(vt->escseq+2, 1) - 1;
            vt->marginbot = asciinum2(vt->escseq+2, 1) - 1;
            vt->cx = 0;
            vt->cy = 0;
            break;

            case 't':
            i = asciinum(vt->escseq+2, 1);
            j = asciinum2(vt->escseq+2, 1);
            fprintf(stderr,"window manipulation %d - %d\n",i,j);
            break;

            case 'h':
            case 'l':
            if (vt->escseq[2] == '?')
            {
              /* private attribs */
              i = asciinum(vt->escseq+3, 1);
              /* fprintf(stderr,"private mode attrib %d = %s\n", i, c=='h' ?"on" : "off"); */
              if (i == 1)
              {
                if (c=='h')
                {
                  /* Application Cursor Keys (DECCKM) */
                }
                else
                {
                  /* Normal Cursor Keys (DECCKM) */
                  
                  /* this is not correct but mostly useful to reset margins */
                  vt->margintop = 0;
                  vt->marginbot = vt->rows - 1;
                }
              }
              if (i == 7)
              {
                vt->wrapping = (c=='h');
              }
              if (i == 25)
              {
                vt->hidecursor = (c=='l');
              }
              if (i == 47)
              {
                /* we dont implement saved screen */
                VTclearlines(vt, 0, vt->rows);
                vt->cx = vt->cy = 0;
              }
              if (i == 1004)
              {
                /* has focus / lost focus */
                vt->focusblur = (c=='l');
                VTfocus(vt, fdout);
              }
              if (i == 1049)
              {
                /* we dont implement an alt buffer */
                VTclearlines(vt, 0, vt->rows);
                vt->cx = vt->cy = 0;
              }
            }
            if (vt->escseq[2] == '=')
            {
              /* set mode */
              i = asciinum(vt->escseq+3, 1);
              /* fprintf(stderr,"set mode %d\n", i); */
              
              if (i == 7)
              {
                vt->wrapping = (c=='h');
              }
              
            }
            break;
            
            default:
            fprintf(stderr,"unhandled: %d %c\n",asciinum(vt->escseq+2, 1),c);
            break;
          }
          
          vt->state = 0;
        }
      }
      else
      if (vt->escseq[1] >= '(' && vt->escseq[1] <= ')')
      {
        if (c == '0' || c == 'A' || c == 'B')
        {
          fprintf(stderr,"Designate G0/1 charset\n");
          vt->state = 0;
        }
      }
      else
      {
        /* Fs Escape */
        if (c == 'D')
        {
          i = vt->marginbot - vt->margintop;
          VTmovelines(vt, vt->margintop, vt->margintop+1, i);
          VTclearlines(vt, vt->marginbot, 1);
        }
        else
        if (c == 'M')
        {
          i = vt->marginbot - vt->margintop;
          VTmovelines(vt, vt->margintop+1, vt->margintop, i);
          VTclearlines(vt, vt->margintop, 1);
        }
        else
        if (c == 'E')
        {
          if (vt->cy < vt->rows)
            vt->cy++;
        }
        else
        if (c == 'c')
        {
          /* reset to initial state */
          VTreset(vt);
        }
        else
        /* Fp Escape */
        if (c == '7')
        {
            /* save cursor */
          vt->sx = vt->cx;
          vt->sy = vt->cy;
        }
        else
        if (c == '8')
        {
          /* restore cursor */
          vt->cx = vt->sx;
          vt->cy = vt->sy;
        }
        else
        if (c == '=')
        {
          /* fprintf(stderr,"Application Keypad (DECKPAM)\n"); */
        }
        else
        if (c == '>')
        {
          /* fprintf(stderr,"Normal Keypad (DECKPNM)\n"); */
        }
        else
        {
          /* unhandled */
          n = n;
        }
        
        vt->state = 0;
      }
    }
    else
    if (c == 0x1b)
    {
      /* escape code */
      vt->escseq[0] = c;
      vt->state = 1;
    }
    else
    if (c == 0x08)
    {
      if (vt->cx)
        vt->cx--;
      vt->dirtylines |= (1 << vt->cy);
    }
    else
    if (c == UNICR)
    {
      vt->cx = 0;
    }
    else
    if (c == UNILF)
    {
      VTnewline(vt, n > 50);
    }
    else
    if (c == '\t')
    {
      vt->cx += 8;
      vt->cx &= -8;
    }
    else
    if (c == '\f')
    {
      fprintf(stderr, "Form Feed\n");
      
      vt->margintop = 0;
      vt->marginbot = vt->rows - 1;

      i = vt->rows - vt->cy;
      while (i--)
        VTnewline(vt, 1);
    }
    else
    if (isprint(c))
    {
      if (vt->cx < vt->cols)
      {
        vt->buffer[RC2OFF(vt->cy,vt->cx)] = c;
        vt->attrib[RC2OFF(vt->cy,vt->cx)] = vt->style;
        vt->dirtylines |= (1<<vt->cy);
        if (vt->cx > vt->linelengths[vt->cy])
          vt->linelengths[vt->cy] = vt->cx;
      }
      vt->cx++;
      if (vt->wrapping && vt->cx >= vt->cols)
      {
        VTnewline(vt, n > 32);
      }
    }
    else
    {
      /* unhandled */
      c = c;
    }
  }

  return 0;
}

