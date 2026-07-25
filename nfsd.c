#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>

/*

 mount -v -t nfs -o proto=udp,vers=2 localhost:/Users/Shared  ~/flexdisk/

*/

/* standard compiler define for Tektronix 440x */
#ifdef tek

#include <sys/sir.h>

#include <net/inet.h>
#include <net/nerrno.h>
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

#define  IPPROTO_UDP IPPR_UDP
#define socklen_t unsigned int

struct sir sirbuf;

#else

//extern int open();
//extern int wait();
//extern int kill();
#include <stdlib.h>

#define in_sockaddr sockaddr_in
#define st_perm st_mode
#define S_IOREAD         S_IROTH         /* backward compatability */
#define S_IOWRITE        S_IWOTH         /* backward compatability */
#define S_IOEXEC         S_IXOTH         /* backward compatability */

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
	unsigned char pathtokens[20];
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
#define MOUNTD_PORT 6135
#define NFSD_PORT 21049


FILE *console;

#ifdef tek
/* missing CRT */
int mkdir(path, mode)
char *path;
unsigned int mode;
{
	char linkpath[256], linkdest[256];
	char *pcVar1;

	mknod(path, 0x0800 + 0x38 + 0x07, 0x0000);
	chown(path, 0);

	/* frpom Ghidra decompile of Tek4404 crdir command */
	strcpy(linkpath,path);
	strcat(linkpath,"/.");
	link(path,linkpath);
	chown(linkpath, 0);
	
	if (path[0] == '/') {
		strcpy(linkpath,path);
	}
	else {
		strcpy(linkpath,"./");
		strcat(linkpath,path);
	}
	pcVar1 = strchr(linkpath, '/');
	if (linkpath == pcVar1) {
		pcVar1 = pcVar1 + 1;
	}
	*pcVar1 = '\0';
	strcpy(linkdest,path);
	strcat(linkdest,"/..");
	link(linkpath,linkdest);
}
#endif

/* cache of file handle entries */
unsigned int filetablemask = 0;
char filetable[32][256];

int stringcachelen = 0;
char *stringcache;

short numsubpaths = 0;
int subpathindex[256];

int addsubpath(path)
char *path;
{
	short n;
	char *ptr;
	
	/* init */
	if (stringcachelen == 0)
	{
		stringcachelen = 2048;
		stringcache = malloc(stringcachelen);

		/* subpath index 0 terminates run */
		numsubpaths = 1;
		subpathindex[0] = 0;
	}
	
	/* find it */
	for (n=0; n<numsubpaths; n++)
	{
		if (!strcmp(path, stringcache + subpathindex[n]))
		{
			return n;
		}
	}

	/* append it  */
	ptr = stringcache + subpathindex[numsubpaths-1];
	ptr += strlen(ptr) + 1;
	strcpy(ptr, path);
	subpathindex[numsubpaths++] = ptr - stringcache;

	/* expand it */
	ptr += strlen(ptr) + 1;
	if (ptr - stringcache > stringcachelen)
	{
		stringcachelen += stringcachelen / 2;
		stringcache = realloc(stringcache, stringcachelen);
		fprintf(console, "pathslen = %d\n",stringcachelen);
	}
	
	return numsubpaths - 1;
}

int encodepath(filepath, encoded)
char *filepath;
unsigned char *encoded;
{
	char working[1024];
	char *ptr;
	int n;
	
	fprintf(console, "encodepath(%s): ",filepath);
	n = 0;
	strcpy(working, filepath);
	ptr = strtok(working, "/");
	while(ptr)
	{
		encoded[n++] = addsubpath(ptr);
		fprintf(console, "%d, ", encoded[n-1]);
		ptr = strtok(NULL,  "/");
	}
	fprintf(console, "\n");
	
	return n;
}

void decodepath(encoded, path)
unsigned char *encoded;
char *path;
{
	int n = 0;

	path[0] = '\0';
	
	if (stringcache == NULL)
		return;
	
	while(encoded[n])
	{
		strcat(path, "/");
		strcat(path, stringcache + subpathindex[encoded[n]]);
		n++;
	}
}

void addint(reply, val)
struct response *reply;
unsigned int val;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->crp);

	*ptr = htonl(val);
	reply->crp += sizeof(val);
}

void addfilehandle(reply, fh)
struct response *reply;
struct filehandle *fh;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->crp);
	int len = sizeof(struct filehandle);

	memcpy(ptr, fh, len);
	len = (len + 3) & -4;
	reply->crp += len;
}

void addstring(reply, string, len)
struct response *reply;
char *string;
int len;
{
	char *ptr;
	
	addint(reply, len);
	ptr = reply->buffer + reply->crp;

	memcpy(ptr, string, len);
	ptr[len] = '\0';
	ptr[len+1] = '\0';
	ptr[len+2] = '\0';
	len = (len + 3) & -4;
	reply->crp += len;
}

void adddata(reply, data, len)
struct response *reply;
unsigned char *data;
int len;
{
	unsigned int *ptr;

	addint(reply, len);
	ptr = (unsigned int *)(reply->buffer + reply->crp);
	
	memcpy(ptr, data, len);
	len = (len + 3) & -4;
	reply->crp += len;
}

void addfromfile(reply, fd, len)
struct response *reply;
int fd;
int len;
{
	unsigned int *ptr;

	addint(reply, len);
	ptr = (unsigned int *)(reply->buffer + reply->crp);
	
	if (len > (4096 - reply->crp))
		len = 4096 - reply->crp;
	
	len = read(fd, ptr, len);
	fprintf(console, "  addfile: read %d bytes\n", len);
	ptr[-1] = htonl(len);				/* what we actually read */
	len = (len + 3) & -4;
	reply->crp += len;
}

unsigned int nfsmode2unix(nfsmode)
unsigned int nfsmode;
{
	unsigned int perms = 0;

	/* translate from NFS bits */
	if (nfsmode & ROWN)
		perms |= S_IREAD;
	if (nfsmode & WOWN)
		perms |= S_IWRITE;
	if (nfsmode & XOWN)
		perms |= S_IEXEC;
	if (nfsmode & ROTH)
		perms |= S_IOREAD;
	if (nfsmode & WOTH)
		perms |= S_IOWRITE;
	if (nfsmode & XOTH)
		perms |= S_IOEXEC;
#ifndef tek
	if (nfsmode & RGRP)
		perms |= S_IRGRP;
	if (nfsmode & WGRP)
		perms |= S_IWGRP;
	if (nfsmode & XGRP)
		perms |= S_IXGRP;
#endif

	fprintf(console, "nfsmode2unix:  %4.4x => %4.4x\n", nfsmode,perms);
	return perms;
}

void addfattr(reply, info)
struct response *reply;
struct stat *info;
{
	unsigned int nfsperms = 0;

	/* translate to NFS bits */
	if (info->st_perm & S_IREAD)
		nfsperms |= ROWN;
	if (info->st_perm & S_IWRITE)
		nfsperms |= WOWN;
	if (info->st_perm & S_IEXEC)
		nfsperms |= XOWN;
	if (info->st_perm & S_IOREAD)
		nfsperms |= ROTH;
	if (info->st_perm & S_IOWRITE)
		nfsperms |= WOTH;
	if (info->st_perm & S_IOEXEC)
		nfsperms |= XOTH;
#ifndef tek
	if (info->st_perm & S_IRGRP)
		nfsperms |= RGRP;
	if (info->st_perm & S_IWGRP)
		nfsperms |= WGRP;
	if (info->st_perm & S_IXGRP)
		nfsperms |= XGRP;
#endif

	if ((info->st_mode & S_IFDIR) == S_IFDIR)
	{
		addint(reply, NFDIR);
		addint(reply, DIR_NFS | nfsperms);			/* info->st_perm */
		addint(reply, info->st_nlink);
		addint(reply, info->st_uid);
#ifdef tek
		addint(reply, info->st_uid);		/* no group */
#else
		addint(reply, info->st_gid);
#endif
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
		addint(reply, REG | nfsperms);
		addint(reply, info->st_nlink);
		addint(reply, info->st_uid);
#ifdef tek
		addint(reply, info->st_uid);		/* no group */
#else
		addint(reply, info->st_gid);
#endif
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
	request->crp += sizeof(struct filehandle);
	
	return ptr;
}

char *getstring(request)
struct conn *request;
{
	static char name[1024];
	unsigned int len = getuint(request);

	memcpy(name, request->buffer + request->crp, len);
	name[len] = '\0';
	len = (len + 3) & -4;
	request->crp += len;
	
	return name;
}

void getsattr(request, info)
struct conn *request;
struct stat *info;
{
		info->st_mode = nfsmode2unix(getuint(request));
		info->st_uid = getuint(request);
#ifdef tek
		getuint(request);	/* no group */
#else
		info->st_gid = getuint(request);
#endif
		info->st_size = getuint(request);
#ifdef tek
		getuint(request);	getuint(request);	/* no access time */
#else
		info->st_atime = getuint(request);	getuint(request);
#endif
		info->st_mtime = getuint(request);	getuint(request);
}



void releasehandle(path)
char *path;
{

}

int makehandle(path, info, handle)
char *path;
struct stat *info;
struct filehandle *handle;
{
	
	/* make a file handle */
	memset(handle, 0, sizeof(struct filehandle));
	handle->inode = info->st_ino;
	handle->dev = info->st_dev;
	encodepath(path, handle->pathtokens);
	
	return 0;
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
			addint(&reply, NFS_OK);
			break;
		case 3:
			/* GetPort */
			prog = getuint(request);
			vers = getuint(request);
			prot = getuint(request);
			port = getuint(request);
			registeredport = 0;
			if (prog == NFSD && prot == IPPROTO_UDP) registeredport = NFSD_PORT;
			if (prog == MOUNTD && prot == IPPROTO_UDP) registeredport = MOUNTD_PORT;
			if (registeredport)
			{
				addint(&reply, NFS_OK);
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
			if (stat(path, &info) == 0 && ((info.st_mode & S_IFDIR) == S_IFDIR))
			{
				makehandle(path, &info, &handle);
				addint(&reply, NFS_OK);
				addfilehandle(&reply, &handle);
				fprintf(console, "mountd: mount Path = %s\n",path);
			}
			else
			{
				addint(&reply, NFSERR_NOENT);	/* no such directory */
				addint(&reply, 0);
			}
			break;
		case 2:
			/* Mount Entries */
			break;
		case 3:
			/* Remove Mount */
			path = getstring(request);
			releasehandle(path);
			addint(&reply, NFS_OK);
			fprintf(console, "mountd: unmount Path = %s\n",path);
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
	int n, count, offset, fd, rc;
	struct filehandle handle;
	struct filehandle *fh;
	DIR *d;
	struct direct *dir;
	
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
			decodepath(fh->pathtokens, filepath);
			if (stat(filepath, &info) == 0)
			{
				addint(&reply, NFS_OK);
				addfattr(&reply, &info);
				fprintf(console, "nfsd: getattr: %s\n",filepath);
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
			decodepath(fh->pathtokens, filepath);
			strcat(filepath, "/");
			strcat(filepath, path);
			if (stat(filepath, &info) == 0)
			{
				makehandle(filepath, &info, &handle);
				addint(&reply, NFS_OK);
				addfilehandle(&reply, &handle);
				addfattr(&reply, &info);
				fprintf(console, "nfsd: lookup = %s\n", filepath);
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
			fh = getfilehandle(request);
			offset = getuint(request);
			count = getuint(request);
			n = getuint(request);
			decodepath(fh->pathtokens, filepath);
			if (stat(filepath, &info) == 0)
			{
				if ((info.st_mode & S_IFREG) == S_IFREG)
				{
					/* TODO: check file permissions */

					fd = open(filepath, O_RDONLY);
					rc = lseek(fd, offset, SEEK_SET);
					
					addint(&reply, NFS_OK);
					addfattr(&reply, &info);
					addfromfile(&reply, fd, count);
					close(fd);
				}
				else
				{
					addint(&reply, NFSERR_ISDIR);
				}
			}
			else
			{
				addint(&reply, 2);	/* no such file */
			}
			break;
		case 8:
			/* Write */
			fh = getfilehandle(request);
			n = getuint(request);
			offset = getuint(request);
			n = getuint(request);
			decodepath(fh->pathtokens, filepath);
			if (stat(filepath, &info) == 0)
			{
				if ((info.st_mode & S_IFREG) == S_IFREG)
				{
					/* TODO: check file permissions */

					fd = open(filepath, O_WRONLY);
					rc = lseek(fd, offset, SEEK_SET);
					n = getuint(request);
					rc = write(fd, request->buffer + request->crp, n);
					close(fd);
					if (rc == n)
					{
						/* update info with new length */
						if (offset + n > info.st_size)
							info.st_size = offset + n;
							
						addint(&reply, NFS_OK);
						addfattr(&reply, &info);
					}
					else
					{
						addint(&reply, NFSERR_IO);
					}
				}
				else
				{
					addint(&reply, NFSERR_ISDIR);
				}
			}
			else
			{
				addint(&reply, 2);	/* no such file */
			}
			break;
		case 9:
			/* Create */
			fh = getfilehandle(request);
			path = getstring(request);
			decodepath(fh->pathtokens, filepath);
			strcat(filepath, "/");
			strcat(filepath, path);
			getsattr(request, &info);
			fd = creat(filepath, info.st_mode);
			lseek(fd, info.st_size, SEEK_SET);
			chown(filepath, info.st_uid, info.st_gid);
			if (stat(filepath, &info) == 0)
			{
				makehandle(filepath, &info, &handle);
				addint(&reply, NFS_OK);
				addfilehandle(&reply, &handle);
				addfattr(&reply, &info);
				fprintf(console, "nfsd: create = %s\n", filepath);
			}
			else
			{
				addint(&reply, 2);	/* no such file */
			}
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
			fh = getfilehandle(request);
			path = getstring(request);
			decodepath(fh->pathtokens, filepath);
			strcat(filepath, "/");
			strcat(filepath, path);
			getsattr(request, &info);
			mkdir(filepath, info.st_mode);
			chown(filepath, info.st_uid, info.st_gid);
			if (stat(filepath, &info) == 0)
			{
				makehandle(filepath, &info, &handle);
				addint(&reply, NFS_OK);
				addfilehandle(&reply, &handle);
				addfattr(&reply, &info);
			
				fprintf(console, "nfsd: mkdir = %s\n", filepath);
			}
			break;
		case 15:
			/* RmDir */
			break;
		case 16:
			/* ReadDir */
			fh = getfilehandle(request);
			offset = getuint(request);
			count = getuint(request);
			fprintf(console, "nfsd: readdir: offset = %d count = %d\n", offset, count);
			
			/* clamp to our buffer size */
			if (count > 4096)
				count = 4096;
				
			/* account for some wrapping costs */
			count -= 32;
			decodepath(fh->pathtokens, filepath);
			d = opendir(filepath);
			if (d)
			{
				n = 0;
				addint(&reply, NFS_OK);
				while ((dir = readdir(d)) != NULL)
				{
					/* skip if not past starting point */
					if (n >= offset)
					{
						/* entry follows */
						addint(&reply, 1);
						
						addint(&reply, n);
						addstring(&reply, dir->d_name, dir->d_namlen);
						addint(&reply, offset + n);
						fprintf(console, "nfsd: readdir: %3d: %s\n", offset + n, dir->d_name);
					}
	
					n++;
					
					if (reply.crp > count)
						break;
				}
				closedir(d);

				/* no entry follows */
				addint(&reply, 0);

				/* complete or run out of room? */
				addint(&reply, (reply.crp > count) ? 0 : 1);
			}
			break;
		case 17:
			/* StatFS */
			addint(&reply, NFS_OK);
			addint(&reply, TRANSFER_SIZE);			/* tsize: optimum transfer size */
			addint(&reply, BLOCK_SIZE);			/* Block size of FS */
#ifndef tek
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

#ifdef tek
	if (geteuid() != 0)
		exit(-1);
#endif

	/* we act as portmapd, mountd and nfsd... */
	portmapsock = createudpsock(PORTMAPPERD_PORT);
	mountsock = createudpsock(MOUNTD_PORT);
	nfssock = createudpsock(NFSD_PORT);

	/* cannot continue */
	if (portmapsock < 0 || mountsock < 0 || nfssock < 0)
		exit(-2);

	/* run loop */
	while(1)
	{
		struct conn request;
		fd_set fd_in;
		socklen_t fromSize = sizeof(request.from);
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
	
