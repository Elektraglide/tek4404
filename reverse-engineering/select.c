#include <net/netioc.h>
#include <net/select.h>

int setsockopt(fd, type, cmd, param, size)
int fd;
int type;
int cmd;
char *param;
int size;
{
	struct el cmds[EL_MAX_SIZE];
    struct el *elp;

	elp = _el_init(cmds, ELC_SETOPT,  0);

	elp = _elc(cmds, EL_P0,  4,  F_EL_STRUCTURED);
	elp->e1_u1 = type;

	elp = _elc(cmds, EL_P1,  4,  F_EL_STRUCTURED);
	elp->e1_u1 = cmd;

	elp = _elc(cmds, EL_DATA,  size,  F_EL_IN);
	elp->e1_u1 = param;

	_net_ioctl(fd, cmds);
}



struct el *_elc_init(elp, cmd, flags)
struct el *elp;
int cmd;
int flags;
{

	elp->el_type = EL_HEADER;
    elp->el_flags = (short)flags;
	elp->el_u1 = elp + 1;
	elp->el_u2 = cmd;
	return elp;
}

struct el *_elc(elp, type, size, flags)
struct el *elp;
int type, size, flags;
{
	struct el *next;

    /* update to next one */
	next = elp->el_u1;
	elp->el_u1 = next + 1;

	next->el_type = (short)type;
	next->el_len = size;
    next->el_flags = (short)flags;

    return next;
}

int _net_ioctl(fd, cmds)
int fd;
struct el *cmds;
{
	int n;

	n = (struct el *)cmds->e1_u1 - cmds;

	/ * WHAT IS THIS MAGIC? */
	if (write(fd, cmds, n + 0x00007ff4) >= 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int nselect(set, count, timeout)
struct sel *set;
int count;
int timeout;
{
	struct el cmds[EL_MAX_SIZE];
    struct el *elp;
	int timeout_flag;
	int lenresult;
	int fd;

	if (count > 0)
    {
		timeout_flag = timeout ? 0x8000 : 0x0000;

		elp = _el_init(cmds, ELC_NSELECT,  timeout_flag);

		elp = _elc(cmds, EL_DATA, count * 8,  F_EL_IN | F_EL_OUT);
		elp->e1_u1 = set;

		elp = _elc(cmds, EL_D_LEN,  4,  F_EL_STRUCTURED | F_EL_OUT);
		elp->e1_u1 =  &lenresult;

		elp = _elc(cmds, EL_P0,  4,  F_EL_STRUCTURED);
		elp->e1_u1 = timout;

		fd = set->se_fd;
		if (_net_ioctl(fd, cmds))
        {
			return lenresult;
        }
    }

	return 0;
}

struct sel smallset[32];

int select(nfds, fdread, fdwrite, fdexcep, timeout)
int nfds;
int *fdread;
int *fdwrite;
int * fdexcep;
struct timeval *timeout;
{
	struct sel *set = smallset;
	struct sel *setptr;
	int mstimeval;
	int rc,n,i, mask;

	if (nfds < 0)
	{
		errno = 12;
		return -1;
	}

	if (nfds > 32)
    {
			set = (struct sel *)calloc(nfds, sizeof(struct sel));		
			if (!set)
			{
				errno = 12;
				return -1;
			}
	}

	if (timeout)
	{
        /* convert to (kind of) milleseconds */
		mstimeval = (timeout->tv_sec << 10) + (timeout->tv_usec >> 10);
	}
	else
	{
		mstimeval = 0xffffffff;
	}

	setptr = set;
	for(i=0;	i<nfds;  i++, setptr++)
	{
		if (fdread)
		{
			mask = 1 << (i & 31);

			if (fdread[i/32] & mask)
			{
				setptr->se_flags |= F_SE_READ;
			}
		}

		if (fdwrite)
		{
			mask = 1 << (i & 31);

			if (fdwrite[i/32] & mask)
			{
				setptr->se_flags |= F_SE_WRITE;
			}
		}

		if (fdexcep)
		{
			mask = 1 << (i & 31);

			if (fdexcep[i/32] & mask)
			{
				setptr->se_flags |= F_SE_OTHER;
			}
		}
	}

	n = setptr - set;
	rc = nselect(set,  n, mstimeval);
	if (rc >= 0)
	{
		while(setptr-- > set)
		{
			if (fdread)
			{
				if (setptr->se_flags & F_SE_READ)
				{
					mask = 1 << (setptr->se_fd & 31);
					fdread[setptr->se_fd/32] &= ~mask;
				}
			}
			if (fdwrite)
			{
				if (setptr->se_flags & F_SE_READ)
				{
					mask = 1 << (setptr->se_fd & 31);
					fdwrite[setptr->se_fd/32] &= ~mask;
				}
			}
			if (fdexcep)
			{
				if (setptr->se_flags & F_SE_READ)
				{
					mask = 1 << (setptr->se_fd & 31);
					fdexcep[setptr->se_fd/32] &= ~mask;
				}
			}
		}
	}

	if (set != smallset)
	{
		free(set);
	}
	
	return rc;
}
