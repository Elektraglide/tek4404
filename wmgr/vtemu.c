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
#ifndef __clang__
int memmove(dst, src, len)
char *dst;
char *src;
int len;
{
  memcpy(dst, src, len);

  return(0);

}
#endif



void VTreset(vt)
VTemu *vt;
{
  vt->state = 0;
  vt->wrapping = 0;
  vt->hidecursor = 0;
  vt->focusblur = 0;
  vt->style = 0;
  vt->cx = 0;
  vt->cy = 0;
  vt->dirty = 0xffffffff;
  vt->margintop = 0;
  vt->marginbot = vt->rows - 1;
  
  memset(vt->buffer, 0, vt->rows*vt->cols);
  memset(vt->attrib, 0, vt->rows*vt->cols);
  vt->dirty |= 0xffffffff;
}

void VTmovelines(vt, dst, src, n)
VTemu *vt;
int dst,src,n;
{
  int i;

    memmove(vt->buffer+vt->cols*dst, vt->buffer+vt->cols*src, vt->cols*n);
    memmove(vt->attrib+vt->cols*dst, vt->attrib+vt->cols*src, vt->cols*n);
    for(i=0; i<n; i++)
    {
      vt->dirty |= (1 << (dst+i));
      vt->dirty |= (1 << (src+i));
    }
}
void VTclearlines(vt, dst, n)
VTemu *vt;
int dst,n;
{
  int i;

    memset(vt->buffer+vt->cols*dst, 0, vt->cols*n);
    memset(vt->attrib+vt->cols*dst, 0, vt->cols*n);
    for(i=0; i<n; i++)
    {
      vt->dirty |= (1 << (dst+i));
    }
}

void VTnewline(vt)
VTemu *vt;
{
  /* scroll only region inside margintop/marginbot */
  vt->cx = 0;
  vt->cy++;
  if (vt->cy > vt->marginbot)
  {
    /* scroll up */
    VTmovelines(vt, vt->margintop, vt->margintop+1, vt->marginbot - vt->margintop);
    VTclearlines(vt,vt->marginbot,1);
    vt->cy--;
  }
  vt->dirty |= (1<<vt->cy);
}

int asciinum(msg, defval)
char *msg;
int defval;
{
  int i = 0;
  while (isdigit(*msg))
  {
    i *= 10;
    i += *msg++ - '0';
  }
  return i ? i : defval;
  
}

int asciinum2(msg, defval)
char *msg;
int defval;
{
  while(*msg++ != ';');
  return asciinum(msg, defval);
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
        vt->style |= 1;
        break;
      case 21:
        vt->style &= ~1;
        break;

      /* INVERSE */
      case 7:
        vt->style |= 2;
        break;
      case 27:
        vt->style &= ~2;
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
              memset(vt->buffer, 0, vt->cols*vt->cy+vt->cx);
              memset(vt->attrib, 0, vt->cols*vt->cy+vt->cx);
              vt->dirty |= 0xffffffff;
            }
            else
            if (i == 2)
            {
              VTclearlines(vt, 0, vt->rows);
            }
            else
            {
              i = vt->cols * vt->rows;
              memset(vt->buffer+vt->cols*vt->cy+vt->cx, 0, i - (vt->cy*vt->cols+vt->cx));
              memset(vt->attrib+vt->cols*vt->cy+vt->cx, 0, i - (vt->cy*vt->cols+vt->cx));
              vt->dirty |= 0xffffffff;
            }
            vt->cx = 0;
            vt->cy = 0;
            break;
            case 'K':
            i = asciinum(vt->escseq+2, 0);
            if (i == 1)
            {
              memset(vt->buffer+vt->cols*vt->cy, 0, vt->cx);
              memset(vt->attrib+vt->cols*vt->cy, 0, vt->cx);
              vt->dirty |= (1<<vt->cy);
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
              memset(vt->buffer+vt->cols*vt->cy+vt->cx, 0, vt->cols-vt->cx);
              memset(vt->attrib+vt->cols*vt->cy+vt->cx, 0, vt->cols-vt->cx);
              vt->dirty |= (1<<vt->cy);
            }
            break;

            case 'L':
            i = asciinum(vt->escseq+2, 1);
            memmove(vt->buffer+vt->cols*(vt->cy+i), vt->buffer+vt->cols*vt->cy, vt->cols*i);
            memmove(vt->attrib+vt->cols*(vt->cy+i), vt->attrib+vt->cols*vt->cy, vt->cols*i);
            VTclearlines(vt, vt->cy, i);

            break;
            case 'M':
            i = asciinum(vt->escseq+2, 1);
            memmove(vt->buffer+vt->cols*vt->cy, vt->buffer+vt->cols*(vt->cy+i), vt->cols*i);
            memmove(vt->attrib+vt->cols*vt->cy, vt->attrib+vt->cols*(vt->cy+i), vt->cols*i);
            memset(vt->buffer+vt->cols*(vt->cy+i), 0, vt->cols*(vt->rows-vt->cy-i));
            memset(vt->attrib+vt->cols*(vt->cy+i), 0, vt->cols*(vt->rows-vt->cy-i));
            break;

            case 'P':
            i = asciinum(vt->escseq+2, 1);
            j = vt->cols - vt->cx - i;
            memmove(vt->buffer+vt->cols*vt->cy+vt->cx,vt->buffer+vt->cols*vt->cy+vt->cx+i,j);
            memmove(vt->attrib+vt->cols*vt->cy+vt->cx,vt->attrib+vt->cols*vt->cy+vt->cx+i,j);
            break;

            case 'S':
            i = asciinum(vt->escseq+2, 1);
            memmove(vt->buffer, vt->buffer+vt->cols*i, vt->cols*i);
            memmove(vt->attrib, vt->attrib+vt->cols*i, vt->cols*i);
            VTclearlines(vt, vt->rows-i, i);
            break;
            case 'T':
            i = asciinum(vt->escseq+2, 1);
            memmove(vt->buffer+vt->cols*i, vt->buffer, vt->cols*i);
            memmove(vt->attrib+vt->cols*i, vt->attrib, vt->cols*i);
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
          VTmovelines(vt, vt->margintop, vt->margintop+1, vt->marginbot - vt->margintop);
          VTclearlines(vt, vt->marginbot, 1);
        }
        else
        if (c == 'M')
        {
          i = vt->marginbot - vt->margintop;
          memmove(vt->buffer+vt->cols*(vt->margintop+1), vt->buffer+vt->cols*vt->margintop, vt->cols*i);
          memmove(vt->attrib+vt->cols*(vt->margintop+1), vt->attrib+vt->cols*vt->margintop, vt->cols*i);
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
      vt->dirty |= (1 << vt->cy);
    }
    else
    if (c == UNICR)
    {
      vt->cx = 0;
    }
    else
    if (c == UNILF)
    {
      VTnewline(vt);
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
        VTnewline(vt);
    }
    else
    if (isprint(c))
    {
      if (vt->cx < vt->cols)
      {
        vt->buffer[vt->cy*vt->cols+vt->cx] = c;
        vt->attrib[vt->cy*vt->cols+vt->cx] = vt->style;
        vt->dirty |= (1<<vt->cy);
      }
      vt->cx++;
      if (vt->wrapping && vt->cx >= vt->cols)
      {
        VTnewline(vt);
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

