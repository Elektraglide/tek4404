
#define stat_h

#ifndef modes_h
#include <sys/modes.h>
#endif

struct stat    /* structure returned by stat */
{
       short   st_dev ;  /* device number */
       short   st_ino ;  /* fdn number */
       char    st_filler;
       char    st_mode;  /* file mode (type) */
       char    st_perm;  /* file access permissions */
       char    st_nlink; /* file link count */
       short   st_uid ;  /* file owner's user id */
       long    st_size;  /* file size in bytes */
       long    st_mtime; /* last modified time */
       long    st_spr ;  /* spare - future use only */
};
