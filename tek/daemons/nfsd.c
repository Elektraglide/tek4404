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
#define CHOWN(A,B,C) chown(A,B) /* does not have group id */

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

#define CHOWN(A,B,C) chown(A,B,C)

//#include "uniflexshim.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

enum auth_flavor {
	AUTH_NULL       = 0,
	AUTH_UNIX       = 1,
	AUTH_SHORT      = 2,
	AUTH_DES        = 3
	/* and more to be defined */
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
		
		NFS3ERR_INVAL		= 22,
		
		NFSERR_FBIG			= 27,
		NFSERR_NOSPC		= 28,
		NFSERR_ROFS			= 30,
		NFSERR_NAMETOOLONG	= 63,
		NFSERR_NOTEMPTY		= 66,
		NFSERR_DQUOT		= 69,
		NFSERR_STALE		= 70,
		NFSERR_WFLUSG		= 99,
		
		NFS3ERR_BADHANDLE = 10001,
		NFS3ERR_BAD_COOKIE = 10003,
		NFS3ERR_NOTSUPP = 10004,
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

enum createmode3 {
UNCHECKED = 0,
GUARDED = 1,
EXCLUSIVE = 2
};

enum time_how {
DONT_CHANGE = 0,
SET_TO_SERVER_TIME = 1,
SET_TO_CLIENT_TIME = 2
};

#define NFS_TRUE 1
#define NFS_FALSE 0

struct rpcheader {
	unsigned int xid;
	unsigned int msg_type;
	unsigned int rpcvers;
	unsigned int prog;
	unsigned int vers;
	unsigned int proc;
};

struct filehandle {
	unsigned int length;		/* embedded length for NFSv3 support */
	unsigned int inode;
	unsigned short dev;
	unsigned short fsid;
	unsigned char pathtokens[20];	/* 32 bytes total */
};

/* request and response state */
struct conn {
	int sock;
	struct in_sockaddr from;
	char buffer[TRANSFER_SIZE];
	int crp,len;
};

/* needs to be dynamically resized? */
struct response {
	char buffer[TRANSFER_SIZE];
	int cwp;
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

/* logging credential details */
#define LOGCREDS 0

FILE *console;

#ifdef tek
/* missing CRT */
int mkdir(path, mode)
char *path;
unsigned int mode;
{
	char linkpath[256], linkdest[256];
	char *pcVar1;

	/* TODO: convert mode to Uniflex */
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

int add_subpath(path)
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
	
	/* do we need to expand */
	n = strlen(path) + 1;
	if (ptr - stringcache + n > stringcachelen)
	{
		stringcachelen += stringcachelen / 2;
		stringcache = realloc(stringcache, stringcachelen);
		ptr = stringcache + subpathindex[numsubpaths-1];
		ptr += strlen(ptr) + 1;
	}
	strcpy(ptr, path);
	subpathindex[numsubpaths++] = ptr - stringcache;
	
	return numsubpaths - 1;
}

int encodepath(filepath, encoded)
char *filepath;
unsigned char *encoded;
{
	char working[1024];
	char *ptr;
	int n;
	
	n = 0;
	strcpy(working, filepath);
	ptr = strtok(working, "/");
	while(ptr)
	{
		encoded[n++] = add_subpath(ptr);
		ptr = strtok(NULL,  "/");
	}
	
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

void add_uint(reply, val)
struct response *reply;
unsigned int val;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->cwp);

	*ptr = htonl(val);
	reply->cwp += sizeof(val);
}

void add_uint64(reply, val)
struct response *reply;
unsigned int val;
{
	
	add_uint(reply, 0);
	add_uint(reply, val);
}

void add_nfstime(reply, seconds)
struct response *reply;
unsigned int seconds;
{
	
	add_uint(reply, seconds);
	add_uint(reply, 0);
}

void add_cookie3(reply, cookie)
struct response *reply;
unsigned int cookie;
{
	
	add_uint(reply, 0);
	add_uint(reply, cookie);
}

void add_filehandle(reply, fh)
struct response *reply;
struct filehandle *fh;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->cwp);
	int len = sizeof(struct filehandle);

	memcpy(ptr, fh, len);
	len = (len + 3) & -4;
	reply->cwp += len;
}

void add_post_filehandle(reply, fh)
struct response *reply;
struct filehandle *fh;
{
	add_uint(reply, 1);
	add_filehandle(reply, fh);
}

void add_string(reply, string, len)
struct response *reply;
char *string;
int len;
{
	char *ptr;
	
	add_uint(reply, len);
	ptr = reply->buffer + reply->cwp;

	memcpy(ptr, string, len);
	ptr[len] = '\0';
	ptr[len+1] = '\0';
	ptr[len+2] = '\0';
	len = (len + 3) & -4;
	reply->cwp += len;
}

void add_data(reply, data, len)
struct response *reply;
unsigned char *data;
int len;
{
	unsigned int *ptr;

	add_uint(reply, len);
	ptr = (unsigned int *)(reply->buffer + reply->cwp);
	
	memcpy(ptr, data, len);
	len = (len + 3) & -4;
	reply->cwp += len;
}

int add_fromfile(reply, fd, len)
struct response *reply;
int fd;
int len;
{
	unsigned int *ptr;
	int rc;

	add_uint(reply, len);
	ptr = (unsigned int *)(reply->buffer + reply->cwp);
	
	/* clamp to remaining space */
	if (len > (TRANSFER_SIZE - reply->cwp))
		len = TRANSFER_SIZE - reply->cwp;
	
	rc = read(fd, ptr, len);
	ptr[-1] = htonl(rc);				/* what we actually read */
	rc = (rc + 3) & -4;
	reply->cwp += rc;
	
	return rc;
}

int add_fromfile3(reply, fd, len)
struct response *reply;
int fd;
int len;
{
	unsigned int *ptr;
	int rc;
	
	add_uint(reply, len);
	add_uint(reply, 0);
	add_uint(reply, 0);
	ptr = (unsigned int *)(reply->buffer + reply->cwp);
	
	/* clamp to remaining space */
	if (len > (TRANSFER_SIZE - reply->cwp))
		len = TRANSFER_SIZE - reply->cwp;
	
	rc = read(fd, ptr, len);
	ptr[-3] = htonl(rc);				/* what we actually read */
	ptr[-2] = (rc < len);				/* eof */
	
	/* variable length array */
	ptr[-1] = htonl(rc);
	rc = (rc + 3) & -4;
	reply->cwp += rc;
	
	return rc;
}

unsigned int nfsmode2host(nfsmode)
unsigned int nfsmode;
{
	unsigned int perms = 0;

	if (nfsmode == -1)
		perms = -1;
	else
	{
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
	}
	
	fprintf(console, "nfsmode2host:  %4.4x => %4.4x\n", nfsmode,perms);
	return perms;
}

unsigned int host2nfsmode(hostperms)
unsigned int hostperms;
{
	unsigned int nfsperms = 0;

	if (hostperms & S_IREAD)
		nfsperms |= ROWN;
	if (hostperms & S_IWRITE)
		nfsperms |= WOWN;
	if (hostperms & S_IEXEC)
		nfsperms |= XOWN;
	if (hostperms & S_IOREAD)
		nfsperms |= ROTH;
	if (hostperms & S_IOWRITE)
		nfsperms |= WOTH;
	if (hostperms & S_IOEXEC)
		nfsperms |= XOTH;
#ifndef tek
	if (hostperms & S_IRGRP)
		nfsperms |= RGRP;
	if (hostperms & S_IWGRP)
		nfsperms |= WGRP;
	if (hostperms & S_IXGRP)
		nfsperms |= XGRP;
#endif

	return nfsperms;
}

void add_fattr(reply, info, fsid)
struct response *reply;
struct stat *info;
int fsid;
{
	unsigned int nfsperms = host2nfsmode(info->st_perm);

	if ((info->st_mode & S_IFDIR) == S_IFDIR)
	{
		add_uint(reply, NFDIR);
		add_uint(reply, DIR_NFS | nfsperms);
		add_uint(reply, info->st_nlink);
		add_uint(reply, info->st_uid);
#ifdef tek
		add_uint(reply, info->st_uid);		/* no group */
#else
		add_uint(reply, info->st_gid);
#endif
		add_uint(reply, (unsigned int)info->st_size);
		add_uint(reply, BLOCK_SIZE);
		add_uint(reply, info->st_dev);
		add_uint(reply, BLOCK_SIZE / FDNPB);
	}
	else
	{
		add_uint(reply, NFREG);
		add_uint(reply, REG | nfsperms);
		add_uint(reply, info->st_nlink);
		add_uint(reply, info->st_uid);
#ifdef tek
		add_uint(reply, info->st_uid);		/* no group */
#else
		add_uint(reply, info->st_gid);
#endif
		add_uint(reply, (unsigned int)info->st_size);
		add_uint(reply, BLOCK_SIZE);
		add_uint(reply, info->st_dev);
		add_uint(reply, ((unsigned int)info->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
	}
	add_uint(reply, fsid);
	add_uint(reply, (unsigned int)info->st_ino);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
}

void add_fattr3(reply, info, fsid)
struct response *reply;
struct stat *info;
int fsid;
{
	unsigned int nfsperms = host2nfsmode(info->st_perm);
	if ((info->st_mode & S_IFDIR) == S_IFDIR)
	{
		add_uint(reply, NFDIR);
		add_uint(reply, nfsperms);			/* info->st_perm */
		add_uint(reply, info->st_nlink);
		add_uint(reply, info->st_uid);
#ifdef tek
		add_uint(reply, info->st_uid);		/* no group */
#else
		add_uint(reply, info->st_gid);
#endif
		add_uint64(reply, BLOCK_SIZE);
		add_uint64(reply, BLOCK_SIZE / FDNPB);
	}
	else
	{
		add_uint(reply, NFREG);
		add_uint(reply, nfsperms);
		add_uint(reply, info->st_nlink);
		add_uint(reply, info->st_uid);
#ifdef tek
		add_uint(reply, info->st_uid);		/* no group */
#else
		add_uint(reply, info->st_gid);
#endif
		add_uint64(reply, (unsigned int)info->st_size);
		add_uint64(reply, ((unsigned int)info->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
	}
	add_uint64(reply, 0);														/* specdata3 */
	add_uint64(reply, fsid);
	add_uint64(reply, (unsigned int)info->st_ino);	/* fileid */
	add_nfstime(reply, (unsigned int)info->st_atime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_ctime);
}

void add_post_fattr3(reply, info, fsid)
struct response *reply;
struct stat *info;
int fsid;
{
	add_uint(reply, 1);
	add_fattr3(reply, info, fsid);
}

void add_post_wccattr3(reply, info)
struct response *reply;
struct stat *info;
{
	add_uint(reply, 1);
	add_uint64(reply, (unsigned int)info->st_size);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_ctime);
}

void add_wcc_data(reply, preinfo, postinfo)
struct response *reply;
struct stat *preinfo;
struct stat *postinfo;
{
	add_post_wccattr3(reply, preinfo);

	if (postinfo)
		add_post_fattr3(reply, postinfo);
	else
		add_uint(reply, 0);
}

int validate(request, prognum)
struct conn *request;
int prognum;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;

	/* fprintf(console,"RPC: xid:%8.8x rpcvers:%d vers:%d prog:%d proc:%d\n", ntohl(header->xid), ntohl(header->rpcvers), ntohl(header->vers), ntohl(header->prog), ntohl(header->proc));
	*/

	reply.cwp = 0;
	if (ntohl(header->msg_type) != CALL)
	{
		/* corrupted */
		add_uint(&reply, ntohl(header->xid));
		add_uint(&reply, REPLY);
		add_uint(&reply, MSG_ACCEPTED);
		add_uint(&reply, 0);		/* opaque_verf */
		add_uint(&reply, 0);		/* opaque_verf size */
		add_uint(&reply, GARBAGE_ARGS);
		sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	else
	if (ntohl(header->rpcvers) != 2)
	{
		/* not NFSv2 or NFSv3 */
		add_uint(&reply, ntohl(header->xid));
		add_uint(&reply, REPLY);
		add_uint(&reply, MSG_DENIED);
		add_uint(&reply, PROG_MISMATCH);
		add_uint(&reply, 2);
		add_uint(&reply, 2);
		sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	else
	if (ntohl(header->prog) != prognum)
	{
		/* not correct service */
		add_uint(&reply, ntohl(header->xid));
		add_uint(&reply, REPLY);
		add_uint(&reply, MSG_ACCEPTED);
		add_uint(&reply, 0);		/* opaque_verf */
		add_uint(&reply, 0);		/* opaque_verf size */
		add_uint(&reply, PROG_MISMATCH);
		add_uint(&reply, prognum);
		add_uint(&reply, prognum);
		sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
		return 0;
	}
	
	request->crp += sizeof(struct rpcheader);

	return ntohl(header->vers);
}

unsigned int get_uint(request)
struct conn *request;
{
	unsigned int *ptr = (unsigned int *)(request->buffer + request->crp);
	unsigned int val = ntohl(*ptr);
	request->crp += sizeof(val);

	return val;
}

unsigned int get_uint64(request)
struct conn *request;
{
	get_uint(request);
	unsigned int val = get_uint(request);
	return val;
}

#define get_cookie3 get_uint64

void get_verifier(request)
struct conn *request;
{
	unsigned int flavour = get_uint(request);
	unsigned int length = get_uint(request);
	request->crp += length;
}

void get_credentials(request, verbose)
struct conn *request;
int verbose;
{
	unsigned int flavour = get_uint(request);
	unsigned int length = get_uint(request);

	if (verbose)
	{
		if (flavour == AUTH_NULL)
		{
			fprintf(console, "get_credentials: %d AUTH_NULL\n", length);
		}
		else
		if (flavour == AUTH_UNIX)
		{
			unsigned int *ptr = (unsigned int *)(request->buffer + request->crp);
			int n;
			n = ntohl(ptr[1]);
			fprintf(console, "get_credentials: stamp:%8.8x machinename:'%*s'\n", ptr[0], n, ptr + 2);
			ptr += n;
			fprintf(console, "get_credentials: uid:%d gid:%d\n", ntohl(ptr[2]), ntohl(ptr[3]));
			fprintf(console, "get_credentials: gids: [ ");
			n = ntohl(ptr[4]);
			while(n--)
				fprintf(console, "%d ",ntohl(ptr[5+n]));
			fprintf(console, "]\n");
		}
	}

	request->crp += length;
}

struct filehandle *get_filehandle(request, filepath)
struct conn *request;
char *filepath;
{
	struct filehandle *ptr = (struct filehandle *)(request->buffer + request->crp);
	request->crp += sizeof(struct filehandle);

	/* TODO: if we have flushed the stringcache, return NFS3ERR_STALE */
	decodepath(ptr->pathtokens, filepath);

	return ptr;
}

char *get_string(request)
struct conn *request;
{
	static char name[1024];
	unsigned int len = get_uint(request);

	memcpy(name, request->buffer + request->crp, len);
	name[len] = '\0';
	len = (len + 3) & -4;
	request->crp += len;
	
	return name;
}

void get_sattr(request, info)
struct conn *request;
struct stat *info;
{
		info->st_mode = nfsmode2host(get_uint(request));
		info->st_uid = get_uint(request);
#ifdef tek
		getuint(request);	/* no group */
#else
		info->st_gid = get_uint(request);
#endif
		info->st_size = get_uint(request);
#ifdef tek
		getuint(request);	getuint(request);	/* no access time */
#else
		get_uint(request); info->st_atime = get_uint(request);
#endif
		get_uint(request); info->st_mtime = get_uint(request);
}

void get_sattr3(request, info)
struct conn *request;
struct stat *info;
{
	int time_how;
	
	info->st_mode = -1;
	if (get_uint(request))
		info->st_mode = nfsmode2host(get_uint(request));
		
	info->st_uid = -1;
	if (get_uint(request))
		info->st_uid = get_uint(request);
	if (get_uint(request))
#ifdef tek
			getuint(request);	/* no group */
#else
			info->st_gid = get_uint(request);
#endif

	info->st_size = -1;
	if (get_uint(request))
	{
		get_uint(request); info->st_size = get_uint(request);
	}
	
	time_how = get_uint(request);
#ifdef tek
	if (time_how == SET_TO_CLIENT_TIME)
	{
		getuint(request);	getuint(request);	/* no access time */
	}
#else
	if (time_how == SET_TO_SERVER_TIME)
		info->st_atime = time(NULL);
	else
	if (time_how == SET_TO_CLIENT_TIME)
	{
		get_uint(request); info->st_atime = get_uint(request);
	}
#endif
	
	time_how = get_uint(request);
	if (time_how == SET_TO_SERVER_TIME)
		info->st_mtime = time(NULL);
	else
	if (time_how == SET_TO_CLIENT_TIME)
	{
		get_uint(request); info->st_mtime = get_uint(request);
	}
}

void get_sattrguard3(request, info)
struct conn *request;
struct stat *info;
{
	if (get_uint(request))
	{
		get_uint(request); info->st_ctime = get_uint(request);
	}
}

void release_filehandle(path)
char *path;
{

}

int make_filehandle(path, info, handle)
char *path;
struct stat *info;
struct filehandle *handle;
{
	
	/* make a file handle */
	memset(handle, 0, sizeof(struct filehandle));
	handle->length = htonl(sizeof(struct filehandle) - 4);
	handle->inode = info->st_ino;
	handle->dev = info->st_dev;
	handle->fsid = 0;
	encodepath(path, handle->pathtokens);
	
	return 0;
}

int make_fsid(handle)
struct filehandle *handle;
{
	short n = 0;
	int result = 0xaa;
	
	while(handle->pathtokens[n])
	{
		result ^= handle->pathtokens[n];
		n++;
	}

	return result;
}

int create_UDP_sock(port)
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
		fprintf(console, "bind: port %d: %s\n", port, strerror(errno));
		close(sock);
		return -2;
	}
	fprintf(console,"listen on %d\n", port);

	return sock;
}

void logtimes(info)
struct stat *info;
{
	struct tm *ts;

	ts = localtime(&info->st_atime);
	fprintf(console, "atime: %2.2d-%2.2d-%4.4d %2.2d:%2.2d\n",
			ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900, ts->tm_hour, ts->tm_min);
	ts = localtime(&info->st_mtime);
	fprintf(console, "mtime: %2.2d-%2.2d-%4.4d %2.2d:%2.2d\n",
			ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900, ts->tm_hour, ts->tm_min);
	ts = localtime(&info->st_ctime);
	fprintf(console, "ctime: %2.2d-%2.2d-%4.4d %2.2d:%2.2d\n",
			ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900, ts->tm_hour, ts->tm_min);

}

void portmapperprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	unsigned int prog, vers, prot, port, registeredport;
	int n;
	
	/* expecting nullop credentials */
	get_credentials(request, 0);
	get_verifier(request);
	
	reply.cwp = 0;
	add_uint(&reply, ntohl(header->xid));
	add_uint(&reply, REPLY);
	add_uint(&reply, MSG_ACCEPTED);
	add_uint(&reply, 0);		/* opaque_verf */
	add_uint(&reply, 0);		/* opaque_verf size */

	/* I dont understand why it does not need SUCCESS here.. */

	switch(ntohl(header->proc))
	{
		default:
		case 0:
			add_uint(&reply, NFS_OK);
			break;
		case 3:
			/* GetPort */
			prog = get_uint(request);
			vers = get_uint(request);
			prot = get_uint(request);
			port = get_uint(request);
			registeredport = 0;
			if (prog == NFSD && prot == IPPROTO_UDP) registeredport = NFSD_PORT;
			if (prog == MOUNTD && prot == IPPROTO_UDP) registeredport = MOUNTD_PORT;
			if (registeredport)
			{
				add_uint(&reply, NFS_OK);
				add_uint(&reply, registeredport);
			}
			else
			{
				add_uint(&reply, PROG_UNAVAIL);
			}
			fprintf(console, "portmapd: prog:%d vers:%d prot:%d => registeredport:%d\n", prog, vers, prot, registeredport);
			break;
			
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
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
	
	get_credentials(request, 0);
	get_verifier(request);
	
	reply.cwp = 0;
	add_uint(&reply, ntohl(header->xid));
	add_uint(&reply, REPLY);
	add_uint(&reply, MSG_ACCEPTED);
	add_uint(&reply, 0);		/* opaque_verf */
	add_uint(&reply, 0);		/* opaque_verf size */
	add_uint(&reply, SUCCESS);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* Add Mount */
			path = get_string(request);
			if (stat(path, &info) == 0)
			{
				if ((info.st_mode & S_IFDIR) == S_IFDIR)
				{
					make_filehandle(path, &info, &handle);
					handle.fsid = make_fsid(&handle);
					add_uint(&reply, NFS_OK);
					add_filehandle(&reply, &handle);
					fprintf(console, "mountd: mount Path = %s for client@%s\n", path,  inet_ntoa((request->from.sin_addr)) );
					if (header->vers == htonl(3))
					{
						add_uint(&reply, 1);	/* maxlen */
						add_uint(&reply, 1);	/* len */
						add_uint(&reply, AUTH_UNIX);
					}
				}
				else
				{
				add_uint(&reply, NFSERR_NOTDIR);
				add_uint(&reply, 0);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
			}
			break;
		case 2:
			/* Mount Entries */
			break;
		case 3:
			/* Remove Mount */
			path = get_string(request);
			release_filehandle(path);
			add_uint(&reply, NFS_OK);
			fprintf(console, "mountd: unmount Path = %s for client@%s\n", path,  inet_ntoa((request->from.sin_addr)) );
			
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
	{
			fprintf(console, "mountd: sendto: %s\n",strerror(errno));
	}
}

void nfsprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info,reqinfo;
	char *path;
	char filepath[1024];
	int disksize,freesize;
	int n, count, offset, fd, rc;
	struct filehandle handle;
	struct filehandle *fh;
	DIR *d;
	struct direct *dir;
	
	get_credentials(request, LOGCREDS);
	get_verifier(request);
	
	reply.cwp = 0;
	add_uint(&reply, ntohl(header->xid));
	add_uint(&reply, REPLY);
	add_uint(&reply, MSG_ACCEPTED);
	add_uint(&reply, 0);		/* opaque_verf */
	add_uint(&reply, 0);		/* opaque_verf size */
	add_uint(&reply, SUCCESS);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* GetAttr */
			/* TODO: deal with NFSERR_STALE */
			fh = get_filehandle(request, filepath);
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_fattr(&reply, &info, fh->fsid);
				/*fprintf(console, "nfsd: get_attr: %s  perm:%4.4x\n", filepath, info.st_mode);*/
			}
			else
			{
				/* this should never happen.. */
				add_uint(&reply, NFSERR_NOENT);
			}
			break;
		case 2:
			/* SetAttr */
			fh = get_filehandle(request, filepath);
			get_sattr(request, &reqinfo);
			if (reqinfo.st_mode != -1)
				chmod(filepath, reqinfo.st_mode);
			if (reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if (reqinfo.st_size == 0)
				truncate(filepath, reqinfo.st_size);

			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_fattr(&reply, &info, fh->fsid);
				/*fprintf(console, "nfsd: setattr = %s\n", filepath);*/
			}
			else
			{
				add_uint(&reply, errno);
			}
			break;
		case 3:
			/* Root(). No-op. */
			break;
		case 4:
			/* Lookup */
			fh = get_filehandle(request, filepath);
			path = get_string(request);
			strcat(filepath, "/");
			strcat(filepath, path);
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_filehandle(&reply, &handle);
				add_fattr(&reply, &info, fh->fsid);
				/*fprintf(console, "nfsd: lookup = %s\n", filepath);*/
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);	/* no such file */
				add_uint(&reply, 0);
			}
			break;
		case 5:
			/* ReadLink */
			break;
		case 6:
			/* Read */
			fh = get_filehandle(request, filepath);
			offset = get_uint(request);
			count = get_uint(request);
			n = get_uint(request);
			if (stat(filepath, &info) == 0)
			{
				if ((info.st_mode & S_IFREG) == S_IFREG)
				{
					if (info.st_perm & S_IREAD)
					{
						fd = open(filepath, O_RDONLY);
						rc = lseek(fd, offset, SEEK_SET);
						
						add_uint(&reply, NFS_OK);
						add_fattr(&reply, &info, fh->fsid);
						add_fromfile(&reply, fd, count);
						close(fd);
					}
					else
					{
						add_uint(&reply, NFSERR_ACCES);
					}
				}
				else
				{
					add_uint(&reply, NFSERR_ISDIR);
				}
			}
			else
			{
				add_uint(&reply, 2);	/* no such file */
			}
			break;
		case 8:
			/* Write */
			fh = get_filehandle(request, filepath);
			n = get_uint(request);
			offset = get_uint(request);
			n = get_uint(request);
			if (stat(filepath, &info) == 0)
			{
				if ((info.st_mode & S_IFREG) == S_IFREG)
				{
					if (info.st_perm & S_IWRITE)
					{
						fd = open(filepath, O_WRONLY);
						rc = lseek(fd, offset, SEEK_SET);
						count = get_uint(request);
						rc = write(fd, request->buffer + request->crp, count);
						/*fprintf(console, "  write: %d bytes at %d\n", rc, offset);*/
						close(fd);
						if (rc == count)
						{
							add_uint(&reply, NFS_OK);

							/* quick update of stat */
							if (offset+rc > info.st_size)
								info.st_size = offset + rc;

							add_fattr(&reply, &info, fh->fsid);
						}
						else
						{
							add_uint(&reply, NFSERR_IO);
						}
					}
					else
					{
							add_uint(&reply, NFSERR_ACCES);
					}
				}
				else
				{
					add_uint(&reply, NFSERR_ISDIR);
				}
			}
			else
			{
				add_uint(&reply, 2);	/* no such file */
			}
			break;
		case 9:
			/* Create */
			fh = get_filehandle(request, filepath);
			path = get_string(request);
			strcat(filepath, "/");
			strcat(filepath, path);
			get_sattr(request, &info);
			fd = creat(filepath, info.st_mode);
			close(fd);
			if (info.st_mode != -1)
				chmod(filepath, info.st_mode);
			if (info.st_uid != -1)
				CHOWN(filepath, info.st_uid, info.st_gid);
			if (info.st_size == 0)
				truncate(filepath, info.st_size);
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_filehandle(&reply, &handle);
				add_fattr(&reply, &info, fh->fsid);
				/*fprintf(console, "nfsd: create = %s perm:%4.4x\n", filepath, info.st_mode);*/
			}
			else
			{
				add_uint(&reply, 2);	/* no such file */
			}
			break;
		case 10:
			/* Remove */
			fh = get_filehandle(request, filepath);
			path = get_string(request);
			strcat(filepath, "/");
			strcat(filepath, path);
			if (unlink(filepath) == 0)
			{
				add_uint(&reply, NFS_OK);
				/*fprintf(console, "nfsd: remove = %s\n", filepath);*/
			}
			else
			{
				add_uint(&reply, errno);
			}
			break;
		case 11:
			/* Rename */
			break;
		case 13:
			/* SymLink */
			break;
		case 14:
			/* MkDir */
			fh = get_filehandle(request, filepath);
			path = get_string(request);
			strcat(filepath, "/");
			strcat(filepath, path);
			get_sattr(request, &reqinfo);
			mkdir(filepath, reqinfo.st_mode);
			CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);

			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_filehandle(&reply, &handle);
				add_fattr(&reply, &info, fh->fsid);
				/*fprintf(console, "nfsd: mkdir = %s\n", filepath);*/
			}
			break;
		case 15:
			/* RmDir */
			break;
		case 16:
			/* ReadDir */
			fh = get_filehandle(request, filepath);
			offset = get_uint(request);
			count = get_uint(request);
			/*fprintf(console, "nfsd: readdir: offset = %d count = %d\n", offset, count);*/
			
			/* clamp to our buffer size */
			if (count > TRANSFER_SIZE)
				count = TRANSFER_SIZE;
				
			/* account for some wrapping costs */
			count -= 12;
			d = opendir(filepath);
			if (d)
			{
				n = 0;
				add_uint(&reply, NFS_OK);
				while ((dir = readdir(d)) != NULL)
				{
					/* skip if not past starting point */
					if (n >= offset)
					{
						/* entry follows */
						add_uint(&reply, 1);
						
						add_uint(&reply, n);
#ifdef __linux__
						add_string(&reply, dir->d_name, strlen(dir->d_name));
#else
						add_string(&reply, dir->d_name, dir->d_namlen);
#endif
						add_uint(&reply, offset + n);
						/*fprintf(console, "nfsd: readdir: %3d: %s\n", offset + n, dir->d_name);*/
					}
	
					n++;
					
					if (reply.cwp > count)
						break;
				}
				closedir(d);

				/* no entry follows */
				add_uint(&reply, 0);

				/* complete or run out of room? */
				add_uint(&reply, (reply.cwp > count) ? 0 : 1);
			}
			break;
		case 17:
			/* StatFS */
			add_uint(&reply, NFS_OK);
			add_uint(&reply, TRANSFER_SIZE);			/* tsize: optimum transfer size */
			add_uint(&reply, BLOCK_SIZE);			/* Block size of FS */
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
			add_uint(&reply, disksize);					/* Total # of blocks (of the above size) */
			add_uint(&reply, freesize);					/* Free blocks */
			add_uint(&reply, freesize);					/* Free blocks available to non-priv. users */
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
	{
			fprintf(console, "nfsd: sendto: %s\n",strerror(errno));
	}
	
}

#define FSF3_LINK 0x0001
#define FSF3_SYMLINK 0x0002
#define FSF3_HOMOGENEOUS 0x0008
#define FSF3_CANSETTIME 0x0010

#define ACCESS3_READ 0x0001
#define ACCESS3_LOOKUP 0x0002
#define ACCESS3_MODIFY 0x0004
#define ACCESS3_EXTEND 0x0008
#define ACCESS3_DELETE 0x0010
#define ACCESS3_EXECUTE 0x0020

void nfs3prog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info, reqinfo,preinfo;
	char *path;
	char dirpath[1024];
	char filepath[1024];
	int disksize,freesize,totalfdns,freefdns;
	int n, count, offset, fd, rc, how, cookieverf;
	struct filehandle handle;
	struct filehandle *fh;
	DIR *d;
	struct direct *dir;
	
	get_credentials(request, LOGCREDS && header->proc);
	get_verifier(request);
	
	reply.cwp = 0;
	add_uint(&reply, ntohl(header->xid));
	add_uint(&reply, REPLY);
	add_uint(&reply, MSG_ACCEPTED);
	add_uint(&reply, 0);		/* opaque_verf */
	add_uint(&reply, 0);		/* opaque_verf size */
	add_uint(&reply, SUCCESS);

	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* GetAttr */
			/* TODO: deal with NFS3ERR_STALE */
			fh = get_filehandle(request, filepath);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: get_attr3: %s  perm:%4.4x\n", filepath, info.st_mode);
			}
			else
			{
				/* this should never happen.. */
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
			}
			break;
		case 2:
			/* SetAttr */
			fh = get_filehandle(request, filepath);
			get_sattr3(request, &reqinfo);
			get_sattrguard3(request, &reqinfo);
			if (reqinfo.st_mode != -1)
				chmod(filepath, reqinfo.st_mode);
			if (reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if (reqinfo.st_size == 0)
				truncate(filepath, reqinfo.st_size);

			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: setattr3 = %s\n", filepath);
			}
			else
			{
				add_uint(&reply, errno);
				add_uint(&reply, 0);
			}
			break;
		case 3:
			/* Lookup */
			fh = get_filehandle(request, dirpath);
			path = get_string(request);
			strcpy(filepath, dirpath);
			strcat(filepath, "/");
			strcat(filepath, path);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_filehandle(&reply, &handle);
				add_post_fattr3(&reply, &info, fh->fsid);

				stat(dirpath, &info);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: lookup3 = %s on fsid=%d\n", filepath, fh->fsid);
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);	/* no such file */
				stat(dirpath, &info);
				add_post_fattr3(&reply, &info, fh->fsid);
			}
			break;
		case 4:
			/* Access */
			fh = get_filehandle(request, filepath);
			n = get_uint(request);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
					add_uint(&reply, NFS_OK);
					add_post_fattr3(&reply, &info, fh->fsid);
					/* never allow delete or execute */
					rc = n & ~(ACCESS3_DELETE | ACCESS3_EXECUTE);
	
					if (!(info.st_perm & S_IREAD))
						rc &= ~(ACCESS3_READ | ACCESS3_LOOKUP);
					if (!(info.st_perm & S_IWRITE))
						rc &= ~(ACCESS3_MODIFY | ACCESS3_EXTEND);

					if ((info.st_mode & S_IFDIR) == S_IFDIR)
					{
						rc |= (ACCESS3_READ | ACCESS3_LOOKUP);
					}

					add_uint(&reply, rc );
					fprintf(console, "nfsd: access3 = %s perm=%4.4x on fsid=%d\n", filepath, n, fh->fsid);
			}
			else
			{
					add_uint(&reply, NFSERR_NOENT);
					add_uint(&reply, 0);
					
			}
			break;
		case 5:
			/* ReadLink */
			add_uint(&reply, NFS3ERR_NOTSUPP);
			add_uint(&reply, 0);
			break;
		case 6:
			/* Read */
			fh = get_filehandle(request, filepath);
			offset = get_uint64(request);
			count = get_uint(request);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				if ((info.st_mode & S_IFREG) == S_IFREG)
				{
					/* TODO: check file permissions */
			fprintf(console, "nfsd: read3 = %s off=%d count=%d size=%d on fsid=%d\n", filepath, offset,count, info.st_size, fh->fsid);

					fd = open(filepath, O_RDONLY);
					rc = lseek(fd, offset, SEEK_SET);
					
					add_uint(&reply, NFS_OK);
					add_post_fattr3(&reply, &info, fh->fsid);
					add_fromfile3(&reply, fd, count);
					close(fd);
				}
				else
				{
					add_uint(&reply, NFS3ERR_INVAL);
					add_uint(&reply, 0);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
			}
			break;
		case 7:
			/* Write */
			fh = get_filehandle(request, filepath);
			offset = get_uint64(request);
			count = get_uint(request);
			how = get_uint(request);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &preinfo) == 0)
			{
				if ((preinfo.st_mode & S_IFREG) == S_IFREG)
				{
					if (how == 0)	/* UNSTABLE */
					{
						fprintf(console, "UNSTABLE mode ignored\n");
					}
				
					if (info.st_perm & S_IWRITE)
					{
						rc = 0;
						if (count > 0)
						{
							fd = open(filepath, O_WRONLY);
							rc = lseek(fd, offset, SEEK_SET);
							rc = write(fd, request->buffer + request->crp, count);
							/*fprintf(console, "  write: %d bytes at %d\n", rc, offset);*/
							close(fd);
						}
						if (rc >= 0)
						{
							add_uint(&reply, NFS_OK);

							stat(filepath, &info);
							add_wcc_data(&reply, &preinfo, &info);
							add_uint64(&reply, rc);
							add_uint(&reply, 2);			/* FILE_SYNC */
							add_uint64(&reply, 0);		/* writeverf3 */
						}
						else
						{
							add_uint(&reply, NFSERR_IO);
						}
					}
					else
					{
							add_uint(&reply, NFSERR_ACCES);
							add_uint(&reply, 0);
							add_uint(&reply, 0);
					}
				}
				else
				{
					add_uint(&reply, NFSERR_ISDIR);
				}
			}
			else
			{
				add_uint(&reply, 2);	/* no such file */
			}
			break;
		case 8:
			/* Create */
			fh = get_filehandle(request, dirpath);
			if (stat(dirpath, &preinfo) < 0)
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: Create3: %s  NFSERR_NOENT\n", dirpath);
				break;
			}
			path = get_string(request);
			strcpy(filepath, dirpath);
			strcat(filepath, "/");
			strcat(filepath, path);
			how = get_uint(request);
			if (how != EXCLUSIVE)
			{
				get_sattr3(request, &reqinfo);
				fprintf(console, "CREATE3: mode=%x uid=%d gid=%d size=%d\n",reqinfo.st_mode,reqinfo.st_uid,reqinfo.st_gid,reqinfo.st_size);
			}
			else
			{
				add_uint(&reply, NFS3ERR_NOTSUPP);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: Create3: %s  NFS3ERR_NOTSUPP\n", dirpath);
				break;
			}

			if (how == GUARDED)
			{
				if (stat(filepath, &info) == 0)
				{
					add_uint(&reply, NFSERR_EXIST);
					add_post_fattr3(&reply, &info, fh->fsid);
					fprintf(console, "nfsd: Create3: %s  NFS3ERR_EXIST\n", filepath);
					break;
				}
			}

			fd = creat(filepath, reqinfo.st_mode);
			close(fd);
			if (reqinfo.st_mode != -1)
				chmod(filepath, reqinfo.st_mode);
			if (reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if (reqinfo.st_size != -1)
				truncate(filepath, reqinfo.st_size);

			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_post_filehandle(&reply, &handle);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: create3 = %s perm:%4.4x on fsid=%d\n", filepath, info.st_mode, fh->fsid);

				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info);
			}
			else
			{
				add_uint(&reply, NFSERR_ACCES);
				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info);
			}
			break;
		case 9:
			/* MkDir */
			fh = get_filehandle(request, dirpath);
			if (stat(dirpath, &preinfo) < 0)
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: MkDir: %s  NFSERR_NOENT\n", dirpath);
				break;
			}
			path = get_string(request);
			strcpy(filepath, dirpath);
			strcat(filepath, "/");
			strcat(filepath, path);
			get_sattr3(request, &reqinfo);
			mkdir(filepath, reqinfo.st_mode);
			CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_post_filehandle(&reply, &handle);
				add_post_fattr3(&reply, &info, fh->fsid);

				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info);
				fprintf(console, "nfsd: mkdir3 = %s on fsid=%d\n", filepath, fh->fsid);
			}
			else
			{
				add_uint(&reply, NFSERR_ACCES);
				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info);
			}
			break;
		case 10:
			/* SymLink */
			add_uint(&reply, NFS3ERR_NOTSUPP);
			add_uint(&reply, 0);
			break;
		case 11:
			/* MkNod */
			add_uint(&reply, NFS3ERR_NOTSUPP);
			add_uint(&reply, 0);
			break;
		case 12:
			/* Remove */
			fh = get_filehandle(request, dirpath);
			path = get_string(request);
			strcpy(filepath, dirpath);
			strcat(filepath, "/");
			strcat(filepath, path);
			if (stat(filepath, &preinfo) == 0)
			{
				if (unlink(filepath) == 0)
				{
					add_uint(&reply, NFS_OK);

					stat(dirpath, &info);
					add_wcc_data(&reply, &preinfo, &info);
					
					fprintf(console, "nfsd: remove = %s on fsid=%d\n", filepath, fh->fsid);
				}
				else
				{
					add_uint(&reply, errno);
					stat(dirpath, &info);
					add_wcc_data(&reply, &preinfo, &info);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info);
			}
			break;
		case 13:
			/* RmDir */
			break;
		case 14:
			/* Rename */
			break;
		case 15:
			/* Link */
			break;
		case 16:
			/* ReadDir */
			fh = get_filehandle(request, filepath);
			offset = get_cookie3(request);
			cookieverf = get_cookie3(request);
			count = get_uint(request);
			fprintf(console, "nfsd: readdir3: offset = %d count = %d on fsid=%d\n", offset, count, fh->fsid);
			
			/* clamp to our buffer size */
			if (count > TRANSFER_SIZE)
				count = TRANSFER_SIZE;
				
			if (stat(filepath, &info) == 0)
			{
				/* account for some wrapping costs */
				count -= 32;
				d = opendir(filepath);
				if (d)
				{
					n = 0;
					add_uint(&reply, NFS_OK);
					add_post_fattr3(&reply, &info, fh->fsid);
					add_cookie3(&reply, cookieverf + 1);
					while ((dir = readdir(d)) != NULL)
					{
						/* skip if not past starting point */
						if (n >= offset)
						{
							/* entry follows */
							add_uint(&reply, 1);
							
							add_uint64(&reply, n);
	#ifdef __linux__
							add_string(&reply, dir->d_name, strlen(dir->d_name));
	#else
							add_string(&reply, dir->d_name, dir->d_namlen);
	#endif
							add_uint64(&reply, offset + n);
							/*fprintf(console, "nfsd: readdir3: %3d: %s\n", offset + n, dir->d_name);*/
						}
		
						n++;
						
						if (reply.cwp > count)
							break;
					}
					closedir(d);

					/* no entry follows */
					add_uint(&reply, 0);

					/* complete or run out of room? */
					add_uint(&reply, (reply.cwp > count) ? 0 : 1);
				}
				else
				{
					add_uint(&reply, errno);
					add_post_fattr3(&reply, &info, fh->fsid);
					fprintf(console, "nfsd: ReadDir: %s  opendir FAIL\n", filepath);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: ReadDir: %s  NFSERR_NOENT\n", filepath);
			}
			break;
		case 17:
			/* ReadDirPlus */
			fh = get_filehandle(request, filepath);
			fprintf(console, "nfsd: ReadDirPlus3: %s\n", filepath);
			add_uint(&reply, NFS3ERR_NOTSUPP);
			add_uint(&reply, 0);
			break;
		case 18:
			/* FSStat */
			fh = get_filehandle(request, filepath);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_post_fattr3(&reply, &info, fh->fsid);
	#ifndef tek
				/* fake some numbers */
				disksize = 40 * 1024 * 1024 / BLOCK_SIZE;
				freesize = 10 * 1024 * 1024 / BLOCK_SIZE;
				totalfdns = 16384;
				freefdns = 2048;
	#else
				n = open(devname, O_RDONLY);
				lseek(n, BLOCK_SIZE, SEEK_SET);
				read(n, &sirbuf, sizeof(sirbuf));
				close(n);
				disksize = (sirbuf.ssizfr[0] << 16) + (sirbuf.ssizfr[1] << 8) + (sirbuf.ssizfr[2] << 0);
				freesize = (sirbuf.sfreec[0] << 16) + (sirbuf.sfreec[1] << 8) + (sirbuf.sfreec[2] << 0);
				totalfdns = sirbuf.sszfdn;
				freefdns = sirbuf.sfdnc;
	#endif
				add_uint64(&reply, disksize);					/* Total # of blocks (of the above size) */
				add_uint64(&reply, freesize);					/* Free blocks */
				add_uint64(&reply, freesize);					/* Free blocks available to non-priv. users */

				add_uint64(&reply, totalfdns);				/* total FDNs */
				add_uint64(&reply, freefdns);					/* Free FDNs */
				add_uint64(&reply, freefdns);					/* Free FDNs available to non-priv. users */
				add_uint64(&reply, 1);								/* volatile */
				/*fprintf(console, "nfsd: FsStat3: %s\n", filepath);*/
			}
			else
			{
				add_uint(&reply, NFS3ERR_BADHANDLE);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: FsStat3: %s  FAIL\n", filepath);
			}
			break;
		case 19:
			/* FsInfo */
			fh = get_filehandle(request, filepath);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_post_fattr3(&reply, &info), fh->fsid;
				add_uint(&reply, sizeof(struct conn) + TRANSFER_SIZE);			/* rtmax */
				add_uint(&reply, sizeof(struct conn) + TRANSFER_SIZE);			/* rtpref */
				add_uint(&reply, TRANSFER_SIZE);			/* rtmult */
				add_uint(&reply, sizeof(struct conn) + TRANSFER_SIZE);			/* wtmax */
				add_uint(&reply, sizeof(struct conn) + TRANSFER_SIZE);			/* wtpref */
				add_uint(&reply, TRANSFER_SIZE);			/* wtmult */
				add_uint(&reply, TRANSFER_SIZE);			/* dtpref */
				add_uint64(&reply, 1<<23);		/* maxfilesize 8MB */
				add_uint(&reply, 1);			/* timedelta sec */
				add_uint(&reply, 0);			/* timedelta usec */
				add_uint(&reply, FSF3_LINK | FSF3_SYMLINK | FSF3_HOMOGENEOUS | FSF3_CANSETTIME);
				
				fprintf(console, "nfsd: FsInfo3: %s\n", filepath);
			}
			else
			{
				add_uint(&reply, NFS3ERR_BADHANDLE);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: FsInfo3: %s  FAIL\n", filepath);
			}
			break;
		case 20:
			/* PathConf */
			/* TODO: deal with NFS3ERR_STALE */
			fh = get_filehandle(request, filepath);
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_post_fattr3(&reply, &info, fh->fsid);
				add_uint(&reply, 128);	/* nlinks limited by unsigned char in Uniflex */
				add_uint(&reply, MAXNAMLEN);
				add_uint(&reply, 0);
				add_uint(&reply, 0);
				add_uint(&reply, 0);
				add_uint(&reply, 1);
				
				fprintf(console, "nfsd: PathConf: %s  perm:%4.4x\n", filepath, info.st_mode);
			}
			else
			{
				/* this should never happen.. */
				add_uint(&reply, NFSERR_STALE);
				add_post_fattr3(&reply, &info, fh->fsid);
				fprintf(console, "nfsd: PathConf: %s  STALE\n", filepath);
			}

			break;
		case 21:
			/* Commit */
			fh = get_filehandle(request, filepath);
 			add_uint(&reply, NFS3ERR_NOTSUPP);
			fprintf(console, "nfsd: COMMIT: %s\n", filepath);
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
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

	umask(0);
	
	/* we act as portmapd, mountd and nfsd... */
	portmapsock = create_UDP_sock(PORTMAPPERD_PORT);
	mountsock = create_UDP_sock(MOUNTD_PORT);
	nfssock = create_UDP_sock(NFSD_PORT);

	/* cannot continue (not having portmapping is tolerable) */
	if (mountsock < 0 || nfssock < 0)
	{
		fprintf(console, "cannot bind sockets\n");
		exit(-2);
	}
	
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
				n = validate(&request, NFSD);
				if (n)
				{
					if (n == 2)
						 nfsprog(&request);
					if (n == 3)
						 nfs3prog(&request);
				}
			}
		}
	}
	
}
	
