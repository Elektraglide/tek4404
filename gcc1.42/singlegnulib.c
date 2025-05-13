/* Subroutines needed by GCC output code on some machines.  */
/* Compile this file with the Unix C compiler!  */

#include "config.h"

/* Don't use `fancy_abort' here even if config.h says to use it.  */
#ifdef abort
#undef abort
#endif

/* Define the C data type to use for an SImode value.  */

#ifndef SItype
#define SItype long int
#endif

/* Define the type to be used for returning an SF mode value
   and the method for turning a float into that type.
   These definitions work for machines where an SF value is
   returned in the same register as an int.  */

#ifndef SFVALUE  
#define SFVALUE int
#endif

#ifndef INTIFY
#define INTIFY(FLOATVAL)  (intify.f = (FLOATVAL), intify.i)
#endif

#ifdef GNULIB_NEEDS_DOUBLE
#define FLOAT_ARG_TYPE double
#define FLOATIFY(ARG)  ((float) (ARG))
#endif

#ifndef FLOATIFY
#define FLOATIFY(INTVAL)  ((INTVAL).f)
#endif

#ifndef FLOAT_ARG_TYPE
#define FLOAT_ARG_TYPE union flt_or_int
#endif

union flt_or_int { int i; float f; };

matherr(x)
struct exception *x;
{
  return 1;	
}

#include <stdio.h>
/* This is used by the `assert' macro.  */
void
__eprintf (string, expression, line, filename)
     char *string;
     char *expression;
     int line;
     char *filename;
{
  fprintf (stderr, string, expression, line, filename);
  fflush (stderr);
  abort ();
}



SItype
__umulsi3 (a, b)
     unsigned SItype a, b;
{
  return a * b;
}



SItype
__mulsi3 (a, b)
     SItype a, b;
{
  return a * b;
}



SItype
__udivsi3 (a, b)
     unsigned SItype a, b;
{
  return a / b;
}



SItype
__divsi3 (a, b)
     SItype a, b;
{
  return a / b;
}



SItype
__umodsi3 (a, b)
     unsigned SItype a, b;
{
  return a % b;
}



SItype
__modsi3 (a, b)
     SItype a, b;
{
  return a % b;
}



SItype
__lshrsi3 (a, b)
     unsigned SItype a, b;
{
  return a >> b;
}



SItype
__lshlsi3 (a, b)
     unsigned SItype a, b;
{
  return a << b;
}



SItype
__ashrsi3 (a, b)
     SItype a, b;
{
  return a >> b;
}



SItype
__ashlsi3 (a, b)
     SItype a, b;
{
  return a << b;
}


/* these  are all wrapped my gnulibabi to covert Uniflex return double as pointer, to return double as d0,d1 */

double
__uniflex_divdf3 (a, b)
     double a, b;
{
  return a / b;
}



double
__uniflex_muldf3 (a, b)
     double a, b;
{
  return a * b;
}



double
__uniflex_negdf2 (a)
     double a;
{
  return -a;
}



double
__uniflex_adddf3 (a, b)
     double a, b;
{
  return a + b;
}



double
__uniflex_subdf3 (a, b)
     double a, b;
{
  return a - b;
}

double
__uniflex_floatsidf(a)
SItype a;
{
    return (double)a;
}

/* AB added */
double
__uniflex_floatunsidf(a)
unsigned int a;
{
    return (double)a;
}



double
__uniflex_extendsfdf2(a)
FLOAT_ARG_TYPE a;
{
    return FLOATIFY(a);
}




SItype
__cmpdf2 (a, b)
     double a, b;
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  return 0;
}



#define HIGH_BIT_INT_COEFF  (1 << (BITS_PER_WORD - 1))
#define HIGH_BIT_COEFF  (2 * (double) (1 << (BITS_PER_WORD - 2)))

SItype
__fixunsdfsi (a)
     double a;
{
  if (a < HIGH_BIT_COEFF)
    return (SItype)a;
  /* Convert large positive numbers to smaller ones,
     then increase again after you have a fixed point number.  */
  else
    return ((SItype) (a - HIGH_BIT_COEFF)) + HIGH_BIT_INT_COEFF;
}


SItype
__fixdfsi (a)
     double a;
{
  return (SItype) a;
}


__eqdf2(a,b)
double a,b;
{
	return a != b;
}

__ltdf2(a,b)
double a,b;
{
	return a < b;
}

__gtdf2(a,b)
double a,b;
{
	return a > b;
}

__nedf2(a,b)
double a,b;
{
	return a == b;
}



SFVALUE
__addsf3 (a, b)
     FLOAT_ARG_TYPE a, b;
{
  union flt_or_int intify;
  return INTIFY (FLOATIFY (a) + FLOATIFY (b));
}



SFVALUE
__negsf2 (a)
     FLOAT_ARG_TYPE a;
{
  union flt_or_int intify;
  return INTIFY (-FLOATIFY (a));
}



SFVALUE
__subsf3 (a, b)
     FLOAT_ARG_TYPE a, b;
{
  union flt_or_int intify;
  return INTIFY (FLOATIFY (a) - FLOATIFY (b));
}



SItype
__cmpsf2 (a, b)
     FLOAT_ARG_TYPE a, b;
{
  if (FLOATIFY (a) > FLOATIFY (b))
    return 1;
  else if (FLOATIFY (a) < FLOATIFY (b))
    return -1;
  return 0;
}



SFVALUE
__mulsf3 (a, b)
     FLOAT_ARG_TYPE a, b;
{
  union flt_or_int intify;
  return INTIFY (FLOATIFY (a) * FLOATIFY (b));
}



SFVALUE
__divsf3 (a, b)
     FLOAT_ARG_TYPE a, b;
{
  union flt_or_int intify;
  
  return INTIFY (FLOATIFY (a) / FLOATIFY (b));
}



SFVALUE
__truncdfsf2 (a)
     double a;
{
  union flt_or_int intify;
  return INTIFY (a);
}


int __avoid_ranlib_warning;  /* Don't let symbol table be empty.  */



/* frills for C++ */
