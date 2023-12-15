/*
     structure of mount table entry
*/

#define mtab_h

#define     MTFILE      "/etc/mtab"
#define     DEVLEN      28
#define     DIRLEN      60

struct mtab { unsigned m_devno;    /*  mounted device number   */
              unsigned m_usrid;    /*  user id of perpetrator  */
              char     m_dev[DEVLEN];  /*  name of special device  */
              long     m_time;     /*  time device mounted     */
              char     m_dir[DIRLEN];  /*  name of directory       */
            } ;
