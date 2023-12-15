/*
     sgtty.h - Define tty information

          This file defines information needed by the
          "stty()" and "gtty()" calls.
*/

#ifndef sgtty_h
#define sgtty_h

/*
     This is the structure of the data returned by "gtty()"
     and of the data expected by "stty()".
*/
struct sgttyb
{
     unsigned char  sg_flag;       /* mode flag, see below */
     unsigned char  sg_delay;      /* delay type, see below */
     unsigned char  sg_kill;       /* line delete char, ^U default */
     unsigned char  sg_erase;      /* backspace char, ^H default */
     unsigned char  sg_speed;      /* terminal speed, unused */
     unsigned char  sg_prot;       /* protocol type, see below */
};

#ifndef   NULL
#define   NULL      0
#endif

/*
 * High order bit of "sg_speed" indicates "input is ready to be consumed".
 */
#define INCHR 0x80

/* terminal modes */
#define   RAW       0x01 /* raw mode */
#define   ECHO      0x02 /* input character echo */
#define   XTABS     0x04 /* expand tabs on output */
#define   LCASE     0x08 /* map upper- to lower-case on input */
#define   CRMOD     0x10 /* write <cr><lf> for <cr> */
#define   SCOPE     0x20 /* echo <bs> echo char */
#define   CBREAK    0x40 /* single char mode */
#define   CNTRL     0x80 /* ignore control characters */

/*
 * Protocol byte definitions.
 */
#define   ESC       0x80 /* escape start-stop */
#define   OXON      0x40 /* output xon-xoff protocol */
#define   ANY       0x20 /* any character to start for xon */
#define   TRANS     0x10 /* transparent mode for raw w/ xon-xoff */
#define   IXON      0x08 /* input xon-xoff protocol */
#define   BAUD_RATE 0x0F /* baud rate mask */


/* delay types */

#define   DELNL     0x03 /* new line */
#define   DELCR     0x0C /* carriage-return */
#define   DELTB     0x10 /* tab */
#define   DELVT     0x20 /* vertical tab */
#define   DELFF     0x20 /* form-feed (as DELVT) */
#endif
