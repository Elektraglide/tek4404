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

#define FDNPB 8

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

struct conn {
	int sock;
	struct in_sockaddr from;
	char buffer[4096];
	int crp,len;
};

struct rpcheader {
	unsigned int xid;
	unsigned int msg_type;
	unsigned int rpcvers;
	unsigned int prog;
	unsigned int vers;
	unsigned int proc;
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
	
FILE *console;

/* mount table */
unsigned int mountmask = 0;
char mounttable[4][1024];

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


int validate(request, prognum)
struct conn *request;
int prognum;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;

	fprintf(console,"RPC: xid:%8.8x rpcvers:%d vers:%d prog:%d proc:%d\n", ntohl(header->xid), ntohl(header->rpcvers), ntohl(header->vers), ntohl(header->prog), ntohl(header->proc));

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

void skiplump(request)
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

int getfilehandle(request)
struct conn *request;
{
	unsigned char *ptr = (request->buffer + request->crp);
	fprintf(console, "handle=%d dev=%d\n", ptr[0], ptr[1]);
	request->crp += 32;
	
	return ptr[0];
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

						
int makehandle(path, handle)
char *path;
char *handle;
{
	struct stat info;
	int n;
	
	if (stat(path, &info) == 0 && ((info.st_mode & S_IFDIR) == S_IFDIR))
	{
		for (n=0; n<4; n++)
		{
			/* already got it */
			if ((mountmask & (1<<n)))
			{
				if (!strcmp(mounttable[n], path))
				{
					fprintf(console, "makehandle: already have\n");
					break;
				}
			}
			else
			/* find a free slot */
			{
				strcpy(mounttable[n], path);
				mountmask |= (1<<n);
				break;
			}
		}

		/* make a file handle */
		memset(handle, 0, 32);
		handle[0] = n;
		handle[1] = info.st_dev;
		handle[2] = info.st_ino;

		return 0;
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
	
	/* verifier */
	skiplump(request);
	
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
			if (prog == NFSD) registeredport = 2049;
			if (prog == MOUNTD) registeredport = 635;
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
	char handle[32];
	int n;
	
	credentials(request);
	
	/* verifier */
	skiplump(request);
	
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
			/* Add Mount(cracker, packer); */
			path = getstring(request);
			fprintf(console, "mountd: mount Path = %s\n",path);
			if (makehandle(path, handle) >= 0)
			{
				addint(&reply, SUCCESS);
				adddata(&reply, (unsigned char *)handle, sizeof(handle));
			}
			else
			{
				addint(&reply, 2);	/* no such file */
				addint(&reply, 0);
			}
			break;
		case 2:
			/* Mount Entries(cracker, packer); */
			break;
		case 3:
			/* Remove Mount */
			path = getstring(request);
			fprintf(console, "mountd: unmount Path = %s\n",path);
			
			addint(&reply, SUCCESS);
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
	int disksize,freesize;
	int fh, n, count;

	credentials(request);
	
	/* verifier */
	skiplump(request);
	
	reply.crp = 0;
	addint(&reply, ntohl(header->xid));
	addint(&reply, REPLY);
	addint(&reply, MSG_ACCEPTED);
	addint(&reply, 0);		/* opaque_verf */
	addint(&reply, 0);		/* opaque_verf size */
	addint(&reply, SUCCESS);

	addint(&reply, NFS_OK);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* GetAttr(cracker, packer); */
			fh = getfilehandle(request);
			fprintf(console, "nfsd: getattr: %s\n", mounttable[fh]);
			if (stat(mounttable[fh], &info) == 0)
			{
				if ((info.st_mode & S_IFDIR) == S_IFDIR)
				{
					addint(&reply, S_IFDIR);
					addint(&reply, 0777);			/* info.st_perm */
					addint(&reply, info.st_nlink);
					addint(&reply, info.st_uid);
					addint(&reply, info.st_uid);	/* no group */
					addint(&reply, (unsigned int)info.st_size);
					addint(&reply, 512);
					addint(&reply, info.st_dev);
					addint(&reply, FDNPB);
					addint(&reply, 1);	/* fsid */
					addint(&reply, (unsigned int)info.st_ino);
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
				}
				else
				{
					addint(&reply, S_IFREG);
					addint(&reply, 0777);
					addint(&reply, info.st_nlink);
					addint(&reply, info.st_uid);
					addint(&reply, info.st_uid);	/* no group */
					addint(&reply, (unsigned int)info.st_size);
					addint(&reply, 512);
					addint(&reply, info.st_dev);
					addint(&reply, ((unsigned int)info.st_size + 511) / 512);
					addint(&reply, 1);	/* fsid */
					addint(&reply, (unsigned int)info.st_ino);
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
					addint(&reply, (unsigned int)info.st_mtime);
					addint(&reply, 0);	/* usec */
				}
			}
			else
			{
			
			}
			break;
		case 2:
			/* SetAttr(cracker, packer); */
			break;
		case 3:
			/* Root(). No-op. */
			break;
		case 4:
			/* Lookup(cracker, packer); */
			fh = getfilehandle(request);
			path = getstring(request);
			fprintf(console, "nfsd: lookup = %s\n",path);
			break;
		case 5:
			/* ReadLink(cracker, packer); */
			break;
		case 6:
			/* Read(cracker, packer); */
			break;
		case 8:
			/* Write(cracker, packer); */
			break;
		case 9:
			/* Create(cracker, packer); */
			break;
		case 10:
			/* Remove(cracker, packer); */
			break;
		case 11:
			/* Rename(cracker, packer); */
			break;
		case 13:
			/* SymLink(cracker, packer); */
			break;
		case 14:
			/* MkDir(cracker, packer); */
			break;
		case 15:
			/* RmDir(cracker, packer); */
			break;
		case 16:
			/* ReadDir(cracker, packer); */
			fh = getfilehandle(request);
			n = getuint(request);
			count = getuint(request);
			
			fprintf(console, "nfsd: readdir: count = %s\n", count);
			break;
		case 17:
			/* StatFS(cracker, packer); */
			addint(&reply, 4096);			/* tsize: optimum transfer size */
			addint(&reply, 512);			/* Block size of FS */
#ifdef __clang__
			/* fake some numbers */
			disksize = 40 * 1024 * 1024 / 512;
			freesize = 10 * 1024 * 1024 / 512;
#else
			n = open(devname, O_RDONLY);
			lseek(n, 512, SEEK_SET);
			read(n, &sirbuf, sizeof(sirbuf));
			close(n);
			disksize = (sirbuf.ssizfr[0] << 16) + (sirbuf.ssizfr[1] << 8) + (sirbuf.ssizfr[2] << 0);
			freesize = (sirbuf.sfreec[0] << 16) + (sirbuf.sfreec[1] << 8) + (sirbuf.sfreec[2] << 0);
#endif
			addint(&reply, disksize);					/* Total # of blocks (of the above size) */
			addint(&reply, freesize);					/* Free blocks */
			addint(&reply, freesize);					/* Free blocks available to non-priv. users */
			fprintf(console, "nfsd: StatFS: %d blocks, free %d\n", disksize, freesize);
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
	
