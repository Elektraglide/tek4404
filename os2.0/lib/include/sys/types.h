/*
     <sys/types.h>
*/


#ifndef types_h
#define	types_h

/*
     int utime() definitions
 
          utimbuf   A structure containing modification and
                    access times to be applied to a file by
                    "utime()"
          actime    The access time to apply (unused by
                    UniFLEX, included only for compatability)
          modtime   The modification time to apply
*/

struct utimbuf
{
     long      actime;      /* Access time, unused */
     long      modtime;     /* Modification time */
};


/*
     Data type definitions:
         
          time_t    The system-time value's data type
*/

typedef   long      time_t;

/*
 *  Other type definitions for UNIX (c) stdio compatibility
 */
typedef	short		dev_t;
typedef unsigned long	ino_t;
typedef long		off_t;

#endif types_h

