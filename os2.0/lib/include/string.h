/*
     Definitions of the string functions available
     in the UniFLEX standard "C" library
*/

#define string_h

     char *strcat();          /* Concatenation */
     char *strncat();         /* Concatenation with count */

     int   strcmp();          /* Compare strings */
     int   strncmp();         /* Compare strings with count */

     int   strcmpci();        /* Compare strings (case insensitive) */
     int   strncmpci();       /* Compare strings w/ count (case insensitive) */

     char *strcpy();          /* Copy strings */
     char *strncpy();         /* Copy strings with count */

     int   strlen();          /* Determine string length */

     char *strchr();          /* First occurrence of char */
     char *strrchr();         /* Last occurrence of char */

     char *strpbrk();         /* First occurrence of group */

     int   strspn();          /* Length of string in group */
     int   strcspn();         /* Length of string not in group */

     char *strtok();          /* Extract next token */

     char *strstr();          /* Find a string within a string */
     char *strstrci();        /* Case insensitive "strstr()" */

#ifndef   NULL
#define   NULL      0
#endif
