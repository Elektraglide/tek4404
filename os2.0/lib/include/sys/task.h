/*
        UniFlex Task Table Entry
*/

struct task {
       struct task *tslink,   /*  link to running tasks */
                   *tsslnk;   /*  link to sleeping tasks  */
       char         topmem,   /*  physical top memory page  */
                    tsstat,   /*  task status code  */
                    tsmode,   /*  task mode bits  */
                    tsprir;   /*  task priority  */
       unsigned     tsuid,    /*  user id field  */
                    tstid,    /*  task id  */
                    tstidp,   /*  parent's task id  */
                    tstty;    /*  control terminal id  */
       char         tssgnl,   /*  signal number  */
                    tsbias,   /*  task priority bias  */
                    tsact,    /*  task activity  */
                    tsage,    /*  task residency age  */
                   *tsevnt,   /*  task event pointer  */
                   *tstext,   /*  task text table pointer  */
                   *tsswap,   /*  disk offset of swap image  */
                    tssize;   /*  size of swap image  */
       unsigned     tsalrm;   /*  seconds until alarm  */
};

/*
      task status codes
*/
#define TRUN   '\1'   /*  running  */
#define TSLEEP '\2'   /*  sleeping (high priority)  */
#define TWAIT  '\3'   /*  waiting (low priority)  */
#define TCREAT '\4'   /*  creating new task  */
#define TTERM  '\5'   /*  termination process  */
#define TTRACE '\6'   /*  trace mode  */

/*
      task mode codes
*/
#define TCORE  0x01   /*  task is in core  */
#define TLOCK  0x02   /*  task is locked in core  */
#define TSYSTM 0x04   /*  task is system scheduler  */
#define TTRACP 0x08   /*  task is being traced  */
#define TSWAPO 0x10   /*  task is being swapped  */
#define TARGX  0x20   /*  task is in argument expansion  */


