#define stdio_h
#define   BUFSIZ 4096
#define   _NFILE   32
struct _iobuf {
     char     *_ptr;          /* pointer into buffer */
     int       _cnt;          /* count of unused buffer */
     char     *_base;         /* buffer base address */
     int       _flag;         /* file status */
     int       _fd;           /* file descriptor */
     int       _save;         /* for 'ungetc' */
     int       _task_id;      /* task ID when used for a pipe */
} ;

typedef struct _iobuf FILE ;

extern FILE _iob[_NFILE];

/*
     Definition of the bits in the "_flag" field of
     the "iobuf" structure defined above

          _READ      Stream open for read access
          _WRITE     Stream open for write access
          _APPEND    Stream open for append (write at EOF)
                     (_WRITE is also set when a stream is
                      opened for append.)
                     (absence of _READ and _WRITE indicates
                     that the stream has not been opened)
          _UPDATE    Stream open for both read and write access
                     (_READ and _WRITE are set when a stream is
                      opened for update)
          _PIPE      Stream opened as a pipe
          _UNBUF     Stream is unbuffered
                     (absence indicates buffered I/O)
          _BIGBUF    Indicates that _fillbuf() or _flushbuf()
                     has automatically allocated a buffer
                     for the stream's buffered I/O
          _EOF       Indicates that the end of the stream
                     has been detected
          _ERR       Indicates that an error has been detected
                     while performing I/O on the stream
          _FLEOL     Indicates that the output stream is to
                     be flushed whenever an EOL character is
                     written through standard I/O [except
                     through fwrite()]
          _LASTREAD  A read operation was the last operation
                     performed on this stream.
          _LASTWRITE A write operation was the last operation
                     performed on this stream.
*/

#define _READ       0x0001
#define _WRITE      0x0002
#define _UNBUF      0x0004
#define _BIGBUF     0x0008
#define _EOF        0x0010
#define _ERR        0x0020
#define _FLEOL      0x0040
#define _PIPE       0x0080
#define _APPEND     0x0100
#define _UPDATE     0x0200
#define _LASTWRITE  0x0400
#define _LASTREAD   0x0800

#define EOF         (-1)
#ifndef EOL
#define EOL         '\n'
#endif

#ifndef NULL
#define NULL        0
#endif

#ifndef TRUE
#define TRUE        't'
#endif

#ifndef FALSE
#define FALSE       0
#endif

#define stdin       _iob
#define stdout      (&_iob[1])
#define stderr      (&_iob[2])

extern  int         errno;

#define PMODE       0x000b   /* r/w for owner, r for others */

#define putc(c,p)     fputc(c,p)
#define getc(s)       fgetc(s)
#define putchar(c)    putc(c,stdout)
#define getchar()     getc(stdin)
#define ferror(p)     ((p)->_flag&_ERR)
#define feof(p)       ((p)->_flag&_EOF)
#define clearerr(p)   ((p)->_flag&=~(_ERR|_EOF))
#define fileno(p)     ((p)->_fd)

/*
     Standard I/O Function declarations
*/

     int       fclose();
     FILE     *fdopen();
     int       fflush();
     int       fgetc();
     char     *fgets();
     FILE     *fopen();
     int       fputc();
     int       fputs();
     int       fread();
     FILE     *freopen();
     int       fseek();
     long      ftell();
     int       fwrite();
     char     *gets();
     int       getw();
     int       puts();
     int       putw();
     void      rewind();
     void      setbuf();

#define L_cuserid 9
#define L_ctermid 13
