#include <stdio.h>
#include <math.h>
#include "mathf.h"

typedef union {
  unsigned int v32;
  unsigned short v16[2];
} dword;

typedef struct{
	unsigned int dummy;
	dword data;
	volatile dword status;

	/* SPC slave */
    dword select;

} ns32081;

ns32081* mapped_fpu;

vec3 src[8192], dst[8192];
mat33 mat;

/***********************************/
int flmul(lex, ley)
int lex;
int ley;
{
	register short ans,busy;
    register ns32081 *ptr = mapped_fpu;
    
	ptr->select.v16[0] = 0xbe; 	/* broadcast slave ID */
	ptr->data.v16[0] = 0x3142;	/* FMUL */
	ptr->data.v32 = lex;
	ptr->data.v32 = ley;

	ans = 16;
	do
	{
		busy = ptr->status.v16[0];
		if (busy == 0) break;
	} while (--ans != -1);

    return  ptr->data.v32;
}

int fladd(lex, ley)
int lex;
int ley;
{
	register short ans, busy;
    register ns32081 *ptr = mapped_fpu;

	ptr->select.v16[0] = 0xbe; 	/* broadcast slave ID */
	ptr->data.v16[0] = 0x0142;	/* FADD */
	ptr->data.v32 = lex;
	ptr->data.v32 = ley;

	ans = 16;
	do
	{
		busy = ptr->status.v16[0];
		if (busy == 0) break;
	} while (--ans != -1);

    return  ptr->data.v32;
}


void origxform(dst, src, mat, count)
vec3* dst;
vec3* src;
mat33* mat;
int count;
{

	while (count--)
	{
		dst->x.fv = (((src->x.fv * mat->row0.x.fv) + (src->y.fv * mat->row1.x.fv)) + (src->z.fv * mat->row2.x.fv));
		dst->y.fv = (((src->x.fv * mat->row0.y.fv) + (src->y.fv * mat->row1.y.fv)) + (src->z.fv * mat->row2.y.fv));
		dst->z.fv = (((src->x.fv * mat->row0.z.fv) + (src->y.fv * mat->row1.z.fv)) + (src->z.fv * mat->row2.z.fv));
		src++;
		dst++;
	}
}


void xform(dst, src, mat, count)
register vec3* dst;
register vec3* src;
mat33* mat;
register int count;
{

	while (count--)
	{
		dst->x.iv = fladd(fladd(flmul(src->x.iv, mat->row0.x.iv), flmul(src->y.iv, mat->row1.x.iv)), flmul(src->z.iv, mat->row2.x.iv));
		dst->y.iv = fladd(fladd(flmul(src->x.iv, mat->row0.y.iv), flmul(src->y.iv, mat->row1.y.iv)), flmul(src->z.iv, mat->row2.y.iv));
		dst->z.iv = fladd(fladd(flmul(src->x.iv, mat->row0.z.iv), flmul(src->y, mat->row1.z.iv)), flmul(src->z.iv, mat->row2.z.iv));
		src++;
		dst++;
	}
}

void asmxform(dst, src, mat, count)
register vec3* dst;
register vec3* src;
mat33* mat;
register int count;
{

	while (count--)
	{
		dst->x.iv = vmaddf(vmaddf(vmmul(src->x.iv, mat->row0.x.iv), vmmulf(src->y.iv, mat->row1.x.iv)), vmmulf(src->z.iv, mat->row2.x.iv));
		dst->y.iv = vmaddf(vmaddf(vmmul(src->x.iv, mat->row0.y.iv), vmmulf(src->y.iv, mat->row1.y.iv)), vmmulf(src->z.iv, mat->row2.y.iv));
		dst->z.iv = vmaddf(vmaddf(vmmul(src->x.iv, mat->row0.z.iv), vmmulf(src->y, mat->row1.z.iv)), vmmulf(src->z.iv, mat->row2.z.iv));
		src++;
		dst++;
	}
}

void setup()
{
	int i;

	for (i = 0; i < 8192; i++)
	{
		src[i].x.fv = 10.0;
		src[i].y.fv = 20.0;
		src[i].z.fv = 30.0;
	}

	mat.row0.x.fv = 0.0;
	mat.row0.y.fv = 1.0;
	mat.row0.z.fv = 0.0;
	mat.row1.x.fv = 0.0;
	mat.row1.y.fv = 0.0;
	mat.row1.z.fv = 1.0;
	mat.row2.x.fv = 1.0;
	mat.row2.y.fv = 0.0;
	mat.row2.z.fv = 0.0;
}

main(argc, argv)
int argc;
char **argv;
{
   short ans,busy;
   long t1,t2;
   
 mapped_fpu = (ns32081 *)phys(2);    	
 if (mapped_fpu)
 {
   register ns32081 *ptr = mapped_fpu;

   printf("using mapped fpu at 0x%8.8x\n", ptr);	

   ptr->select.v16[0] = 0xbe; 	/* broadcast slave ID */
   ptr->data.v16[0] = 0x3142;	/* FMUL */
   ptr->data.v32 = 0x0fda4049;	/* little endian 3.1415926 */
   ptr->data.v32 = 0x00004000;	/* little endian 2.0 */
  
   ans = 16;
   do
   {
     busy = ptr->status.v16[0];
     if (busy == 0) break;
   } while (--ans != -1);

   printf("busy = 0x%4.4x (%d)\n",busy,ans);

   printf("result = 0x%8.8x\n",ptr->data.v32);    


   setup();
   printf("Starting 8192 mat33 original transforms\n");
   t1 = time(NULL);
   origxform(dst, src, &mat, 8192);
   t2 = time(NULL);
   printf("FINISHED %d\n", t2 - t1);

   printf("Starting 8192 mat33 transforms\n");
   t1 = time(NULL);
   xform(dst, src, &mat, 8192);
   t2 = time(NULL);
   printf("FINISHED %d\n", t2 - t1);

   printf("Starting 8192 mat33 asm transforms\n");
   t1 = time(NULL);
   asmxform(dst, src, &mat, 8192);
   t2 = time(NULL);
   printf("FINISHED %d\n", t2 - t1);


 }
}
