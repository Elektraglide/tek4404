/* AB missing file */
#ifndef STDARG_H
#define STDARG_H
#include <varargs.h>

#undef va_start
#undef va_end

#define va_start(AP, PARAM)  (AP=(char *) &PARAM + _va_rounded_size(PARAM))
#define va_end(AP)


typedef unsigned int size_t;

#endif

