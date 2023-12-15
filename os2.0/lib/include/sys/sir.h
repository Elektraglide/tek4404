/*
 * definition of the uniflex superblock...
 */

#define        CFDN    50
#define        CDBLKS  100
struct sir {
       char    supd;           /* sir update flag */
       char    swprot;         /* mounted read only flag */
       char    slkfr;          /* lock for free list manipulation */
       char    slkfdn;         /* lock for fdn list manipulation */
       long    sinitd;         /* initialising system identifier */
       long    scrtim;         /* creation time */
       long    sutime;         /* last update time */
       short   sszfdn;         /* total fdn blocks */
       char    ssizfr[3];      /* fdn blocks + free blocks + used blocks +1 */
       char    sfreec[3];      /* number of unused blocks */
       short   sfdnc;          /* number of free fdns */
       char    sfname[14];     /* system name */
       char    spname[14];     /* volume name */
       short   sfnumb;         /* volume number */
       short   sflawc;         /* flawed block count */
       char    sdenf;          /* density flag; 0=single */
       char    ssidf;          /* side flag; 0=single */
       char    sswpbg[3];      /* swap starting block number */
       char    sswpsz[2];      /* swap block count */
       char    sspare[23];     /* spare bytes */
       char    snfdn;          /* number of in-core fdns */
       char    fdnlst[CFDN][2];/* list of free fdns */
       char    blkptr;         /* number of in-core free blocks */
       char    blklist[CDBLKS][3]; /* list of free blocks */
};
