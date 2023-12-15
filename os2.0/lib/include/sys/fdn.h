
/*  file descriptor node structure definition  */

struct inode { char     fd_mod;         /* file mode - see see modes.h */
               char     fd_prm;         /* permission bits */
               char     fd_cnt;         /* link count */
               char     fd_own[2];      /* file owners id */
               char     fd_siz[4];      /* file size in bytes */
               char     fd_blk[13][3];  /* block list */
               long     fd_mtm;         /* last time modified */
               char     fd_pad[12];     /* padding */
             };

#define        FDNPB   8       /* 8 fdns per block */

