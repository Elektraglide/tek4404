#include <stdio.h>
#include <math.h>
#include <font.h>
#include <graphics.h>
#include <signal.h>

#include "mathf.h"

#define PAGE_FLIP

#ifndef unix
#info 3d
#info Version 1.0
#tstmp
#endif

extern struct POINT page;
extern struct BBCOM bb;
extern int pindex;

typedef struct
{
    vec3 wc;
    vec3 cc;
    short vpx, vpy;
} Vertex3d;

real zero,half,one,two, fov,camdist;
vec3 spinaxis;

real costable[128],sintable[128];

short vcount;
Vertex3d vertices[2048];

short lcount;
unsigned short lines[2048][2];

mat33 ltm;

int addvertex(x, y, z)
double x, y, z;
{
    vertices[vcount].wc.x.fv = x * 1.0;
    vertices[vcount].wc.y.fv = y * 1.0;
    vertices[vcount].wc.z.fv = z * 1.0;
    
    return vcount++;
}

void addline(a, b)
int a, b;
{
    int i;

    /* unique edges only */  
    for(i=0; i<lcount; i++)
    {
      if (lines[i][0] == b && lines[i][1] == a)
        return;	
    }

    lines[lcount][0] = a;
    lines[lcount][1] = b;
    lcount++;
}

void init3d()
{
  int i;

 printf("build table\n");  
 for(i=0; i<128; i++)
 {
   costable[i].fv = cos((i/128.0) * 2.0 * 3.1415926);	
   sintable[i].fv = sin((i/128.0) * 2.0 * 3.1415926);	
 }

    zero.fv = 0.0;
    half.fv = 0.5;
    one.fv = 1.0;
    two.fv = 2.0;

    fov.fv = 2.0;
    camdist.fv = 5.0;
    spinaxis.x.fv = 0.666;
    spinaxis.y.fv = 0.666;
    spinaxis.z.fv = 0.333;

  
    vcount = 0;

    addvertex(-1.0, -1.0, -1.0);
    addvertex( 1.0, -1.0, -1.0);
    addvertex( 1.0,  1.0, -1.0);
    addvertex(-1.0,  1.0, -1.0);

    addvertex(-1.0, -1.0,  1.0);
    addvertex( 1.0, -1.0,  1.0);
    addvertex( 1.0,  1.0,  1.0);
    addvertex(-1.0,  1.0,  1.0);

    lcount = 0;

    addline(0, 1);
    addline(1, 2);
    addline(2, 3);
    addline(3, 0);

    addline(4, 5);
    addline(5, 6);
    addline(6, 7);
    addline(7, 4);

    addline(0, 4);
    addline(1, 5);
    addline(2, 6);
    addline(3, 7);
    
    printf("model has %d vertices %d edges\n", vcount, lcount);
}

void scalev(s, v)
int s;
vec3* v;
{
    v->x.iv = mulf(v->x.iv, s);
    v->y.iv = mulf(v->y.iv, s);
    v->z.iv = mulf(v->z.iv, s);
}

void rotatem(mat, radians, v)
mat33* mat;
int radians;
vec3* v;
{
    vec3 leading,crossed,scaled;
    real cosf, sinf;

    cosf.iv = subf(one.iv, costable[radians & 127].iv);
    sinf.iv = sintable[radians & 127].iv;

    leading.x.iv = subf(one.iv, mulf(v->x.iv, v->x.iv));
    leading.y.iv = subf(one.iv, mulf(v->y.iv, v->y.iv));
    leading.z.iv = subf(one.iv, mulf(v->z.iv, v->z.iv));
    scalev(cosf.iv, &leading);

    crossed.x.iv = mulf(v->y.iv, v->z.iv);
    crossed.y.iv = mulf(v->z.iv, v->x.iv);
    crossed.z.iv = mulf(v->x.iv, v->y.iv);
    scalev(cosf.iv, &crossed);

    scaled.x.iv = v->x.iv;
    scaled.y.iv = v->y.iv;
    scaled.z.iv = v->z.iv;
    scalev(sinf.iv, &scaled);

    mat->row0.x.iv = subf(one.iv, leading.x.iv);
    mat->row0.y.iv = addf(crossed.z.iv, scaled.z.iv);
    mat->row0.z.iv = subf(crossed.y.iv, scaled.y.iv);
    mat->row1.x.iv = subf(crossed.z.iv, scaled.z.iv);
    mat->row1.y.iv = subf(one.iv, leading.y.iv);
    mat->row1.z.iv = addf(crossed.x.iv, scaled.x.iv);
    mat->row2.x.iv = addf(crossed.y.iv, scaled.y.iv);
    mat->row2.y.iv = subf(crossed.x.iv, scaled.x.iv);
    mat->row2.z.iv = subf(one.iv, leading.z.iv);
}

void rotateY(mat, radians)
mat33* mat;
int radians;
{
    real cf,sf;
    
    cf.iv = costable[radians & 127].iv;
    sf.iv = sintable[radians & 127].iv;

    mat->row0.x.iv = cf.iv;
    mat->row0.y.iv = zero.iv;
    mat->row0.z.fv = -sf.fv;
    mat->row1.x.iv = zero.iv;
    mat->row1.y.iv = one.iv;
    mat->row1.z.iv = zero.iv;
    mat->row2.x.fv = sf.fv;
    mat->row2.y.iv = zero.iv;
    mat->row2.z.iv = cf.iv;
}

scalem(mat, sx,sy,sz)
mat33* mat;
int sx,sy,sz;
{
   mat->row0.x.iv = mulf(mat->row0.x.iv, sx);
   mat->row0.y.iv = mulf(mat->row0.y.iv, sy);
   mat->row0.z.iv = mulf(mat->row0.z.iv, sz);
   mat->row1.x.iv = mulf(mat->row1.x.iv, sx);
   mat->row1.y.iv = mulf(mat->row1.y.iv, sy);
   mat->row1.z.iv = mulf(mat->row1.z.iv, sz);
   mat->row2.x.iv = mulf(mat->row2.x.iv, sx);
   mat->row2.y.iv = mulf(mat->row2.y.iv, sy);
   mat->row2.z.iv = mulf(mat->row2.z.iv, sz);
}

#define PERSP
void transform3d(radians)
int radians;
{
    mat33* mat = &ltm;
    Vertex3d* src = vertices;
    short count = vcount;
    real vpw, vph;

    rotatem(mat, radians, &spinaxis);

    /* viewport scaling */
    vpw.fv = 240.0;
    vph.fv = 240.0;
    scalem(mat, vpw.iv, vph.iv, one.iv); 

    while (count--)
    {
    	real ooz;
    	
        /* view space */
        src->cc.x.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.x.iv), vmmulf(src->wc.y.iv, mat->row1.x.iv)), vmmulf(src->wc.z.iv, mat->row2.x.iv));
        src->cc.y.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.y.iv), vmmulf(src->wc.y.iv, mat->row1.y.iv)), vmmulf(src->wc.z.iv, mat->row2.y.iv));
        src->cc.z.iv = vmaddf(vmaddf(vmmulf(src->wc.x.iv, mat->row0.z.iv), vmmulf(src->wc.y.iv, mat->row1.z.iv)), vmmulf(src->wc.z.iv, mat->row2.z.iv));

#ifdef PERSP
        /* perspective projection */
        ooz.iv = divf(fov.iv, vmaddf(camdist.iv, src->cc.z.iv)); 
#else
        /* parallel projection */
        ooz.iv = half.iv;
#endif
       
        src->vpx = ftoi(vmmulf(src->cc.x.iv, ooz.iv)) + 320;
        src->vpy = ftoi(vmmulf(src->cc.y.iv, ooz.iv)) + 240 + (page.y);

        src++;
    }
}

void draw3d()
{
    struct POINT p2;
    short i;

      bb.srcform = NULL;
      bb.rule = bbZero;
      bb.halftoneform = NULL;

      /* clear viewport */
      bb.destrect.x = 80;
      bb.destrect.y = page.y;
      bb.destrect.w = 480;
      bb.destrect.h = 480;
      RectDrawX(&bb.destrect, &bb);


      bb.rule = bbS;
      bb.destrect.w = 1;
      bb.destrect.h = 1;
      
    for (i = 0; i < lcount; i++)
    {
        bb.destrect.x = vertices[lines[i][0]].vpx;
        bb.destrect.y = vertices[lines[i][0]].vpy;

        p2.x = vertices[lines[i][1]].vpx;
        p2.y = vertices[lines[i][1]].vpy;

        PaintLine(&bb, &p2);
    }
}

int loadmodel(filename)
char *filename;
{
  char aline[128];
  float x,y,z;
  int p0,p1,p2,p3;
  
   FILE *fp = fopen(filename, "r");
   if (fp)
   {
   	 vcount = 0;
   	 lcount = 0;
   	 
     /* RWX is 1-based... */
     addvertex(0.0, 0.0, 0.0);	
   
     while (fgets(aline, sizeof(aline), fp))
     {
     	
       if (sscanf(aline, " Vertex %f %f %f", &x,&y,&z) == 3)
       {
         addvertex(x,y,z);	
       }
       else
       if (sscanf(aline, " Triangle %d %d %d", &p0,&p1,&p2) == 3)
       {
         addline(p0,p1);
         addline(p1,p2);
         addline(p2,p0);
       }
       else
       if (sscanf(aline, " Quad %d %d %d %d", &p0,&p1,&p2,&p3) == 4)
       {
         addline(p0,p1);
         addline(p1,p2);
         addline(p2,p3);
         addline(p3,p0);
       }
       
      if (lcount > 400)
        break;

     }

     printf("model has %d vertices %d edges\n", vcount, lcount);
     fclose(fp);
   }

   return 1;
}
