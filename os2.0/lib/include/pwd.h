/*
     pwd.h - Password-file definitions

          This file describes the structure used
          to contain the information found in an
          entry in the password file, on Tektronix 4400:

                    /etc/log/password

     Fields:
          pw_name   char *
                    Pointer to a null-terminated char string,
                    the login name
          pw_passwd char *
                    Pointer to a null-terminated char string,
                    encrypted password or null-string if none
          pw_uid    int
                    The user's user identifying number (user id)
          pw_dir    char *
                    Pointer to a null-terminated char string,
                    the user's initial directory (also the
                    initial home directory)
          pw_shell  char *
                    Pointer to a null-terminated char string,
                    the initial program to run for the user,
                    or the shell if a null-string


          Functions which require this structure:

               endpwent()     Close password-file
               getpwent()     Get next password-file entry
               getpwnam()     Get entry keying on name
               getpwuid()     Get entry keying on user id
               putpwent()     Write password-file entry info
               setpwent()     Rewind password-file
*/

#define   pwd_h
#define   PWFILNAM  "/etc/log/password"
#define   PWRECSZ   132


struct passwd {
     char     *pw_name;       /* user name */
     char     *pw_passwd;     /* encrypted password */
     int       pw_uid;        /* user id */
     char     *pw_dir;        /* home directory */
     char     *pw_shell;      /* log-in program */
};


/*
     Function definitions:
*/

struct passwd *getpwent();
struct passwd *getpwnam();
struct passwd *getpwuid();
         void  setpwent();
         void  endpwent();
          int  putpwent();

#ifndef NULL
#define NULL   0
#endif
