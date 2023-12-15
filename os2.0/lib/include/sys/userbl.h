/*
    definition of UniFlex user block
*/
struct userbl {
     unsigned    *usp,         /*  stack pointer  */
                 *uswi2v;      /*  SWI2 vector  */
     char         umem[16],    /*  memory map  */
                  urglst[8];   /*  register list  */
     unsigned     umark[3];    /*  register stuff  */
     struct task *utask;       /*  task block pointer  */
     unsigned     uuid,        /*  effective user id  */
                  uuida,       /*  actual user id  */
                  ucrdir;      /*  fdn of current directory  */
     char         uwrkbf[14];  /*  work buffer for path name  */
     unsigned     ufdel,       /*  first deleted directory  */
                  ufdn;        /*  fdn number of current dir  */
     char         udname[14];  /*  current directory entry  */
     unsigned     ulstdr;      /*  fdn of last searched dir  */
     char         udperm,      /*  default file permissions  */
                 *ucname;      /*  pointer to command name  */
     unsigned    *ufiles[16],  /*  pointers to file blocks  */
                  usarg[4];    /*  user argument registers  */
     char         uiosp;       /*  I/O space flag  */
     unsigned     uistrt,      /*  I/O start address  */
                  uicnt;       /*  I/O byte count  */
     long         uipos;       /*  I/O position count  */
     char         umaprw,      /*  read/write mapping flag  */
                  unxtbl[3],   /*  pre-read buffered block  */
                  uinter,      /*  interrupt error flag  */
                  utimu[3],    /*  task user time  */
                  utims[3];    /*  task system time  */
     long         utimeuc,     /*  total child's user time  */
                  utimesc;     /*  total child's system time  */
     char         usizet,      /*  text size (in segments)  */
                  usized,      /*  data size (in segments)  */
                  usizes;      /*  stack size (in segments)  */
     unsigned     usigs[12],   /*  total status of all signals  */
                  uprfpc,      /*  profile program counter  */
                 *uprfbf,      /*  pointer to profile buffer  */
                  uprfsz,      /*  profile buffer size  */
                  upfrsc;      /*  profile scale byte  */
};

#define upostb urglst[7]       /*  system call post byte  */

