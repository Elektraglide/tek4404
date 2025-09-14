#include <stdio.h>
#include <math.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

#include "mathf.h"
#define abs(A) (A < 0 ? -A : A)

typedef union
{
 int fixed;
 struct {
   short integral;
   unsigned short fraction;	
 } part;
} fixed16;


static unsigned int mask[] = {
  0x80000000,0x40000000,0x20000000,0x10000000,
  0x08000000,0x04000000,0x02000000,0x01000000,
  0x00800000,0x00400000,0x00200000,0x00100000,
  0x00080000,0x00040000,0x00020000,0x00010000,
  0x00008000,0x00004000,0x00002000,0x00001000,
  0x00000800,0x00000400,0x00000200,0x00000100,
  0x00000080,0x00000040,0x00000020,0x00000010,
  0x00000008,0x00000004,0x00000002,0x00000001
};

void myLine32(bbcom,p2)
struct BBCOM *bbcom;
struct POINT *p2;
{
	register unsigned int *addr;
	fixed16 j;
	register int decInc;

    short x = bbcom->destrect.x;
    short y = bbcom->destrect.y;
    short x2 = p2->x;
    short y2 = p2->y;

	short yLonger=0;
	short shortLen=y2 - y;
	short longLen=x2 - x;
	if (abs(shortLen)>abs(longLen)) 
	{
		short swap = shortLen;
		shortLen = longLen;
		longLen = swap;

		yLonger=1;
	}

	if (longLen==0) decInc=0;
	else decInc = ((int)shortLen << 16) / longLen;

	if (yLonger) 
	{
		addr = (unsigned int *)(bbcom->destform->addr + (y<<7));
        j.part.integral = x;
        j.part.fraction = 0x8000;
		if (longLen>0) 
		{
return;
			/* make absolute y */
			longLen+=y;

			do			
	        {
		    	short c = *(short *)&j.part.integral;
				addr[c >> 5] ^= mask[c & 31];

				j.fixed += decInc;
				addr += 32;
	        } while(++y < longLen);

			return;
		}
		else
		{
return;

			longLen+=y;

			do
	        {
		    	short c = *(short *)&j.part.integral;
				addr[c >> 5] ^= mask[c & 31];
				j.fixed -= decInc;
				addr -= 32;
			} while(--y >= longLen);
		}
	}
	else
	{
		addr = (unsigned int *)bbcom->destform->addr + (x>>5);
	    j.part.integral = y;
	    j.part.fraction = 0x8000;
		if (longLen>0) 
		{
			/* make absolute x */
			longLen+=x;
			do
		    {
		    	short c = *(short *)&j.part.integral;
				addr[c << 5] ^= mask[x & 31];
				if ((x & 31) == 31)
					addr++;
				j.fixed += decInc;
			} while(++x < longLen);
		}
		else
		{
return;
			/* make absolute x */
			longLen+=x;
			do
		    {
		    	short c = *(short *)&j.part.integral;
				addr[c << 5] ^= mask[x & 31];
				if ((x & 31) == 0)
					addr--;
				j.fixed -= decInc;
			} while(--x >= longLen);
		}
	}
}

static unsigned char mask8[] = {
  0x80,0x40,0x20,0x10,
  0x08,0x04,0x02,0x01
};


void myLine8(bbcom,p2)
struct BBCOM *bbcom;
struct POINT *p2;
{
	register unsigned char *addr;
	register int decInc;

    short x = bbcom->destrect.x;
    short y = bbcom->destrect.y;
    short x2 = p2->x;
    short y2 = p2->y;

	short yLonger=0;
	short shortLen=y2 - y;
	short longLen=x2 - x;
	if (abs(shortLen)>abs(longLen)) 
	{
		short swap = shortLen;
		shortLen = longLen;
		longLen = swap;

		yLonger=1;
	}

	if (longLen==0) decInc=0;
	else decInc = ((int)shortLen << 16) / longLen;

	if (yLonger) 
	{
		fixed16 j;
		addr = (unsigned char *)(bbcom->destform->addr + (y<<7));
        j.part.integral = x;
        j.part.fraction = 0x8000;
		if (longLen>0) 
		{
return;
			/* make absolute y */
			longLen+=y;

			do			
	        {
		    	short c = *(short *)&j.part.integral;
				addr[c >> 5] ^= mask8[c & 7];

				j.fixed += decInc;
				addr += 32;
	        } while(++y < longLen);

			return;
		}
		else
		{
return;

			longLen+=y;

			do
	        {
		    	short c = *(short *)&j.part.integral;
				addr[c >> 5] ^= mask8[c & 7];
				j.fixed -= decInc;
				addr -= 32;
			} while(--y >= longLen);
		}
	}
	else
	{
		fixed16 j;

		addr = (unsigned char *)bbcom->destform->addr + (x>>3);
	    j.part.integral = y;
	    j.part.fraction = 0x8000;
		if (longLen>0) 
		{
			/* make absolute x */
			longLen+=x;
			do
		    {
		    	register short c = *(short *)&j.part.integral;
				addr[c << 7] ^= mask8[x & 7];
				if ((x & 7) == 7)
					addr++;
				j.fixed += decInc;
			} while(++x < longLen);
		}
		else
		{
return;
			/* make absolute x */
			longLen+=x;
			do
		    {
		    	short c = *(short *)&j.part.integral;
				addr[c << 5] ^= mask8[x & 31];
				if ((x & 31) == 0)
					addr--;
				j.fixed -= decInc;
			} while(--x >= longLen);
		}
	}
}


