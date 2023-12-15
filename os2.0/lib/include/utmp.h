/*
     utmp.h - Define information needed to handle the
              "/act/utmp" accounting file for Tektronix 4400.
*/

/*
     struct utmp - Structure containing the information
                   extracted from a record in the file
*/

struct utmp
{
     char           ut_user[8];         /* Login name */
     char           ut_id[4];           /* Entry number */
     char           ut_line[12];        /* Device name */
     unsigned long  ut_time;            /* Login time */
};

#ifndef   NULL
#define   NULL      0
#endif

/*
     Routines used to reference the file "/act/utmp"
*/

struct utmp   *getutent();         /* Get next entry */
struct utmp   *getutline();        /* Get next entry on line */
void           setutent();         /* Rewind file */
void           endutent();         /* Close file */

/*
     The following entry points are defined in UNIX to handle
     the UNIX "/etc/utmp" file which contains more information.
     They make no sense for the more simple "/act/utmp" file
     in Tektronix 4400:

     struct utmp *getutid()   Get next entry with id
     void pututline()         Write formatted line
     void utmpname()          Change to alternate utmp-like file

     An entry in the UNIX "/etc/utmp" file is defined thusly
     [from UNIX System User's Manual, GETUT(3C)]:

     struct utmp {
          char      ut_user[8];    user login name
          char      ut_id[4];      /etc/inittab id (usu line #)
          char      ut_line[12];   device name
          short     ut_pid;        process id
          short     ut_type;       type of entry
          struct    exit_status {
            short     e_termination;    process termination status
            short     e_exit;           process exit status
          } ut_exit;               exit status of a status marked
                                   DEAD_PROCESS
          time_t    ut_time;       time entry was made
     ];
*/
