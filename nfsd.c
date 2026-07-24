#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

/*

 mount -v -t nfs -o proto=udp,vers=2 localhost:/Users/Shared  ~/flexdisk/

*/

#ifndef __clang__

#include <sys/sir.h>

#include <net/inet.h>
#include <net/nerrno.h>
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

#define  IPPROTO_UDP IPPR_UDP

struct sir sirbuf;

#else

//extern int open();
//extern int wait();
//extern int kill();

// clashes with Uniflex, so use MacOS constants
#define F_GETFL         3               /* get file status flags */
#define F_SETFL         4               /* set file status flags */
#define O_NONBLOCK      0x00000004      /* no delay */

#define in_sockaddr sockaddr_in

//#include "uniflexshim.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif

#define TRANSFER_SIZE 4096
#define BLOCK_SIZE 512
#define FDNPB 8

/* RPC for NFS constants */
enum msg_type {
 	CALL = 0,
 	REPLY = 1
};
/*
 * A reply to a call message can take on two forms: The message was
 * either accepted or rejected.
 */
enum reply_stat {
	MSG_ACCEPTED = 0,
	MSG_DENIED = 1
};

/*
 * Given that a call message was accepted, the following is the
 * status of an attempt to call a remote procedure.
 */
enum accept_stat {
 	SUCCESS = 0,       /* RPC executed successfully */
 	PROG_UNAVAIL = 1,  /* remote service hasn't exported prog */
 	PROG_MISMATCH = 2, /* remote service can't support versn # */
 	PROC_UNAVAIL = 3,  /* program can't support proc */
 	GARBAGE_ARGS = 4   /* procedure can't decode params */
};

enum NFSStatus
{
		NFS_OK				= 0,
		NFSERR_PERM			= 1,
		NFSERR_NOENT		= 2,
		NFSERR_IO			= 5,
		NFSERR_NXIO			= 6,
		NFSERR_ACCES		= 13,
		NFSERR_EXIST		= 17,
		NFSERR_NODEV		= 19,
		NFSERR_NOTDIR		= 20,
		NFSERR_ISDIR		= 21,
		NFSERR_FBIG			= 27,
		NFSERR_NOSPC		= 28,
		NFSERR_ROFS			= 30,
		NFSERR_NAMETOOLONG	= 63,
		NFSERR_NOTEMPTY		= 66,
		NFSERR_DQUOT		= 69,
		NFSERR_STALE		= 70,
		NFSERR_WFLUSG		= 99
};

enum ftype
{
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5
};

enum modes
{
	DIR_NFS = 16384,		/* clash with Uniflex DIR */
	CHR = 8192,
	BLK = 24576,
	REG = 32768,
	LNK = 40960,
	NON = 49152,
	SUID	= 2048,
	SGID	= 1024,
	SWAP	= 512,
	ROWN	= 256,
	WOWN	= 128,
	XOWN	= 64,
	RGRP	= 32,
	WGRP	= 16,
	XGRP	= 8,
	ROTH	= 4,
	WOTH	= 2,
	XOTH	= 1
};
		
struct rpcheader {
	unsigned int xid;
	unsigned int msg_type;
	unsigned int rpcvers;
	unsigned int prog;
	unsigned int vers;
	unsigned int proc;
};

struct filehandle {
	unsigned int index;
	unsigned int inode;
	unsigned int dev;
	unsigned char unused[20];
};

/* request and response state */
struct conn {
	int sock;
	struct in_sockaddr from;
	char buffer[4096];
	int crp,len;
};

/* needs to be dynamically resized? */
struct response {
	char buffer[4096];
	int crp;
};

/* well-known RPC prog names */
enum {
	PORTMAPPERD = 100000,
	NFSD = 100003,
	MOUNTD = 100005
} progs;

/* ports for RPC progs */
#define PORTMAPPERD_PORT 111
#define MOUNTD_PORT 635
#define NFSD_PORT 2049


FILE *console;

/* cache of file handle entries */
unsigned int filetablemask = 0;
char filetable[4][1024];

void addint(reply, val)
struct response *reply;
unsigned int val;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->crp);

	*ptr = htonl(val);
	reply->crp += sizeof(val);
}

void adddata(reply, data, len)
struct response *reply;
unsigned char *data;
int len;
{
	char *ptr = reply->buffer + reply->crp;
	
	if (reply->crp + len < 4096)
	{
		memcpy(ptr, data, len);		
		len = (len + 3) & -4;
		reply->crp += len;
	}
}

void addstat(reply, info)
struct response *reply;
struct stat *info;
{

	if ((info->st_mode & S_IFDIR) == S_IFDIR)
	{
		addint(reply, NFDIR);
		addint(reply, DIR_NFS | ROWN | WOWN | XOWN | ROTH);			/* info->st_perm */
		addint(reply, info->st_nlink);
		addint(reply, info->st_uid);
		addint(reply, info->st_uid);	/* no group */
		addint(reply, (unsigned int)info->st_size);
		addint(reply, BLOCK_SIZE);
		addint(reply, info->st_dev);
		addint(reply, FDNPB);
		addint(reply, 1);	/* fsid */
		addint(reply, (unsigned int)info->st_ino);
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
	}
	else
	{
		addint(reply, NFREG);
		addint(reply, REG | ROWN | WOWN | XOWN | ROTH);
		addint(reply, info->st_nlink);
		addint(reply, info->st_uid);
		addint(reply, info->st_uid);	/* no group */
		addint(reply, (unsigned int)info->st_size);
		addint(reply, BLOCK_SIZE);
		addint(reply, info->st_dev);
		addint(reply, ((unsigned int)info->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
		addint(reply, 1);	/* fsid */
		addint(reply, (unsigned int)info->st_ino);
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
		addint(reply, (unsigned int)info->st_mtime);
		addint(reply, 0);	/* usec */
	}
}

int validate(request, prognum)
struct conn *request;
int prognum;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;

	/* fprintf(console,"RPC: xid:%8.8x rpcvers:%d vers:%d prog:%d proc:%d\n", ntohl(header->xid), ntohl(header->rpcvers), ntohl(header->vers), ntohl(header->prog), ntohl(header->proc));
	*/

	reply.crp = 0;
	if (ntohl(header->msg_type) != CALL)
	{
		/* corrupted */
		addint(&reply, ntohl(header->xid));
		addint(&reply, REPLY);
		addint(&reply, MSG_ACCEPTED);
		addint(&reply, 0);		/* opaque_verf */
		addint(&reply, 0);		/* opaque_verf size */
		addint(&reply, GARBAGE_ARGS);
		sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	else
	if (ntohl(header->rpcvers) != 2)
	{
		/* not NFSv2 */
		addint(&reply, ntohl(header->xid));
		addint(&reply, REPLY);
		addint(&reply, MSG_DENIED);
		addint(&reply, PROG_MISMATCH);
		addint(&reply, 2);
		addint(&reply, 2);
		sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	else
	if (ntohl(header->prog) != prognum)
	{
		/* not correct service */
		addint(&reply, ntohl(header->xid));
		addint(&reply, REPLY);
		addint(&reply, MSG_ACCEPTED);
		addint(&reply, 0);		/* opaque_verf */
		addint(&reply, 0);		/* opaque_verf size */
		addint(&reply, PROG_MISMATCH);
		addint(&reply, prognum);
		addint(&reply, prognum);
		sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	
	request->crp += sizeof(struct rpcheader);
	return 1;
}

unsigned int getuint(request)
struct conn *request;
{
	unsigned int *ptr = (unsigned int *)(request->buffer + request->crp);
	unsigned int val = ntohl(*ptr);
	request->crp += sizeof(val);

	return val;
}

void verifier(request)
struct conn *request;
{
	unsigned int flavour = getuint(request);
	unsigned int length = getuint(request);
	request->crp += length;
}

void credentials(request)
struct conn *request;
{
	unsigned int flavour = getuint(request);
	unsigned int length = getuint(request);

	/* log AUTH_UNIX */
	if (flavour == 1)
	{
		unsigned int *ptr = (unsigned int *)(request->buffer + request->crp);
		/* fprintf(console, "credentials: AUTH_UNIX: %8.8x: %s\n", ptr[0], ptr+1); */
	}
	
	request->crp += length;
}

struct filehandle *getfilehandle(request)
struct conn *request;
{
	struct filehandle *ptr = (struct filehandle *)(request->buffer + request->crp);
	fprintf(console, "  getfilehandle: index=%d inode=%d\n", ptr->index, ptr->inode);
	request->crp += sizeof(struct filehandle);
	
	return ptr;
}


char *getstring(request)
struct conn *request;
{
	static char name[1024];
	unsigned int len = getuint(request);

	memcpy(name, request->buffer + request->crp, len);
	len = (len + 3) & -4;
	request->crp += len;
	
	return name;
}

						
int makehandle(path, handle, modefilter)
char *path;
struct filehandle *handle;
unsigned int modefilter;
{
	struct stat info;
	int n;
	
	if (stat(path, &info) == 0 && ((info.st_mode & modefilter) == modefilter))
	{
		for (n=0; n<4; n++)
		{
			/* already got it */
			if ((filetablemask & (1<<n)))
			{
				if (!strcmp(filetable[n], path))
				{
					fprintf(console, "  makehandle: found at slot%d\n",n);
					break;
				}
			}
			else
			/* find a free slot */
			{
				strcpy(filetable[n], path);
				filetablemask |= (1<<n);
				break;
			}
		}

		if (n < 4)
		{
			/* make a file handle */
			memset(handle, 0, sizeof(struct filehandle));
			handle->index = n;
			handle->inode = info.st_ino;
			handle->dev = info.st_dev;

			return n;
		}
		else
		{
			/* TODO: not cached, so lookup using inode */
			return -1;
		}
	}
	else
	{
		return -errno;
	}
}

int createudpsock(port)
int port;
{
	struct in_sockaddr serv_addr;
	int sock,n;
		
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		fprintf(console, "socket: %s\n",strerror(errno));
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	n = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
	if (n < 0) {
		fprintf(console, "bind: %s\n",strerror(errno));
		close(sock);
		return -2;
	}
	fprintf(console,"listen on %d\n", port);

	return sock;
}

void portmapperprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	unsigned int prog, vers, prot, port, registeredport;
	int n;
	
	credentials(request);
	verifier(request);
	
	reply.crp = 0;
	addint(&reply, ntohl(header->xid));
	addint(&reply, REPLY);
	addint(&reply, MSG_ACCEPTED);
	addint(&reply, 0);		/* opaque_verf */
	addint(&reply, 0);		/* opaque_verf size */

	switch(ntohl(header->proc))
	{
		default:
		case 0:
			addint(&reply, SUCCESS);
			break;
		case 3:
			/* GetPort */
			prog = getuint(request);
			vers = getuint(request);
			prot = getuint(request);
			port = getuint(request);
			registeredport = 0;
			if (prog == NFSD && prot == IPPROTO_UDP) registeredport = 2049;
			if (prog == MOUNTD && prot == IPPROTO_UDP) registeredport = 635;
			if (registeredport)
			{
				addint(&reply, SUCCESS);
				addint(&reply, registeredport);
			}
			else
			{
				addint(&reply, PROG_UNAVAIL);
			}
			fprintf(console, "portmapd: prog:%d vers:%d prot:%d => registeredport:%d\n", prog, vers, prot, registeredport);
			break;
			
	}

	n = sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.crp)
	{
			fprintf(console, "portmapd: sendto: %s\n",strerror(errno));
	}
}

void mountprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info;
	char *path;
	struct filehandle handle;
	int n;
	
	credentials(request);
	verifier(request);
	
	reply.crp = 0;
	addint(&reply, ntohl(header->xid));
	addint(&reply, REPLY);
	addint(&reply, MSG_ACCEPTED);
	addint(&reply, 0);		/* opaque_verf */
	addint(&reply, 0);		/* opaque_verf size */
	addint(&reply, SUCCESS);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* Add Mount */
			path = getstring(request);
			if (makehandle(path, &handle, S_IFDIR) >= 0)
			{
				addint(&reply, NFS_OK);
				adddata(&reply, (unsigned char *)&handle, sizeof(handle));
				fprintf(console, "mountd: mount Path = %s\n",path);
			}
			else
			{
				addint(&reply, NFSERR_NOENT);	/* no such file */
				addint(&reply, 0);
			}
			break;
		case 2:
			/* Mount Entries */
			break;
		case 3:
			/* Remove Mount */
			path = getstring(request);
			fprintf(console, "mountd: unmount Path = %s\n",path);
			
			addint(&reply, NFS_OK);
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.crp)
	{
			fprintf(console, "mountd: sendto: %s\n",strerror(errno));
	}
}

void nfsprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info;
	char *path;
	char filepath[1024];
	int disksize,freesize;
	int fh, n, count;

	struct filehandle handle;
	struct filehandle *fh;
	credentials(request);
	verifier(request);
	
	reply.crp = 0;
	addint(&reply, ntohl(header->xid));
	addint(&reply, REPLY);
	addint(&reply, MSG_ACCEPTED);
	addint(&reply, 0);		/* opaque_verf */
	addint(&reply, 0);		/* opaque_verf size */
	addint(&reply, SUCCESS);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* GetAttr */
			fh = getfilehandle(request);
			fprintf(console, "nfsd: getattr: %s\n", filetable[fh->index]);
			if (stat(filetable[fh->index], &info) == 0)
			{
				addint(&reply, NFS_OK);
				addstat(&reply, &info);
			}
			else
			{
				/* this should never happen.. */
				addint(&reply, NFSERR_NOENT);
			}
			break;
		case 2:
			/* SetAttr */
			break;
		case 3:
			/* Root(). No-op. */
			break;
		case 4:
			/* Lookup */
			fh = getfilehandle(request);
			path = getstring(request);
			strcpy(filepath, filetable[fh->index]);
			strcat(filepath, "/");
			strcat(filepath, path);
			fprintf(console, "nfsd: lookup = %s\n", filepath);
			if (makehandle(filepath, &handle, 0) >= 0)
			{
				addint(&reply, SUCCESS);
				adddata(&reply, (unsigned char *)&handle, sizeof(handle));
			}
			else
			{
				addint(&reply, 2);	/* no such file */
				addint(&reply, 0);
			}
			break;
		case 5:
			/* ReadLink */
			break;
		case 6:
			/* Read */
			break;
		case 8:
			/* Write */
			break;
		case 9:
			/* Create */
			break;
		case 10:
			/* Remove */
			break;
		case 11:
			/* Rename */
			break;
		case 13:
			/* SymLink */
			break;
		case 14:
			/* MkDir */
			break;
		case 15:
			/* RmDir */
			break;
		case 16:
			/* ReadDir */
			fh = getfilehandle(request);
			n = getuint(request);
			count = getuint(request);
			
			fprintf(console, "nfsd: readdir: count = %s\n", count);
			break;
		case 17:
			/* StatFS */
			addint(&reply, NFS_OK);
			addint(&reply, TRANSFER_SIZE);			/* tsize: optimum transfer size */
			addint(&reply, BLOCK_SIZE);			/* Block size of FS */
#ifdef __clang__
			/* fake some numbers */
			disksize = 40 * 1024 * 1024 / BLOCK_SIZE;
			freesize = 10 * 1024 * 1024 / BLOCK_SIZE;
#else
			n = open(devname, O_RDONLY);
			lseek(n, BLOCK_SIZE, SEEK_SET);
			read(n, &sirbuf, sizeof(sirbuf));
			close(n);
			disksize = (sirbuf.ssizfr[0] << 16) + (sirbuf.ssizfr[1] << 8) + (sirbuf.ssizfr[2] << 0);
			freesize = (sirbuf.sfreec[0] << 16) + (sirbuf.sfreec[1] << 8) + (sirbuf.sfreec[2] << 0);
#endif
			addint(&reply, disksize);					/* Total # of blocks (of the above size) */
			addint(&reply, freesize);					/* Free blocks */
			addint(&reply, freesize);					/* Free blocks available to non-priv. users */
			fprintf(console, "nfsd: statfs: %d blocks (%d free)\n", disksize, freesize);
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.crp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.crp)
	{
			fprintf(console, "nfsd: sendto: %s\n",strerror(errno));
	}
	
}


int main(argc, argv)
int argc;
char **argv;
{
	int portmapsock, mountsock, nfssock;
	int n;
	
	console = stderr;
	
	/* we act as portmapd, mountd and nfsd... */
	portmapsock = createudpsock(PORTMAPPERD_PORT);
	mountsock = createudpsock(MOUNTD_PORT);
	nfssock = createudpsock(NFSD_PORT);

	/* run loop */
	while(1)
	{
		struct conn request;
		fd_set fd_in;
		size_t fromSize = sizeof(request.from);
		int n,count;

		FD_ZERO(&fd_in);
		n = 0;
		
		FD_SET(portmapsock, &fd_in);
		if (portmapsock > n)
			n = portmapsock;
		FD_SET(mountsock, &fd_in);
		if (mountsock > n)
			n = mountsock;
		FD_SET(nfssock, &fd_in);
		if (nfssock > n)
			n = nfssock;

		request.crp = 0;

		n = select(n + 1, &fd_in, NULL, NULL, NULL);
		if (n < 0)
		{
			if (errno != EINTR)
				break;
			
			continue;
		}
		else
		if (FD_ISSET(portmapsock, &fd_in))
		{
			request.sock = portmapsock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				/* validate */
				if (validate(&request, PORTMAPPERD))
				{
					portmapperprog(&request);
				}
			}
		}
		else
		if (FD_ISSET(mountsock, &fd_in))
		{
			request.sock = mountsock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				/* validate */
				if (validate(&request, MOUNTD))
				{
					mountprog(&request);
				}
			}
		}
		else
		if (FD_ISSET(nfssock, &fd_in))
		{
			request.sock = nfssock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				/* validate */
				if (validate(&request, NFSD))
				{
					nfsprog(&request);
				}
			}
		}
	}
	
}
	
