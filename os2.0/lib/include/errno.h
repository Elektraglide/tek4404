/* UniFLEX system call error numbers.
 * may be found in "errno" after an error has occurred.
 */

#define errno_h

#define EIO 1
#define EFAULT 2
#define ENOMEM 3
#define ENOTDIR 4
#define ENOSPC 5
#define EMFILE 6
#define EBADF 7
#define ENOENT 8
#define EMSDR 9
#define EACCES 10
#define EEXIST 11
#define EINVAL 12
#define ESEEK 13
#define EXDEV 14
#define ENOTBLK 15
#define EBUSY 16
#define ENMNT 17
#define EBDEV 18
#define E2BIG 19
#define EISDR 20
#define ENOEXEC 21
#define EBBIG 22
#define ESTOF 23
#define ECHILD 24
#define EAGAIN 25
#define EBDCL 26
#define EINTR 27
#define ESRCH 28
#define ENOTTY 29
#define EPIPE 30
#define ELOCK 31
#define ETXOF 32
#define EVFORK 33
#define EDIRTY 34
#define EWRTPROT 35
#define ENOFPUDATA 36
#define ENOINPUT 37
#define	ENOSPACE 38

#define ERUNBASE 64


/*
   Error codes for the UniFLEX C Runtime system
   (also found in "errno")
*/

/*
    Floating-point runtime errors:

    EDOM       A floating-point function was given an argument
               that is outside the domain of the function
    ERANGE     A result of a floating-point function can't be
               represented by a floating-point value
    EOVFLO     The result of a floating-point function is
               larger than can be represented by the floating-
               point format
    EUNDFLO    The result of a floating-point function is
               smaller than can be represented by the floating-
               point format
    ETLOSS     A floating-point argument lost all significance
               during an operation
    EPLOSS     A floating-point argument lost some significance
               during an operation (currently not generated)
    EZDIV      The divisor of a division operation is zero
*/

#define   EDOM      (ERUNBASE+0)
#define   ERANGE    (ERUNBASE+1)
#define   EOVFLO    (ERUNBASE+2)
#define   EUNDFLO   (ERUNBASE+3)
#define   ETLOSS    (ERUNBASE+4)
#define   EPLOSS    (ERUNBASE+5)
#define   EZDIV     (ERUNBASE+6)


/*
    errno    The external variable containing the error number
             of the last error that occurred
*/
extern    int       errno;
