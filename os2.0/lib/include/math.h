/* Tektronix 4404 Operating System math definitions */

#define math_h

/* Largest possible values */

     extern    double    HUGE;          /* Largest double */
     extern    float     SHUGE;         /* Largest float */


/* Function definitions */

     /* General purpose functions */

          double    ceil();
          double    fabs();
          double    floor();
          double    fmod();
          double    pow();
          double    sqrt();

     /* Logarithmic functions */

          double    exp();
          double    log();
          double    log10();

     /* Trigonometric functions */

          double    sin();
          double    cos();
          double    tan();

     /* Arc-trigonometric functions */

          double    asin();
          double    acos();
          double    atan();
          double    atan2();

     /* Hyperbolic functions */

          double    sinh();
          double    cosh();
          double    tanh();

     /* Miscellaneous functions */

          int       matherr();
          double    frexp();
          double    ldexp();
          double    modf();
          int       isnan();
          int       finite();

     /*   Character conversion functions */

          char     *ecvt();
          char     *fcvt();
          char     *gcvt();
          double    atof();


/*
     Floating-point exception-handling definitions
        - struct exception
        - exception code
*/

     /* Exception structure definition */

     struct exception
     {
          int       type;          /* Exception type (code) */
          char     *name;          /* Function causing exception */
          double    arg1;          /* 1st arg to fcn (if any) */
          double    arg2;          /* 2nd arg to fcn (if any) */
          double    retval;        /* Non-std return value */
     };


     /* Exception code definitions:
          DOMAIN    Argument out of range
          SING      Singularity
          OVERFLOW  Floating-point overflow
          UNDERFLOW Floating-point underflow
          TLOSS     Total loss-of-significance
          PLOSS     Partial loss-of-significance
     */

#define   DOMAIN    0
#define   SING      1
#define   OVERFLOW  2
#define   UNDERFLOW 3
#define   TLOSS     4
#define   PLOSS     5
