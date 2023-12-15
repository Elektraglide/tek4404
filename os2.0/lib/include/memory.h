/*
     memory.h - Define memory manipulation routines

     char *memccpy(p, q, c, n)
          char     *p;
          char     *q;
          int       c;
          int       n;

          Copies characters from the buffer referenced by <q>
          into the buffer referenced by <p>, stopping after the
          character <c> is copied or the count <n> is exhausted,
          whichever comes first.  Returns a pointer to the byte
          immediately following the copy of <c> in <p> or NULL
          if the operation terminated by exhausting the count <n>.


     char *memchr(s, c, n)
          char     *s;
          int       c;
          int       n;

          Returns a pointer to the first occurrence of the
          character <c> in the first <n> characters of the
          buffer referenced by <n>, or NULL if none.


     int memcmp(p, q, n)
          char     *p;
          char     *q;
          int       n;

          Compares the first <n> characters of the buffers
          and referenced by <p> and <q> and returns an integer
          less than, equal to, or greater than zero indicating
          the buffer referenced by <p> is lexicographically
          less than, equal to, or greater than the buffer
          referenced by <q>.


     char *memcpy(p, q, n)
          char     *p;
          char     *q;
          int       n;

          Copies <n> bytes in the buffer referenced by <q>
          into the buffer referenced by <p>.  It returns <p>.


     char *memset(p, c, n)
          char     *p;
          int       c;
          int       n;

          Sets the first <n> characters of the buffer referenced
          by <p> to the value <c>.  It returns <p>.
*/

#define memory_h

     char     *memccpy();

     char     *memchr();

     int       memcmp();

     char     *memcpy();

     char     *memset();
