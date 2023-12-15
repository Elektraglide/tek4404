/* Tektronix 4404 Operating System Interrupts */

#define signal_h

#define		SIGHUP    1     /* hangup */
#define		SIGINT    2     /* keyboard */
#define		SIGQUIT   3     /* quit */
#define		SIGEMT    4     /* EMT $Axxx emulation */
#define		SIGKILL   5     /* task kill (cannot be caught or ignored)*/
#define		SIGPIPE   6     /* broken pipe */
#define		SIGSWAP   7     /* swapping error */
#define		SIGTRACE  8     /* trace */
#define		SIGTIME   9     /* time limit */
#define		SIGALRM  10     /* alarm */
#define		SIGTERM  11     /* task terminate */
#define		SIGTRAPV 12     /* TRAPV instruction */
#define		SIGCHK   13     /* CHK instruction */
#define		SIGEMT2  14     /* EMT $Fxxx emulation */
#define		SIGTRAP1 15     /* TRAP #1 instruction */
#define		SIGTRAP2 16     /* TRAP #2 instruction */
#define		SIGTRAP3 17     /* TRAP #3 instruction */
#define		SIGTRAP4 18     /* TRAP #4 instruction */
#define		SIGTRAP5 19     /* TRAP #5 instruction */
#define		SIGTRAP6 20     /* TRAP #6 instruction */
#define		SIGPAR   21     /* Parity error */
#define		SIGILL   22     /* Illegal instruction */
#define		SIGDIV   23     /* DIVIDE by 0 */
#define		SIGPRIV  24     /* Priveleged instruction */
#define		SIGADDR  25     /* Address error */
#define		SIGDEAD  26     /* Dead child */
#define		SIGWRIT  27     /* Write to READ-ONLY memory */
#define		SIGEXEC  28     /* Execute from STACK/DATA space */
#define		SIGBND   29     /* Segmentation violation */

#define		SIGUSR1  30     /* User defined interrupt #1 */
#define		SIGUSR2  31     /* User defined interrupt #2 */
#define		SIGUSR3  32     /* User defined interrupt #3 */

#define		SIGABORT 33     /* Program abort */
#define		SIGSPLR  34     /* Spooler interrupt */
#define		SIGINPUT 35     /* Input is ready */
#define		SIGDUMP  36     /* Memory dump */

#define		SIGRFAULT 49	/* page monitoring READ fault */
#define		SIGWFAULT 50	/* page monitoring WRITE fault */

#define		SIGMILLI 62     /* Millisecond alarm */
#define		SIGEVT   63     /* Mouse/keyboard event interupt */

/* special addresses */
#define		SIG_DFL ((int (*)()) 0)
#define		SIG_IGN ((int (*)()) 1)

#define		NSIG     63

/*
     Define the "signal()" function
     (a function returning a pointer to a function returning an int)
*/
     int     (*signal())();
