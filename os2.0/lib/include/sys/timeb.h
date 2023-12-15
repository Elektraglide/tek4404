
/* 'ftime' structure */

#define timeb_h

struct timeb {
       long  time;         /* time in seconds */
       char  tm_tik;       /* ticks in second (tenths) */
       char  dstflag;      /* daylight savings flag */
       short timezone;     /* time zone */
};

#ifndef   NULL
#define   NULL      0
#endif
