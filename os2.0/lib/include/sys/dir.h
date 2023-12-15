/*******************************************************************/
/*                                                                 */
/*                         D I R . H                               */
/*                                                                 */
/*******************************************************************/

#define dir_h

#define DIR_MAX_SLOTS 4 /* maximum number of slots for one entry */
#define DIR_SLOT_SIZE 16   /* size of one slot */
#define DIR_CHARS_PER_SLOT (DIR_SLOT_SIZE-sizeof(short)) /* chars per slot */
#define MAXNAMLEN (DIR_CHARS_PER_SLOT*DIR_MAX_SLOTS-1)  /* max name size */
#define DIRBUFSIZE (32*DIR_SLOT_SIZE) /* buffer size */
#ifndef NULL
#define NULL 0 /* the null pointer */
#endif

/*
   Definition of the structures used for manipulating directories.
*/

struct direct {         /* A single directory entry */
   short d_ino;         /* fdn number */
   short d_namlen;      /* length of the name (not including null) */
   char d_name[MAXNAMLEN+1];  /* the name of the entry */
};

struct _dirdesc {       /* directory information structure */
   int dd_fd;           /* file descriptor */
   long dd_loc;         /* current location */
   long dd_size;        /* size of the directory */
   char dd_buf[DIRBUFSIZE];   /* buffer */
   char *dd_first;      /* buffer "first" address */
   char *dd_limit;      /* buffer "in" pointer */
   char *dd_out;        /* buffer "out" pointer */
   struct direct dd_entry; /* the current entry */
};

typedef struct _dirdesc DIR;  /* directory information structure */

/*
   Routine definitions:
*/
     DIR           *opendir();
     struct direct *readdir();
     long           telldir();
     void           seekdir();
     void           rewinddir();
     void           closedir();
