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
#ifdef TEK4404

#include <sys/sir.h>

#include <net/inet.h>
#include <net/nerrno.h>
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

#define  IPPROTO_UDP IPPR_UDP
#define socklen_t unsigned int
#define CHOWN(A,B,C) chown(A,B) /* does not have group id */

#define st_ctime st_spr	/* does not have */
#define st_atime st_spr	/* does not have */

#define NO_GROUPS	/* does not have */

#define ONLY_MTIME  /* does not have atimne or ctime */

struct sir sirbuf;

#else

/* provide all boot services */
#define SUNBOOT

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

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_dl.h>
#include <ifaddrs.h>

#include <netdb.h>
#include <libgen.h>
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
		NFS3ERR_NOTSUPP = 10004
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
#define	PORTMAPPERD 100000
#define	NFSD 100003
#define	MOUNTD 100005
#define	LOCKD 100024
#define	BOOTPARAMD 100026

/* ports for RPC progs */
#define PORTMAPPERD_PORT 111
#define MOUNTD_PORT 6135
#define NFSD_PORT 2049
#define LOCKD_PORT 41045
#define BOOTPARAMD_PORT 2050

/* logging credential details */
#define LOGCREDS 0


FILE *console;

uint8_t host_mac[6];
struct in_addr host_assigned;
char host_name[256];

/* cmdline args:  nfsd -base /Users/dodah -machinename sparc2 -mac XX:XX:XX:XX:XX:XX -addr 192.168.1.71 -fs /Users/Shared/export/root -swap /Users/Shared/export/swap */
uint8_t rarp_machinemac[6];
char tftp_base[256];
char bp_machinename[256];
char bp_addr[64];
char bp_fs[256];
char bp_swap[256];
char bp_dump[256];

#ifdef TEK4404
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

int truncate(filepath, len)
char *filepath;
int len;
{
	int fd;

	fd = open(filepath, O_RDWR);
	if (fd > 0)
	{
		lseek(fd, len, SEEK_SET);
		truncf(fd);
		close(fd);
	}

	return fd;
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

void add_uint16(reply, val)
struct response *reply;
unsigned short val;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->cwp);

	*ptr = htons(val);
	reply->cwp += sizeof(val);
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

void add_ipv4(reply, ipv4)
struct response *reply;
unsigned int ipv4;
{
	add_uint(reply, 1);
	add_uint(reply, (ipv4>>24) & 0xff);
	add_uint(reply, (ipv4>>16) & 0xff);
	add_uint(reply, (ipv4>>8) & 0xff);
	add_uint(reply, (ipv4>>0) & 0xff);
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

/* always 4 byte boundary */
#define ROUNDLEN(A) ((A + 3) & -4)

void add_filehandle(reply, fh)
struct response *reply;
struct filehandle *fh;
{
	unsigned int *ptr = (unsigned int *)(reply->buffer + reply->cwp);
	int len = sizeof(struct filehandle);

	memcpy(ptr, fh, len);
	len = ROUNDLEN(len);
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
	len = ROUNDLEN(len);
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
	len = ROUNDLEN(len);
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
	rc = ROUNDLEN(rc);
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
	
	add_uint(reply, len);	/* bytes read */
	add_uint(reply, 0);		/* is EOF */
	add_uint(reply, 0);		/* variable length array */
	ptr = (unsigned int *)(reply->buffer + reply->cwp);
	
	/* clamp to remaining space */
	if (len > (TRANSFER_SIZE - reply->cwp))
		len = TRANSFER_SIZE - reply->cwp;
	
	rc = read(fd, ptr, len);
	ptr[-3] = htonl(rc);				/* what we actually read */
	ptr[-2] = (rc < len);				/* eof */
	
	/* variable length array */
	rc = ROUNDLEN(rc);
	ptr[-1] = htonl(rc);
	reply->cwp += rc;
	
	return rc;
}

int add_length_marker(reply)
struct response *reply;
{
	int lomark = reply->cwp;
	add_uint(reply, 0);	/* filled later */
	return lomark;
}

/* update at lomark, the final length */
void update_length(reply, lomark)
struct response *reply;
int lomark;
{
		unsigned int *ptr = (unsigned int *)(reply->buffer + lomark);
		*ptr = htonl(reply->cwp - lomark - 4);		/**/
}

unsigned int nfsmode2host(nfsmode)
unsigned int nfsmode;
{
	unsigned int perms = 0;

	if (nfsmode == -1)
		perms = -1;
	else
	{
		if (nfsmode & SUID)
			perms |= S_ISUID;
		if (nfsmode & DIR_NFS)
			perms |= S_IFDIR;
		if (nfsmode & REG)
			perms |= S_IFREG;
		if (nfsmode & CHR)
			perms |= S_IFCHR;
		if (nfsmode & BLK)
			perms |= S_IFBLK;
#ifndef TEK4404
		if (nfsmode & LNK)
			perms |= S_IFLNK;
#endif

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
#ifndef TEK4404
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

	if ((hostperms & S_ISUID) == S_ISUID)
		nfsperms |= SUID;

	if ((hostperms & S_IFDIR) == S_IFDIR)
		nfsperms |= DIR_NFS;
	if ((hostperms & S_IFREG) == S_IFREG)
		nfsperms |= REG;
	if ((hostperms & S_IFCHR) == S_IFCHR)
		nfsperms |= CHR;
	if ((hostperms & S_IFBLK) == S_IFBLK)
		nfsperms |= BLK;
#ifndef TEK4404
	if ((hostperms & S_IFLNK) == S_IFLNK)
		nfsperms |= LNK;
#endif

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
#ifndef TEK4404
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
#ifdef NO_GROUPS
		add_uint(reply, info->st_uid);
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
#ifdef NO_GROUPS
		add_uint(reply, info->st_uid);
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
#ifdef ONLY_MTIME
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
#else
	add_nfstime(reply, (unsigned int)info->st_atime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_ctime);
#endif
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
#ifdef NO_GROUPS
		add_uint(reply, info->st_uid);
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
#ifdef NO_GROUPS
		add_uint(reply, info->st_uid);
#else
		add_uint(reply, info->st_gid);
#endif
		add_uint64(reply, (unsigned int)info->st_size);
		add_uint64(reply, ((unsigned int)info->st_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
	}
	add_uint64(reply, 0);														/* specdata3 */
	add_uint64(reply, fsid);
	add_uint64(reply, (unsigned int)info->st_ino);	/* fileid */
#ifdef ONLY_MTIME
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
#else
	add_nfstime(reply, (unsigned int)info->st_atime);
	add_nfstime(reply, (unsigned int)info->st_mtime);
	add_nfstime(reply, (unsigned int)info->st_ctime);
#endif
}

/* USES: 88 */
void add_post_fattr3(reply, info, fsid)
struct response *reply;
struct stat *info;
int fsid;
{
	if (info)
	{
		add_uint(reply, 1);
		add_fattr3(reply, info, fsid);
	}
	else
	{
		add_uint(reply, 0);
	}
}

void add_post_wccattr3(reply, info)
struct response *reply;
struct stat *info;
{
	add_uint(reply, 1);
	add_uint64(reply, (unsigned int)info->st_size);
	add_nfstime(reply, (unsigned int)info->st_mtime);
#ifdef ONLY_TIME
	add_nfstime(reply, (unsigned int)info->st_mtime);
#else
	add_nfstime(reply, (unsigned int)info->st_ctime);
#endif
}

void add_wcc_data(reply, preinfo, postinfo, fsid)
struct response *reply;
struct stat *preinfo;
struct stat *postinfo;
int fsid;
{
	if (preinfo)
		add_post_wccattr3(reply, preinfo);
	else
		add_uint(reply, 0);

	if (postinfo)
		add_post_fattr3(reply, postinfo, fsid);
	else
		add_uint(reply, 0);
}

int validate(request, prognum)
struct conn *request;
int prognum;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;

/*	fprintf(console,"RPC: xid:%8.8x rpcvers:%d vers:%d prog:%d proc:%d msg:%d\015\012", ntohl(header->xid), ntohl(header->rpcvers), ntohl(header->vers), ntohl(header->prog), ntohl(header->proc), ntohl(header->msg_type));
*/
	reply.cwp = 0;
	if (ntohl(header->msg_type) != CALL)
	{
		fprintf(console, "validate: corrupt call %d\015\012", ntohl(header->msg_type));
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
		fprintf(console, "validate: wrong ver %d\015\012", ntohl(header->rpcvers));
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
		fprintf(console, "validate: wrong prognum %d  wanted %d\015\012", ntohl(header->prog), prognum);
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

unsigned int get_uint16(request)
struct conn *request;
{
	unsigned short *ptr = (unsigned short *)(request->buffer + request->crp);
	unsigned short val = ntohs(*ptr);
	request->crp += sizeof(val);

	return val;
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
	unsigned int val;
	get_uint(request);
	val = get_uint(request);
	return val;
}

#define get_cookie3 get_uint64

unsigned int get_ipv4(request)
struct conn *request;
{
	unsigned int ipv4 = 0;
	
	int atype = get_uint(request);

	ipv4 <<= 8; ipv4 |= get_uint(request) & 0xff;
	ipv4 <<= 8; ipv4 |= get_uint(request) & 0xff;
	ipv4 <<= 8; ipv4 |= get_uint(request) & 0xff;
	ipv4 <<= 8; ipv4 |= get_uint(request) & 0xff;
	return ipv4;
}

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
			
			/* only from expected machinename */
			if (bp_machinename[0])
			{
				if (strcmp(bp_machinename, ptr + 2))
				{
					fprintf(console, "get_credentials: REJECT unknown machinename\n");
					return;
				}
			}
						
			ptr += n;
			fprintf(console, "get_credentials: uid:%d gid:%d\n", ntohl(ptr[2]), ntohl(ptr[3]));

			n = ntohl(ptr[4]);
			if (n < 64)
			{
				fprintf(console, "get_credentials: gids: [ ");
				while(n--)
					fprintf(console, "%d ",ntohl(ptr[5+n]));
				fprintf(console, "]\n");
			}
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
	if (filepath)
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
	len = ROUNDLEN(len);
	request->crp += len;
	
	return name;
}

void get_sattr(request, info)
struct conn *request;
struct stat *info;
{
		info->st_mode = nfsmode2host(get_uint(request));
		info->st_uid = get_uint(request);
#ifdef NO_GROUPS
		get_uint(request);
#else
		info->st_gid = get_uint(request);
#endif
		info->st_size = get_uint(request);

#ifdef TEK4404
		get_uint(request);	get_uint(request);	/* no access time */
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
	
	info->st_mode = 0xffff;
	if (get_uint(request))
		info->st_mode = nfsmode2host(get_uint(request));
		
	info->st_uid = -1;
	if (get_uint(request))
		info->st_uid = get_uint(request);

#ifdef NO_GROUPS
	if (get_uint(request))
			get_uint(request);
#else
	info->st_gid = -1;
	if (get_uint(request))
			info->st_gid = get_uint(request);
#endif

	info->st_size = -1;
	if (get_uint(request))
	{
		get_uint(request); info->st_size = get_uint(request);
	}
	
	time_how = get_uint(request);
#ifdef TEK4404
	if (time_how == SET_TO_CLIENT_TIME)
	{
		get_uint(request);	get_uint(request);	/* no access time */
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
#ifdef ONLY_MTIME
	if (get_uint(request))
	{
		get_uint(request); get_uint(request);
	}
#else
	info->st_ctime = -1;
	if (get_uint(request))
	{
		get_uint(request); info->st_ctime = get_uint(request);
	}
#endif
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

int create_UDP_sock(daemonname, port)
char *daemonname;
int port;
{
	struct in_sockaddr serv_addr;
	int sock,n;
		
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		fprintf(console, "socket: %s: %s\n",daemonname, strerror(errno));
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = host_assigned.s_addr ;		// INADDR_ANY;
	serv_addr.sin_port = htons(port);
	n = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
	if (n < 0) {
		fprintf(console, "bind: %s: port %d: %s\n", daemonname, port, strerror(errno));
		close(sock);
		return -2;
	}

	fprintf(console,"%s listening on %d\n", daemonname, port);
	return sock;
}

void mountprog(request,isinternal)
struct conn *request;
int isinternal;
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

	if (isinternal)
		add_uint(&reply, MOUNTD_PORT);
	
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
			fprintf(console, "mountd: Mount Entries for client@%s\n",  inet_ntoa((request->from.sin_addr)) );
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
	/*fprintf(console, "mountd: replied %d bytes\n", n);*/
}

void lockprog(request,isinternal)
struct conn *request;
int isinternal;
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

	if (isinternal)
		add_uint(&reply, LOCKD_PORT);
	
	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* Test */
			add_uint(&reply, NFS_OK);
			break;
		case 2:
			/* Lock */
			path = get_string(request);
			add_uint(&reply, NFS_OK);
			break;
		case 3:
			/* Cancel */
			path = get_string(request);
			add_uint(&reply, NFS_OK);
			break;
		case 4:
			/* Unlock */
			path = get_string(request);
			add_uint(&reply, NFS_OK);
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
	{
			fprintf(console, "mountd: sendto: %s\n",strerror(errno));
	}
}

void nfsprog(request,isinternal)
struct conn *request;
int isinternal;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info,reqinfo;
	char *path;
	char filepath[1024];
	int disksize,freesize;
	int n, count, offset, fd, rc, len;
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

	if (isinternal)
		add_uint(&reply, NFSD_PORT);
	
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
				fprintf(console, "nfsd: get_attr: %s  perm:%4.4x  size:%d\n", filepath, info.st_mode, info.st_size);
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
			if (reqinfo.st_mode != 0xffff)
				chmod(filepath, reqinfo.st_mode);
			if ((int)reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if ((int)reqinfo.st_size != -1)
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
						fprintf(console, "  write: %s: %d bytes at %d\n", filepath, rc, offset);
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
			if (info.st_mode != 0xffff)
				chmod(filepath, info.st_mode);
			if ((int)info.st_uid != -1)
				CHOWN(filepath, info.st_uid, info.st_gid);
			if ((int)info.st_size != -1)
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
				fprintf(console, "nfsd: mkdir = %s\n", filepath);
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
			/* fprintf(console, "nfsd: READDIR: offset = %d count = %d\n", offset, count); */
			
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
					n++;

					/* skip if not past starting point */
					if (n > offset)
					{
#ifdef __linux__
						len = strlen(dir->d_name);
#else
						len = dir->d_namlen;
#endif
						/* enough space? */
						if (reply.cwp + ROUNDLEN(len) + 16 > count)
						{
							count = 0;
							break;
						}
						
						/* entry follows */
						add_uint(&reply, 1);
						
						add_uint(&reply, n);	/* fileid */
						add_string(&reply, dir->d_name, len);
						add_uint(&reply, n);
						fprintf(console, "nfsd: READDIR: %3d: cwp(%d) %s\n", n, reply.cwp, dir->d_name);
					}
				}
				closedir(d);

				/* no entry follows */
				add_uint(&reply, 0);

				/* complete or run out of room? */
				add_uint(&reply, (count) ? 1 : 0);
				fprintf(console, "nfsd: READDIR: eof(%d): cwp(%d)\n", (count) ? 1 : 0, reply.cwp);
			}
			break;
		case 17:
			/* StatFS */
			add_uint(&reply, NFS_OK);
			add_uint(&reply, TRANSFER_SIZE);			/* tsize: optimum transfer size */
			add_uint(&reply, BLOCK_SIZE);			/* Block size of FS */
#ifdef TEK4404
			n = open("/dev/disk", O_RDONLY);
			lseek(n, BLOCK_SIZE, SEEK_SET);
			read(n, &sirbuf, sizeof(sirbuf));
			close(n);
			disksize = (sirbuf.ssizfr[0] << 16) + (sirbuf.ssizfr[1] << 8) + (sirbuf.ssizfr[2] << 0);
			freesize = (sirbuf.sfreec[0] << 16) + (sirbuf.sfreec[1] << 8) + (sirbuf.sfreec[2] << 0);
#else
			/* fake some numbers */
			disksize = 64 * 1024 * 1024 / BLOCK_SIZE;
			freesize = 56 * 1024 * 1024 / BLOCK_SIZE;
#endif
			add_uint(&reply, disksize);					/* Total # of blocks (of the above size) */
			add_uint(&reply, freesize);					/* Free blocks */
			add_uint(&reply, freesize);					/* Free blocks available to non-priv. users */
			fprintf(console, "nfsd: statfs: disk:%d free:%d\n", disksize, freesize);
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

void nfs3prog(request,isinternal)
struct conn *request;
int isinternal;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	struct stat info, reqinfo,preinfo;
	char *path;
	char dirpath[1024];
	char filepath[1024];
	int disksize,freesize,totalfdns,freefdns;
	int n, count, offset, fd, rc, how, cookieverf, len, maxcount;
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

	if (isinternal)
		add_uint(&reply, NFSD_PORT);
		
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
				/* fprintf(console, "nfsd: GETATTR3: %s  perm:%4.4x\n", filepath, info.st_mode); */
			}
			else
			{
				/* this should never happen.. */
				add_uint(&reply, NFS3ERR_BADHANDLE);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: GETATTR3: %s  perm:%4.4x FAILED\n", filepath, info.st_mode);
			}
			break;
		case 2:
			/* SetAttr */
			fh = get_filehandle(request, filepath);
			get_sattr3(request, &reqinfo);
			get_sattrguard3(request, &reqinfo);
			/* fprintf(console, "nfsd: SETATTR3: mode=%x uid=%d size=%ld\n",reqinfo.st_mode,reqinfo.st_uid,reqinfo.st_size); */
			if (reqinfo.st_mode != 0xffff)
				chmod(filepath, reqinfo.st_mode);
			if ((int)reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if ((int)reqinfo.st_size != -1)
				truncate(filepath, reqinfo.st_size);

			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_wcc_data(&reply, &preinfo, &info, fh->fsid);
				/* fprintf(console, "nfsd: SETATTR3 = %s mode=%4.4x size=%d\n", filepath, info.st_perm, info.st_size); */
			}
			else
			{
				add_uint(&reply, errno);
				add_wcc_data(&reply, &preinfo, NULL, fh->fsid);
				fprintf(console, "nfsd: SETATTR3 = %s FAIL *****\n", filepath);
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
				/*fprintf(console, "nfsd: LOOKUP3 = %s on fsid=%d\n", filepath, fh->fsid);*/
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
					/*fprintf(console, "nfsd: ACCESS3 = %s perm=%4.4x on fsid=%d\n", filepath, n, fh->fsid);*/
			}
			else
			{
					add_uint(&reply, NFSERR_NOENT);
					add_post_fattr3(&reply, NULL, fh->fsid);
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
					add_post_fattr3(&reply, &info, fh->fsid);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_post_fattr3(&reply, NULL, fh->fsid);
			}
			break;
		case 7:
			/* Write */
			fh = get_filehandle(request, filepath);
			offset = get_uint64(request);
			count = get_uint(request);
			how = get_uint(request);
			n = get_uint(request);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &preinfo) == 0)
			{
				if ((preinfo.st_mode & S_IFREG) == S_IFREG)
				{
					if (how == 0)	/* UNSTABLE */
					{
						fprintf(console, "nfsd: WRITE3: UNSTABLE mode ignored\n");
					}
				
					if (preinfo.st_perm & S_IWRITE)
					{
						rc = 0;
						if (count > 0)
						{
							fd = open(filepath, O_WRONLY);
							rc = lseek(fd, offset, SEEK_SET);
							rc = write(fd, request->buffer + request->crp, count);
							close(fd);
						}

						if (rc >= 0)
						{
							add_uint(&reply, NFS_OK);

							stat(filepath, &info);
							add_wcc_data(&reply, &preinfo, &info, fh->fsid);
							add_uint(&reply, rc);
							add_uint(&reply, 2);			/* FILE_SYNC */
							add_uint64(&reply, 0);		/* writeverf3 */
						}
						else
						{
							add_uint(&reply, NFSERR_IO);
							stat(filepath, &info);
							add_wcc_data(&reply, &preinfo, &info, fh->fsid);
							fprintf(console, "nfsd: WRITE3: NFSERR_IO\n");
						}
					}
					else
					{
						add_uint(&reply, NFSERR_ACCES);
						add_wcc_data(&reply, &preinfo, &preinfo, fh->fsid);
						fprintf(console, "nfsd: WRITE3: NFSERR_ACCES\n");
					}
				}
				else
				{
					add_uint(&reply, NFSERR_ISDIR);
					add_wcc_data(&reply, &preinfo, NULL, fh->fsid);
					fprintf(console, "nfsd: WRITE3: NFSERR_ISDIR\n");
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_wcc_data(&reply, NULL, NULL, fh->fsid);
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
				fprintf(console, "nfsd: CREATE3: %s  NFSERR_NOENT\n", dirpath);
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
				/* fprintf(console, "nfsd: CREATE3: mode=%x uid=%d size=%d\n",reqinfo.st_mode,reqinfo.st_uid,reqinfo.st_size); */
			}
			else
			{
				add_uint(&reply, NFS3ERR_NOTSUPP);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: CREATE3: %s  NFS3ERR_NOTSUPP\n", dirpath);
				break;
			}

			if (how == GUARDED)
			{
				if (stat(filepath, &info) == 0)
				{
					add_uint(&reply, NFSERR_EXIST);
					add_post_fattr3(&reply, &info, fh->fsid);
					fprintf(console, "nfsd: CREATE3: %s  NFS3ERR_EXIST\n", filepath);
					break;
				}
			}

			fd = creat(filepath, reqinfo.st_mode);
			close(fd);
			if (reqinfo.st_mode != 0xffff)
				chmod(filepath, reqinfo.st_mode);
			if ((int)reqinfo.st_uid != -1)
				CHOWN(filepath, reqinfo.st_uid, reqinfo.st_gid);
			if ((int)reqinfo.st_size != -1)
				truncate(filepath, reqinfo.st_size);

			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				make_filehandle(filepath, &info, &handle);
				handle.fsid = fh->fsid;
				add_uint(&reply, NFS_OK);
				add_post_filehandle(&reply, &handle);
				add_post_fattr3(&reply, &info, fh->fsid);
				/* fprintf(console, "nfsd: CREATE3 = %s perm:%4.4x on fsid=%d\n", filepath, info.st_mode, fh->fsid); */

				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info, fh->fsid);
			}
			else
			{
				add_uint(&reply, NFSERR_ACCES);
				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info, fh->fsid);
			}
			break;
		case 9:
			/* MkDir */
			fh = get_filehandle(request, dirpath);
			if (stat(dirpath, &preinfo) < 0)
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: MKDIR3: %s  NFSERR_NOENT\n", dirpath);
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
				add_wcc_data(&reply, &preinfo, &info, fh->fsid);
				/*fprintf(console, "nfsd: MKDIR3 = %s on fsid=%d\n", filepath, fh->fsid);*/
			}
			else
			{
				add_uint(&reply, NFSERR_ACCES);
				stat(dirpath, &info);
				add_wcc_data(&reply, &preinfo, &info, fh->fsid);
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
			if (stat(dirpath, &preinfo) < 0)
			{
				add_uint(&reply, NFSERR_NOENT);
				add_uint(&reply, 0);
				fprintf(console, "nfsd: REMOVE3: %s  NFSERR_NOENT\n", dirpath);
				break;
			}
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
					add_wcc_data(&reply, &preinfo, &info, fh->fsid);
					
					/*fprintf(console, "nfsd: REMOVE3 = %s on fsid=%d\n", filepath, fh->fsid);*/
				}
				else
				{
					add_uint(&reply, errno);
					stat(dirpath, &info);
					add_wcc_data(&reply, &preinfo, &info, fh->fsid);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				stat(dirpath, &info);
				add_wcc_data(&reply, NULL, &info, fh->fsid);
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
			/* ReadDir3 */
			fh = get_filehandle(request, dirpath);
			offset = get_cookie3(request);
			cookieverf = get_cookie3(request);
			count = get_uint(request);
			/*fprintf(console, "nfsd: READDIR3: offset = %d count = %d on fsid=%d\n", offset, count, fh->fsid);*/
			
			/* clamp to our buffer size */
			if (count > TRANSFER_SIZE)
				count = TRANSFER_SIZE;
				
			if (stat(dirpath, &info) == 0)
			{
				/* verify cookie */
				if (cookieverf && (cookieverf != (unsigned int)info.st_mtime))
				{
					fprintf(console, "nfsd: READDIR3: verify FAIL %d %d\n",cookieverf, (unsigned int)info.st_mtime);
					add_uint(&reply, NFS3ERR_BAD_COOKIE);
					add_post_fattr3(&reply, NULL, fh->fsid);
					break;
				}

				/* account for some wrapping costs */
				count -= 32;
				d = opendir(dirpath);
				if (d)
				{
					n = 0;
					add_uint(&reply, NFS_OK);
					add_post_fattr3(&reply, &info, fh->fsid);
					add_cookie3(&reply, (unsigned int)info.st_mtime);
					while ((dir = readdir(d)) != NULL)
					{
		
						n++;

						/* skip if not past starting point */
						if (n > offset)
						{
#ifdef __linux__
							len = strlen(dir->d_name);
#else
							len = dir->d_namlen;
#endif
							/* enough space? */
							if (reply.cwp + ROUNDLEN(len) + 24 > count)
							{
								count = 0;
								break;
							}

							/* entry follows */
							add_uint(&reply, 1);
							
							add_uint64(&reply, n);	/* fileid */
							add_string(&reply, dir->d_name, len);
							add_uint64(&reply, n);
							/*fprintf(console, "nfsd: readdir3: %3d: %s\n", offset + n, dir->d_name);*/
						}
					}
					closedir(d);

					/* no entry follows */
					add_uint(&reply, 0);

					/* complete or run out of room? */
					add_uint(&reply, (count) ? 1 : 0);
				}
				else
				{
					add_uint(&reply, errno);
					add_post_fattr3(&reply, &info, fh->fsid);
					fprintf(console, "nfsd: READDIR3: %s  opendir FAIL\n", dirpath);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_post_fattr3(&reply, NULL, fh->fsid);
				fprintf(console, "nfsd: READDIR3: %s  NFSERR_NOENT\n", dirpath);
			}
			break;
		case 17:
			/* ReadDirPlus */
			fh = get_filehandle(request, dirpath);
			offset = get_cookie3(request);
			cookieverf = get_cookie3(request);
			count = get_uint(request);
			maxcount = get_uint(request);
			/* fprintf(console, "nfsd: READDIRPLUS3: offset:%d cookie:%d count=%d maxcount:%d on fsid=%d\n", offset, cookieverf, count, maxcount, fh->fsid); */
#if 0
			/* uNFS skips support because of lack of atomicity of getting info.. do we care`? */
			add_uint(&reply, NFS3ERR_NOTSUPP);
			add_uint(&reply, 0);
#else
			/* clamp to our buffer size */
			if (maxcount > TRANSFER_SIZE)
				maxcount = TRANSFER_SIZE;
				
			if (stat(dirpath, &info) == 0)
			{
				/* verify cookie */
				if (cookieverf && (cookieverf != (unsigned int)info.st_mtime))
				{
					fprintf(console, "nfsd: READDIRPLUS3: verify FAIL %d %d\n",cookieverf, (unsigned int)info.st_mtime);
					add_uint(&reply, NFS3ERR_BAD_COOKIE);
					add_post_fattr3(&reply, NULL, fh->fsid);
					break;
				}

				/* account for some wrapping costs */
				maxcount -= 32;

				d = opendir(dirpath);
				if (d)
				{
					n = 0;
					
					add_uint(&reply, NFS_OK);
					add_post_fattr3(&reply, &info, fh->fsid);
					add_cookie3(&reply, (unsigned int)info.st_mtime);
					while ((dir = readdir(d)) != NULL)
					{
		
						n++;
						
						/* skip if not past starting point */
						if (n > offset)
						{
#ifdef __linux__
							len = strlen(dir->d_name);
#else
							len = dir->d_namlen;
#endif
							/* enough space? */
							if (reply.cwp + ROUNDLEN(len) + 152 > maxcount)
							{
								count = 0;
								break;
							}
								
							/* entry follows */
							add_uint(&reply, 1);
							
							strcpy(filepath, dirpath);
							strcat(filepath, "/");
							strncat(filepath, dir->d_name, len);
							if (stat(filepath, &info) == 0)
							{
								add_uint64(&reply, (unsigned int)info.st_ino);
								add_string(&reply, dir->d_name, len);
								add_uint64(&reply, n);
								
								make_filehandle(filepath, &info, &handle);
								handle.fsid = fh->fsid;
								add_post_fattr3(&reply, &info, fh->fsid);
								add_post_filehandle(&reply, &handle);
								/* fprintf(console, "nfsd: READDIRPLUS3: %3d: cwp(%d) %s\n", n, reply.cwp, dir->d_name); */
							}
							else
							{
								add_uint64(&reply, 0);
								add_string(&reply, dir->d_name, len);
								add_uint64(&reply, n);
								
								add_uint(&reply, 0);	/* post_fattr3 */
								add_uint(&reply, 0);	/* post_filehandle */
							}
						}
					}
					closedir(d);

					/* no entry follows */
					add_uint(&reply, 0);

					/* complete or run out of room? */
					add_uint(&reply, (count) ? 1 : 0);
					/* fprintf(console, "nfsd: READDIRPLUS3: eof(%d) cwp(%d)\n",(count) ? 1 : 0, reply.cwp);*/
				}
				else
				{
					add_uint(&reply, errno);
					add_post_fattr3(&reply, &info, fh->fsid);
					fprintf(console, "nfsd: READDIRPLUS3: %s  opendir FAIL\n", dirpath);
				}
			}
			else
			{
				add_uint(&reply, NFSERR_NOENT);
				add_post_fattr3(&reply, NULL, fh->fsid);
				fprintf(console, "nfsd: READDIRPLUS3: %s  NFSERR_NOENT\n", dirpath);
			}
#endif
			break;
		case 18:
			/* FSStat */
			fh = get_filehandle(request, filepath);
			memset(&info, 0, sizeof(info));
			if (stat(filepath, &info) == 0)
			{
				add_uint(&reply, NFS_OK);
				add_post_fattr3(&reply, &info, fh->fsid);
#ifdef TEK4404
				n = open("/dev/disk", O_RDONLY);
				lseek(n, BLOCK_SIZE, SEEK_SET);
				read(n, &sirbuf, sizeof(sirbuf));
				close(n);
				disksize = (sirbuf.ssizfr[0] << 16) + (sirbuf.ssizfr[1] << 8) + (sirbuf.ssizfr[2] << 0);
				freesize = (sirbuf.sfreec[0] << 16) + (sirbuf.sfreec[1] << 8) + (sirbuf.sfreec[2] << 0);
				totalfdns = sirbuf.sszfdn;
				freefdns = sirbuf.sfdnc;
#else
				/* fake some numbers */
				disksize = 40 * 1024 * 1024 / BLOCK_SIZE;
				freesize = 10 * 1024 * 1024 / BLOCK_SIZE;
				totalfdns = 16384;
				freefdns = 2048;
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
				add_uint(&reply, 0);
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
				add_uint(&reply, TRANSFER_SIZE);			/* rtpref */
				add_uint(&reply, TRANSFER_SIZE);			/* rtmult */
				add_uint(&reply, sizeof(struct conn) + TRANSFER_SIZE);			/* wtmax */
				add_uint(&reply,  TRANSFER_SIZE);			/* wtpref */
				add_uint(&reply, TRANSFER_SIZE);			/* wtmult */
				add_uint(&reply, TRANSFER_SIZE);			/* dtpref */
				add_uint64(&reply, 1<<23);		/* maxfilesize 8MB */
				add_uint(&reply, 1);			/* timedelta sec */
				add_uint(&reply, 0);			/* timedelta usec */
				add_uint(&reply, FSF3_LINK | FSF3_SYMLINK | FSF3_HOMOGENEOUS | FSF3_CANSETTIME);
				
				/* fprintf(console, "nfsd: FsInfo3: %s\n", filepath); */
			}
			else
			{
				add_uint(&reply, NFS3ERR_BADHANDLE);
				add_uint(&reply, 0);
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
				
				/* fprintf(console, "nfsd: PathConf: %s  perm:%4.4x\n", filepath, info.st_mode); */
			}
			else
			{
				/* this should never happen.. */
				add_uint(&reply, NFSERR_STALE);
				add_uint(&reply, 0);
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

void bootparamprog(request,isinternal)
struct conn *request;
int isinternal;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	char hostname[256],pathname[256];
	char path[256];
	int n,atype,lomark;
	uint32_t ipv4;
	
	get_credentials(request, 0);
	get_verifier(request);
	
	reply.cwp = 0;
	add_uint(&reply, ntohl(header->xid));
	add_uint(&reply, REPLY);
	add_uint(&reply, MSG_ACCEPTED);
	add_uint(&reply, 0);		/* opaque_verf */
	add_uint(&reply, 0);		/* opaque_verf size */
	add_uint(&reply, SUCCESS);

	if (isinternal)
		add_uint(&reply, BOOTPARAMD_PORT);
	
	switch(ntohl(header->proc))
	{
		case 0:
			/* NULL-op */
			break;
		case 1:
			/* WHOAMI */
			if (isinternal)
				n = get_uint(request);

			ipv4 = get_ipv4(request);	/* ss2 passes 4 uint32 for address.. */
			fprintf(console, "bootparamd: whoami:%8.8X\n", ipv4);

			if (isinternal)
				lomark = add_length_marker(&reply);

			add_string(&reply, bp_machinename, strlen(bp_machinename));
			add_string(&reply, host_name, strlen(host_name));	/* domain */
			add_ipv4(&reply, htonl(inet_addr("192.168.1.1")));
			fprintf(console, "bootparamd: whoami:%8.8X => machinename:%s domain:%s address:%s\n", ipv4, bp_machinename, host_name, "192.168.1.1");

			if (isinternal)
				update_length(&reply, lomark);

			break;
		case 2:
			/* GETFILE */
			if (isinternal)
				n = get_uint(request);

			strcpy(hostname, get_string(request));
			strcpy(pathname, get_string(request));
			fprintf(console, "bootparamd: getfile: client:%s asking for %s\n", hostname, pathname);

			/* is the hostname we have info about? */
			if (!strcmp(hostname, bp_machinename))
			{
				char *result;
				
				if (isinternal)
					lomark = add_length_marker(&reply);

				add_string(&reply, inet_ntoa(host_assigned), strlen(inet_ntoa(host_assigned)));		/* server name */
				add_ipv4(&reply, htonl(host_assigned.s_addr));				/* server address */

				result = NULL;
				if (!strcmp(pathname, "root"))
				{
					result = bp_fs;
				}
				else
				if (!strcmp(pathname, "swap"))
				{
					result = bp_swap;
				}
				else
				if (!strcmp(pathname, "dump"))
				{
					result = bp_dump;
				}

				if (result)
				{
					add_string(&reply, result, strlen(result));
					fprintf(console, "bootparamd: getfile: client:%s %s => %s\n", hostname, pathname, result);
				}
				
				if (isinternal)
					update_length(&reply, lomark);
			}
			break;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
	{
			fprintf(console, "bootparamd: sendto: %s\n",strerror(errno));
	}
	/*fprintf(console, "bootparamd: replied %d bytes\n", n);*/
}

/* portmapper is special and can invoke other progs */
void portmapperprog(request)
struct conn *request;
{
	struct rpcheader *header = (struct rpcheader *)request->buffer;
	struct response reply;
	unsigned int prog, vers, prot, port, registeredport;
	unsigned int proc;
	char *lomark,*himark;
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
			if (prot == IPPROTO_UDP)
			{
				if (prog == NFSD) registeredport = NFSD_PORT;
				if (prog == MOUNTD) registeredport = MOUNTD_PORT;
				if (prog == BOOTPARAMD) registeredport = BOOTPARAMD_PORT;
				if (prog == LOCKD && vers == 4) registeredport = LOCKD_PORT;
			}

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

		case 5:
			/* Call-It */
			lomark = request->buffer + request->crp;
			prog = get_uint(request);
			vers = get_uint(request);
			proc = get_uint(request);
			himark = request->buffer + request->crp;
			fprintf(console, "portmapd: CALLIT: prog:%d vers:%d proc:%d \n", prog, vers, proc);

			/* roll back buffer */
			while(lomark < request->buffer+request->len)
			{
				*lomark++ = *himark++;
			}

			/* edit header */
			request->crp = sizeof(struct rpcheader);
			header->proc = ntohl(proc);
			
			if (prog == MOUNTD) mountprog(request, NFS_TRUE);
			if (prog == LOCKD) lockprog(request, NFS_TRUE);
			if (prog == BOOTPARAMD) bootparamprog(request, NFS_TRUE);
			if (prog == NFSD)
			{
				if (vers == 2)
					nfsprog(request, NFS_TRUE);
				if (vers == 3)
					nfs3prog(request, NFS_TRUE);
			}
			return;
	}

	n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	if(n != reply.cwp)
	{
			fprintf(console, "portmapd: sendto: %s\n",strerror(errno));
	}
}

#ifdef SUNBOOT
/* https://www.rfc-editor.org/info/rfc1350/ */
#define TFTP_RRQ 1
#define TFTP_DATA 3
#define TFTP_ACK 4
#define TFTP_ERROR 5
void tftp(request)
struct conn *request;
{
	int opcode = get_uint16(request);
	char *filepath = request->buffer + request->crp;
	char *mode = filepath + strlen(filepath) + 1;
	char fullpath[256];
	sprintf(fullpath, "%s/%s", tftp_base, filepath);

	/* only offer RRQ */
	if (opcode != TFTP_RRQ)
		return;
		
	fprintf(console, "tftpd: opcode %d for filename: %s using mode: %s\n", opcode, fullpath, mode);
	
	FILE *fp;
	fp = fopen(fullpath, "rb");
	if (fp)
	{
		unsigned char buffer[1024];
		int blocknum = 1;
		int total = 0;
		int n;
		
		while(blocknum)
		{
			struct response reply;
			socklen_t fromSize = sizeof(request->from);

			n = fread(buffer, 1, 512, fp);
			if (n <= 0)
				break;

			total += n;
			reply.cwp = 0;
			add_uint16(&reply, TFTP_DATA);
			add_uint16(&reply, blocknum);
			memcpy(reply.buffer+reply.cwp, buffer, n);
			reply.cwp += n;
			n = sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
			if(n != reply.cwp)
			{
					fprintf(console, "tftpd: sendto: %s\n",strerror(errno));
					break;
			}

			/* ACK */
			n = recvfrom(request->sock, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&request->from, &fromSize);
			if (n > 0)
			{
				unsigned short *ptr = (unsigned short *)buffer;
				opcode = ntohs(*ptr++);
				n = ntohs(*ptr++);
				if (opcode != TFTP_ACK || n != blocknum)
				{
					fprintf(console, "tftpd: ACK mismatch: got:%d exp:%d\n", n, blocknum);
					
					/* TODO: send ERROR packet */
					break;
				}
			}
			
			blocknum++;
		}
		fclose(fp);
		fprintf(console, "tftpd: sent %d bytes\n", total);
	}
	else
	{
			struct response reply;

			reply.cwp = 0;
			add_uint16(&reply, TFTP_ERROR);
			add_uint16(&reply, 1);
			strcpy(reply.buffer+reply.cwp, "File not found.");
			reply.cwp += 16;
			sendto(request->sock, reply.buffer, reply.cwp, 0, (struct sockaddr *) &request->from, sizeof(request->from));
	}
}

#ifdef __clang__
#pragma pack(push, 1)
#endif
struct eth2
{
	uint8_t destmac[6];
	uint8_t srcmac[6];
	uint16_t type;
	
	union
	{
		struct
		{
			uint16_t hwtype,ptype;
			uint8_t hwlen,plen;
			uint16_t op;
			uint8_t srcmac[6];
			uint32_t srcip;
			uint8_t destmac[6];
			uint32_t dstip;
		} arp;

		struct
		{
			uint16_t verihl,len;
			uint16_t ident,fragoff;
			uint8_t ttl,proto;
			uint16_t checksum;
			uint32_t srcip;
			uint32_t dstip;
			
			union
			{
				struct
				{
					uint16_t srcport;
					uint16_t dstport;
					uint16_t len;
					uint16_t chksum;
				} udp;
				struct
				{
					uint16_t srcport;
					uint16_t dstport;
					uint16_t sequence;
					uint16_t ack;
				} tcp;
			};
		} ipv4;
	};
};

#define BUFFER_SIZE 2048
#define ETHER_ADDR_LEN 6

#define RARP_ETHERTYPE 0x8035
#define RARP_REQUEST   3
#define RARP_REPLY     4


int open_bpf_device(const char *iface_name, uint8_t *hostmac) {
    char bpf_path[32];
    int bpf_fd = -1;
		int i;
		
    // macOS dynamically allocates BPF devices; look for a free node
    for (i = 0; i < 99; i++) {
        snprintf(bpf_path, sizeof(bpf_path), "/dev/bpf%d", i);
        bpf_fd = open(bpf_path, O_RDWR);
        if (bpf_fd >= 0) break;
    }

    if (bpf_fd < 0) {
        perror("Failed to open any /dev/bpfXX device node. Try running as sudo");
        return -1;
    }

    // Allocate buffer size for the BPF interface
    u_int buf_len = BUFFER_SIZE;
    if (ioctl(bpf_fd, BIOCSBLEN, &buf_len) < 0) {
        perror("BIOCSBLEN failed");
        close(bpf_fd);
        return -1;
    }

    // Bind the BPF file descriptor to your chosen hardware interface (e.g., en0)
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ);
    if (ioctl(bpf_fd, BIOCSETIF, &ifr) < 0) {
        perror("BIOCSETIF failed to bind interface");
        close(bpf_fd);
        return -1;
    }

    // Ensure write operations flush directly to the hardware link immediately
    u_int immediate = 1;
    ioctl(bpf_fd, BIOCIMMEDIATE, &immediate);

	// we only want IPv4 packets
	struct bpf_insn insnsIPV4[] = {
		{ BPF_LD + BPF_H + BPF_ABS, 0, 0, 12 },				// Ethernet Type (offset 12)
		{ BPF_JMP + BPF_JEQ + BPF_K, 0, 1, RARP_ETHERTYPE },		// is type 0x8035 (RARP)
		{ BPF_RET + BPF_K, 0, 0, 65535 },
		{ BPF_JMP + BPF_JEQ + BPF_K, 0, 1, 0x0800 },						// is type 0x0800 (IPv4)
		{ BPF_RET + BPF_K, 0, 0, 65535 },
		{ BPF_RET + BPF_K, 0, 0, 0 }
	};
	
	struct bpf_program filter = { 6, insnsIPV4 };
	ioctl(bpf_fd, BIOCSETF, &filter);

    printf("Successfully bound BPF node to interface: %s\n", iface_name);

	// get our MAC address
#ifdef __linux__
    #include <sys/ioctl.h>
    #include <net/if.h>
    #include <sys/socket.h>

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    int result = ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (result == 0)
        memcpy(hostmac, ifr.ifr_hwaddr.sa_data, 6);

    close(fd);
#elif defined(__APPLE__)
    struct ifaddrs *ifap, *p;

    if (getifaddrs(&ifap) != 0)
        return -1;

    for (p = ifap; p; p = p->ifa_next)
    {
        /* Check the device name */
        if ((strcmp(p->ifa_name, iface_name) == 0) &&
            (p->ifa_addr->sa_family == AF_LINK))
        {
						//printf("checking AF_LINK %s\n", p->ifa_name);
        
            struct sockaddr_dl* sdp;

            sdp = (struct sockaddr_dl*) p->ifa_addr;
            memcpy((void *)hostmac, sdp->sdl_data + sdp->sdl_nlen, ETHER_ADDR_LEN);
            break;
        }
    }
    freeifaddrs(ifap);
#endif

    return bpf_fd;
}

#endif

int main(argc, argv)
int argc;
char **argv;
{
	int portmapsock, mountsock, locksock, nfssock;
	int n;
	struct hostent *host_entry;
	int launched_by_server = 0;

#ifdef TEK4404
	/* are we being launched by /etc/server? */
	struct stat s;
	fstat(0, &s);
	if (s.st_mode & S_IFPIPE)
	{
		launched_by_server = 1;
	}
	console = fopen("/dev/console","w");
	if (geteuid() != 0)
		exit(-1);
#else
	console = stdout;
#endif

	/* get our IP address so we can point clients back at us */
	gethostname(host_name, sizeof(host_name));
	host_entry = gethostbyname(host_name);
	n = 0;
	while(host_entry->h_addr_list[n])
	{
		host_assigned = *(struct in_addr*)(host_entry->h_addr_list[n]);
		if (host_assigned.s_addr != htonl(INADDR_LOOPBACK))
			break;
		n++;
	}
	/* shorten it */
	if (strchr(host_name, '.'))
		*strchr(host_name,'.') = '\0';

	fprintf(console, "%s: running on host: %s (%s)\n",  basename(argv[0]), host_name, inet_ntoa(host_assigned) );

	umask(0);

	/* we act as portmapd, mountd and nfsd... */
	portmapsock = launched_by_server ? fileno(stdin) : create_UDP_sock("portmapd", PORTMAPPERD_PORT);
	mountsock = create_UDP_sock("mountd", MOUNTD_PORT);
	locksock = create_UDP_sock("lockd", LOCKD_PORT);
	nfssock = create_UDP_sock("nfsd", NFSD_PORT);

	/* cannot continue (not having portmapping is tolerable) */
	if (mountsock < 0 || locksock < 0 || nfssock < 0)
	{
		fprintf(console, "cannot bind sockets\n");
		exit(-2);
	}
	
#ifdef SUNBOOT
	/* we are going to offer rarp, tftp and bootparams too */
	int rarp_bpf_fd = 0;
	int tftpsock = 0;
	int bootparamsock = 0;
	if (argc > 1)
	{
		for(n=1; n<argc; n++)
		{
			if (!strcmp(argv[n],"-mac"))
			{
				uint32_t values[6];
				n++;
				if (sscanf(argv[n], "%x:%x:%x:%x:%x:%x", values+0, values+1,values+2,values+3,values+4,values+5) == 6)
				{
					rarp_machinemac[0] = (uint8_t)values[0];
					rarp_machinemac[1] = (uint8_t)values[1];
					rarp_machinemac[2] = (uint8_t)values[2];
					rarp_machinemac[3] = (uint8_t)values[3];
					rarp_machinemac[4] = (uint8_t)values[4];
					rarp_machinemac[5] = (uint8_t)values[5];
				}
				else
				{
					fprintf(console, "badly formed mac address\n");
					exit(-3);
				}
			}
			else
			if (!strcmp(argv[n],"-base") || !strcmp(argv[n],"-ftpbase"))
			{
				int fd;

				n++;
				strcpy(tftp_base, argv[n]);
				fd = open(tftp_base, 0);
				if (fd < 0)
				{
					fprintf(console, "%s: not found\n", tftp_base);
					exit(-3);
				}
				close(fd);
			}
			else
			if (!strcmp(argv[n],"-hostname"))
			{
				n++;
				strcpy(bp_machinename, argv[n]);
			}
			else
			if (!strcmp(argv[n],"-addr"))
			{
				int dummy1,dummy2,dummy3,dummy4;
				n++;
				strcpy(bp_addr, argv[n]);
				if (sscanf(bp_addr, "%d.%d.%d.%d", &dummy1,&dummy2,&dummy3,&dummy4) != 4)
				{
					fprintf(console, "badly formed ip address\n");
					exit(-3);
				}
			}
			else
			if (!strcmp(argv[n],"-fs"))
			{
				int fd;
				
				n++;
				strcpy(bp_fs, argv[n]);
				fd = open(bp_fs, 0);
				if (fd < 0)
				{
					fprintf(console, "%s: not found\n", bp_fs);
					exit(-3);
				}
				close(fd);
			}
			else
			if (!strcmp(argv[n],"-swap"))
			{
				n++;
				strcpy(bp_swap, argv[n]);
			}
			else
			if (!strcmp(argv[n],"-dump"))
			{
				n++;
				strcpy(bp_dump, argv[n]);
			}
		}
	
		/* do we have all the info we need? */
		if (bp_machinename[0] && bp_addr[0] && bp_fs[0] && bp_swap[0])
		{
			/* other services */
			tftpsock = create_UDP_sock("tftpd", 69);
			bootparamsock = create_UDP_sock("bootparamd", BOOTPARAMD_PORT);
			if (tftpsock < 0 || bootparamsock < 0)
			{
				fprintf(console, "cannot bind sockets\n");
				exit(-3);
			}

			/* setup RARP packet handling using BPF */
			rarp_bpf_fd = open_bpf_device("en0", host_mac);
		}
		else
		{
			fprintf(console, "missing bootparam info.\n");
			exit(-4);
		}
	}
#endif

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
		FD_SET(locksock, &fd_in);
		if (locksock > n)
			n = locksock;
		FD_SET(nfssock, &fd_in);
		if (nfssock > n)
			n = nfssock;

#ifdef SUNBOOT
		/* are we in all-in-one mode? */
		if (tftpsock > 0)
		{
			FD_SET(rarp_bpf_fd, &fd_in);
			if (rarp_bpf_fd > n)
				n = rarp_bpf_fd;
			FD_SET(tftpsock, &fd_in);
			if (tftpsock > n)
				n = tftpsock;
			FD_SET(bootparamsock, &fd_in);
			if (bootparamsock > n)
				n = bootparamsock;
		}
#endif

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
					mountprog(&request, NFS_FALSE);
				}
			}
		}
		else
		if (FD_ISSET(locksock, &fd_in))
		{
			request.sock = locksock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				/* validate */
				n = validate(&request, LOCKD);
				if (n)
				{
					lockprog(&request, NFS_FALSE);
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
						 nfsprog(&request, NFS_FALSE);
					if (n == 3)
						 nfs3prog(&request, NFS_FALSE);
				}
			}
		}
#ifdef SUNBOOT
		else
		if (FD_ISSET(rarp_bpf_fd, &fd_in))
		{
			uint8_t buffer[BUFFER_SIZE];
		
			int len = read(rarp_bpf_fd, buffer, BUFFER_SIZE);
			struct bpf_hdr *hdr = (struct bpf_hdr *)buffer;

			// NB we may have read >1 packet
			while ((len > 0) && hdr->bh_hdrlen == 18)
			{
				uint8_t *packet_data = (uint8_t *)hdr + hdr->bh_hdrlen;
				struct eth2 *ethpkt = (struct eth2 *)packet_data;

				if (ethpkt->type == ntohs(0x0800))
				{
						if ((ethpkt->destmac[0]==0xff && ethpkt->destmac[1]==0xff &&						/* BROADCAST */
								ethpkt->destmac[2]==0xff && ethpkt->destmac[3]==0xff &&
								ethpkt->destmac[4]==0xff && ethpkt->destmac[5]==0xff))
						{
								if (ethpkt->ipv4.proto == 17)
								{
									/* is it for portmapper? */
									if (ethpkt->ipv4.udp.dstport == ntohs(111))
									{
										/* redirect */
										
										printf("redirecting broadcast:111 size:%d from %s\n",  hdr->bh_caplen, inet_ntoa(*(struct in_addr *)&(ethpkt->ipv4.srcip)));
										
										request.crp = 0;
										request.sock = portmapsock;
										request.len = hdr->bh_caplen;
										memcpy(request.buffer, (&ethpkt->ipv4)+1, hdr->bh_caplen);
										request.from.sin_family = AF_INET;
										request.from.sin_port = ethpkt->ipv4.udp.srcport;
										request.from.sin_addr.s_addr = ethpkt->ipv4.srcip;
										if (request.len > 0)
										{
											/* validate */
											if (validate(&request, PORTMAPPERD))
											{
												portmapperprog(&request);
											}
										}
									
									}
								}
						}
				}
				else
				if (ethpkt->type == ntohs(RARP_ETHERTYPE))
				{
					if (ethpkt->arp.op == ntohs(3))
					{
						if ((ethpkt->destmac[0]==0xff && ethpkt->destmac[1]==0xff &&						/* BROADCAST */
								ethpkt->destmac[2]==0xff && ethpkt->destmac[3]==0xff &&
								ethpkt->destmac[4]==0xff && ethpkt->destmac[5]==0xff))
#if 0
								||
								(ethpkt->destmac[0]==host_mac[0] && ethpkt->destmac[1]==host_mac[1] &&	/* for THIS NIC */
								ethpkt->destmac[2]==host_mac[2] && ethpkt->destmac[3]==host_mac[3] &&
								ethpkt->destmac[4]==host_mac[4] && ethpkt->destmac[5]==host_mac[5]))
#endif
						{
							if (!memcmp(ethpkt->srcmac, rarp_machinemac, 6))
							{
								struct eth2 reply;
								uint32_t ipv4;
								char cmd[128];
								
								/* build a reply */
								memcpy(reply.destmac, ethpkt->srcmac, 6);
								memcpy(reply.srcmac, host_mac, 6);
								reply.type = htons(RARP_ETHERTYPE);

								reply.arp.hwtype = htons(1);
								reply.arp.ptype = htons(0x800);
								reply.arp.hwlen = 6;
								reply.arp.plen = 4;
								reply.arp.op = htons(4);
								
								memcpy(reply.arp.srcmac, host_mac, 6);
								ipv4 =  host_assigned.s_addr;
								memcpy(&reply.arp.srcip, &ipv4, 4);
								memcpy(reply.arp.destmac, ethpkt->srcmac, 6);
								ipv4 = inet_addr(bp_addr);
								memcpy(&reply.arp.dstip, &ipv4, 4);
								
								write(rarp_bpf_fd, &reply, sizeof(reply));

								fprintf(console, "rarpd: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x => %s\n",
									reply.destmac[0],reply.destmac[1],reply.destmac[2],reply.destmac[3],reply.destmac[4],reply.destmac[5],
									inet_ntoa(*(struct in_addr *)&(reply.arp.dstip)));


								snprintf(cmd,sizeof(cmd),"arp -s %s %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x", bp_addr,
												rarp_machinemac[0],rarp_machinemac[1],rarp_machinemac[2],
												rarp_machinemac[3],rarp_machinemac[4],rarp_machinemac[5]);
								system(cmd);
								fprintf(console, "rarpd: adding ARP entry: %s\n", cmd);
							}
						}
					}
				}
				

				len -= hdr->bh_hdrlen + hdr->bh_caplen;
				hdr = (struct bpf_hdr *)((uint8_t *)hdr + hdr->bh_hdrlen + hdr->bh_caplen);
				
				//if (len)
				//	printf("MORE packets STUFF: %d\n", len);
			}
			/* printf("***** remain = %d hdrlen = %d caplen = %d\n", len, hdr->bh_hdrlen, hdr->bh_caplen); */
            
		}
		else
		if (FD_ISSET(tftpsock, &fd_in))
		{
			request.sock = tftpsock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				tftp(&request);

			}
		}
		else
		if (FD_ISSET(bootparamsock, &fd_in))
		{
			request.sock = bootparamsock;
			request.len = recvfrom(request.sock, request.buffer, sizeof(request.buffer), 0, (struct sockaddr *)&request.from, &fromSize);
			if (request.len > 0)
			{
				/* validate */
				if (validate(&request, BOOTPARAMD))
				{
					bootparamprog(&request, NFS_FALSE);
				}
			}
		}
#endif
	}
	
	/* TODO: some cleanup */
}
	
