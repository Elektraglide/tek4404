#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <util.h>
// #include <sys/sgtty.h> filename clashes with Tek4404 header so we need to include its contents
#include <sys/cdefs.h>
#define	USE_OLD_TTY
#define _SGTTYB_
#include <sys/ioctl.h>
#define	gtty(fd, buf)	ioctl(fd, TIOCGETP, buf)
#define	stty(fd, buf)	ioctl(fd, TIOCSETP, buf)

// unix kernel calls without stdlib.h
extern void *malloc();
extern void exit();

// implementations of Uniflex calls
int create_pty(int *ptfd)
{

	// NB master,slave is reverse of slave,master on Tek
	return openpty(&ptfd[1], &ptfd[0], NULL, NULL, NULL);;
}

int control_pty(int fd, int code, int cval)
{
	if (code ==  PTY_INQUIRY)
	{
		return 0;
    }

	if (code == PTY_SET_MODE)
    {
		fprintf(stderr, "control_pty: PTY_SET_MODE: unimplemented\n");
#if 0
		if (cval & PTY_REMOTE_MODE)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_READ_WAIT)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_HANDSHAKE_MODE)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_SLAVE_HOLD)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_EOF)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_OUTPUT_QUEUED)
			ioctl_tty(fd, TIOCGPKT, 0);

		if (cval & PTY_INPUT_QUEUED)
			ioctl_tty(fd, TIOCGPKT, 0);
#endif
	    return(0);
    }

	if (code == PTY_START_OUTPUT)
    {
		ioctl(fd, TIOCPKT, TIOCPKT_START);
	    return(0);
    }

	if (code == PTY_STOP_OUTPUT)
    {
		ioctl(fd, TIOCPKT , TIOCPKT_STOP);
	    return(0);
    }

	if (code == PTY_FLUSH_READ)
    {
		ioctl(fd, TIOCPKT , TIOCPKT_FLUSHREAD);
	    return(0);
    }

	if (code == PTY_FLUSH_WRITE)
    {
		ioctl(fd, TIOCPKT , TIOCPKT_FLUSHWRITE);
	    return(0);
    }

    return 0;
}

char *phys(int code)
{
	switch(code)
    {
      // framebuffer memory
      case 1:
        return 0;
      case -1:
        return 0;

      default:
        fprintf(stderr, "phys:  unknown resource %d\n", code);
        return 0;
    }

}
