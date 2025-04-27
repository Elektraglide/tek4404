/* 
 * mathf.h -- float math functions
 *
 * In order to avoid C compiler converting params / results to double 
 * we use a union of float and int
 */

typedef union
{
    int iv;
    float fv;
} real;

typedef struct
{
    real x, y, z;
} vec3;

typedef struct
{
    vec3 row0;
    vec3 row1;
    vec3 row2;
} mat33;


extern int ftoi();
extern int mulf();
extern int divf();
extern int addf();
extern int subf();
extern int maddf();

extern int vmmulf();
extern int vmdivf();
extern int vmaddf();

