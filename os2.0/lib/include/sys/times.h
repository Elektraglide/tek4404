/*
    Task timing information
*/

#define times_h

#ifndef types_h
#include <sys/types.h>
#endif

/*
     This structure defines the data returned by the
     "times()" system call.

          tms_utime    User-time, current task, in CPU-seconds
          tms_stime    System time, current task, in CPU-seconds
          tms_cutime   User-time, all decendents, in CPU-seconds
          tms_cstime   System-time, all decendents, in CPU-seconds
*/

struct tms
{
       time_t    tms_utime;
       time_t    tms_stime;
       time_t    tms_cutime;
       time_t    tms_cstime;
};
