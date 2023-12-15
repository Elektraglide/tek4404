/*
     fcntl.h - File control interface
*/


#ifndef fcntl_h
#define fcntl_h
#endif

/* File control sub-functions */

#define FGETPARAMS 0  /* Get current parameter "mask" */
#define FNOBLOCK   3  /* Don't block reads */
#define FBLOCK     4  /* Block read requests which can't be satisfied */

/* Parameter "mask" */

#define F_NOBLOCK  1  /* Reads will not block */

/*
     Open modes:  [known to "open()"]

          O_RDONLY       Open for reading only
          O_WRONLY       Open for writing only
          O_RDWR         Open for read/write
*/

#define   O_RDONLY       0	/* read only */
#define   O_WRONLY       1	/* write only */
#define   O_RDWR         2	/* read and write */
