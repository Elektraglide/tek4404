/*
     system accounting record produced by acct(fname);
*/

#define acct_h

struct acct { short    ac_uid;        /*  user id number               */
              long     ac_strt;       /*  starting time of task        */
              long     ac_end;        /*  ending time of task          */
              char     ac_syst[3];    /*  system time used by task     */
              char     ac_usrt[3];    /*  user time used by task       */
              unsigned ac_stat;       /*  task termination status      */
              char     ac_tty;        /*  controlling terminal number  */
              char     ac_mem;        /*  maximum memory used by task  */
              unsigned ac_blks;       /*  number if I/O requests       */
              char     ac_spare[2];
              char     ac_name[8];    /*  command name (first chars)   */
            } ;

/* 
 * format of history and utmp records
 */

struct hist
   {
    char tty_num[2];            /* terminal number - in ascii */
    char user_name[10];         /* name of user               */
    long time_field;            /* time of login/history rec  */
   };
