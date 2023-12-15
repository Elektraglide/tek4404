/* file modes and permissions expressed in 16 bits */

#define modes_h

/* file modes */
#define S_IFMT     0xCF      /* mask for type of file */
#define S_IFREG    0x01      /* normal file */
#define S_IFBLK    0x03      /* block special */
#define S_IFCHR    0x05      /* character special */
#define S_IFPTY    0x07      /* pseudo-terminal */
#define S_IFDIR    0x09      /* directory */
#define S_IFPIPE   0x41      /* pipe */
#define S_IFCON    0x81      /* contiguous file */
#define S_SLAVE_PTY  0x07    /* Slave pseudo-terminal device */
#define S_MASTER_PTY 0x87    /* Master pseudo-terminal device */

/* permissions */
#define  S_IPRM    0xFF      /* mask for permission bits */
#define  S_IREAD   0x01      /* owner read */
#define  S_IWRITE  0x02      /* owner write */
#define  S_IEXEC   0x04      /* owner execute */
#define  S_IOREAD  0x08      /* others read */
#define  S_IOWRITE 0x10      /* others write */
#define  S_IOEXEC  0x20      /* others execute */
#define  S_ISUID   0x40      /* set user id for execute */

