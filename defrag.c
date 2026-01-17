#ifdef __clang__
/* long assumes 32-bit.. */
#define long int
#pragma pack(push, 1)
#endif

#include <sys/stat.h>
#include <sys/signal.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/modes.h>
#include <sys/sir.h>
#include <sys/fdn.h>

typedef unsigned short USHORT;
#define ERR (-1)
#define TRUE (1)
#define FALSE (0)

/* Disk geometry*/
#define BLOCKSPERTRACK 18
#define HEADSPERCYLINDER 6
#define BLOCKSPERCYLINDER (BLOCKSPERTRACK * HEADSPERCYLINDER)

/*******************************************************/
/*  Following is split into two because compiler won't */
/*  accept character array of > 32767.  Second value   */
/*  must be 1/2 of first and without L suffix.         */
#define MAXBLKS 312000     /* max disk blocks */
/*******************************************************/

#define FDNSIZ 64
#define FBPER 100 /* free blocks per free block */
#define ROOTFDN 1
#define BBFDN 2
#define SIRADR 1L /* physical block address of SIR */
#define STD_OUT 1 /* fd number of standard output */

#define INITSK 1  /* task id number for "init" */
#define ALL -1    /* number for int to all tasks */

#define FMSZ 10      /* fdn direct map size */
#define SMSZ 128  /* single indirection map block size */
#define DMSZ SMSZ*SMSZ  /* double indirection map size */
#define BGDMSZ SMSZ+FMSZ   /* size of map up to begin of s.i. */
#define BGTMSZ DMSZ+SMSZ+FMSZ /* size of map up to begin of t.i. */

struct dirblk {
	USHORT d_fdn;
	char d_name[14];
};

struct dirpath {
	USHORT fdn;
	USHORT d_parentfdn;
	char	d_path[64];		/* FIXME: should be packed into string array */
};
#define MAX_DIRENTRY 2048
int numdentries = 0;
struct dirpath dircache[MAX_DIRENTRY];


#define MAX_DUPS 60 /* max number of dups in table */
#define MAX_OORS 40 /* max out of ranges in table */
#define MAX_SIZE_ERRS 20 /* max size errors in table */

struct mapstr {
    char mapb[3*SMSZ];
    long indrct[3];
    USHORT nxtblk;
    long msize;
    char mtype;
};

USHORT oors[MAX_OORS];
short oors_cnt;

struct dups_s {
    long dup_b;
    USHORT dup_f;
} dups[MAX_DUPS];

short dups_cnt;      /* count of dups in buffer */
short toomdup;    /* too many dups flag */
short udupcnt;    /* unique dups count */

struct mapstr badmap;
short badblocks;     /* ".badblocks" exists flag */

typedef enum {
	BUILDMAP=0,
	BUILDFILE
} mapmode;

mapmode mappingmode;
char* mapbuf;    /* map of used blocks */
int mapsize;

#define MAX_CYLINDERSET 64
struct fdlayout {
    USHORT cid[MAX_CYLINDERSET];
    USHORT numcylinders;
    short filetype;
    long real_filesize;

    USHORT blocks[16384];
    USHORT numblocks;
};

struct fdlayout currfdlayout;

#define MAX_CANDIDATES 256
struct candidate {
	struct inode inode;
	char filepath[128];
};
int numcandidates;
struct candidate candidates[MAX_CANDIDATES];


char *devname;
short fd_sir;
char sir_area[512];  /* buffer area for SIR */
struct sir *sirbuf;

short v_opt;      /* verbose option */
short fatal_error;   /* nonzero if fatal error */

short modified;      /* disk has been modified */
short must_sir;      /* must update SIR */
short must_frfdn; /* must fix free fdn count */
short must_fix;      /* disk is bad - needs fix */
short must_newfree;  /* must run newfree flag */

char *fbptr;      /* new free block pointer */
short fbbit;      /* new free block bit value */
long fbblock;     /* new free block address */

long freefdns, usedfdns, blkfdns;
long chrfdns, dirfdns, regfdns;
long regblks, dirblks, mapblks;
long freeblks,oorfbks,dupfbks;
long dupblks, oorblks, missbks;

long maxvld, minvld, available, *lptr;
long lastblk, blkcount;
short sszfdn, filetype;

USHORT fdnno;

#ifdef DEBUG
char fdnchk[] = "fdncheck";
char bb[] = "badblocks";
#else
char fdnchk[] = "/etc/fdncheck";
char bb[] = "/etc/badblocks";
#endif
char errinbad[] = "Error checking \".badblocks\" file:  Ignored.\n\n";
char badbad[] = "Bad \".badblocks\" file:  Ignored.\n\n";
char errinmap[] = "Error reading fdn %u:  Part of file may be ignored.\n\n";
char cntcll[] = "Can't call \"%s\".\n";
char errrdgb[] = "Error reading block %ld.\n";
char odtdut[] = "Output directed to device under test.\n";

char* framebuffer;

#ifndef __clang__
#info 68xxx UniFLEX (R) blockcheck
#info Version 2.07:0; Released October 1, 1985
#info Copyright (c) 1983, 1984, 1985 by
#info Technical Systems Consultants, Inc.
#info All rights reserved.
#tstmp
#define htons(A) (A)
#define ntohs(A) (A)
#else
#define htons(A) ((A>>8) | (A<<8))
#define ntohs(A) ((A>>8) | (A<<8))

extern int open();
extern int read();
extern int write();
extern long lseek();
extern int close();
extern void exit();
extern int fstat();
extern char *malloc();
extern void free();
extern char *strcpy();
extern char *strcat();
extern int strlen();

void _l2tos(char *blocks,char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = sindblk[1];
		*blocks++ = sindblk[0];
		sindblk += 2;
	}
}

void l3tol(char *blocks,char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = sindblk[2];
		*blocks++ = sindblk[1];
		*blocks++ = sindblk[0];
		*blocks++ = 0;
		sindblk += 3;
	}
}

void _l4tol(char *blocks,char *sindblk,int count)
{
	while(count--)
	{
		// BE to LE
		*blocks++ = sindblk[3];
		*blocks++ = sindblk[2];
		*blocks++ = sindblk[1];
		*blocks++ = sindblk[0];
		sindblk += 4;
	}
}

struct bmpheader {

  short bfType;
  int bfSize;
  short  bfReserved1;
  short  bfReserved2;
  int bfOffBits;

  int biSize;
  int  biWidth;
  int  biHeight;
  short  biPlanes;
  short  biBitCount;
  int biCompression;
  int biSizeImage;
  int  biXPelsPerMeter;
  int  biYPelsPerMeter;
  int biClrUsed;
  int biClrImportant;

  int palette[2];
};

#define STRIDE 1024/8
void dumpframebuffer()
{
struct bmpheader header;
FILE *fp;
FILE *output;
char *fb;
int i,x,y;
int width = 640;
int height = 480;

  output = fopen("/Users/adambillyard/defrag.bmp", "w");

  /* write an image header */		
  header.bfType = (0x4d42);
  header.bfSize = (14+40+2*4+(width/8)*height);
  header. bfReserved1 = 0;
  header. bfReserved2 = 0;
  header.bfOffBits = (14 + 40 );

  header.biSize = (40);
  header.biWidth = (width);
  header.biHeight = (-height);	/* flipped */
  header.biPlanes = (1);
  header.biBitCount = (1);
  header.biCompression = 0;
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0;
  header.biYPelsPerMeter = 0;

  header.biClrUsed = (2);
  header.biClrImportant = 0;
  /* must use palette */
  header.palette[0] = 0xffffffff;	/* white bg */
  header.palette[1] = 0xff000000;	/* black fg */
  fwrite(&header, sizeof(header), 1, output);

  fb = framebuffer;
  for (y=0; y<height; y++)
  {
    fwrite(fb, width/8, 1, output);
    fb += STRIDE;
  }
  
  fclose(output);
}
#endif

/***************************************************************/


void clear_map()
{
    register int* iptr;
    register short i = mapsize / 4;

    iptr = (int*)mapbuf;
    while (i--) *iptr++ = 0;
}


setbit(blkadr)
long blkadr;
{
    char *bytptr;
    long blkoff;
    short bitmsk;

    blkoff = blkadr - minvld;
    bytptr = mapbuf+(blkoff>>3);
    bitmsk = (0x80)>>((int)blkoff & 0x7);
    if((*bytptr & bitmsk) == 0) {
   *bytptr |= bitmsk;
   return(0);
    } else {
   return(1);
    }
}


clrbit(blkadr)
long blkadr;
{
    char *bytptr;
    long blkoff;
    short bitmsk;

    blkoff = blkadr - minvld;
    bytptr = mapbuf+(blkoff>>3);
    bitmsk = (0x80)>>((int)blkoff & 0x7);
    if((*bytptr & bitmsk)) {
   *bytptr &= ~bitmsk;
   return(1);
    } else {
   return(0);
    }
}

/***************************************************************/

void set_dup(blkadr)
long blkadr;
{
    ++dupblks;
    if(dups_cnt == MAX_DUPS) 
    {
       printf("Too many duplicate blocks.\n");
       return;
    }

    dups[dups_cnt].dup_b = blkadr;
    dups[dups_cnt++].dup_f = fdnno;
}


void set_oor()
{
    ++oorblks;
   printf("Out-of-range block in fdn %u.\n",fdnno);
    if(oors_cnt == MAX_OORS) 
    {
       printf("Too many out-of-range blocks.\n");
       return;
    }
    oors[oors_cnt++] = fdnno;
}


/***************************************************************/

void contin(badblk)
long badblk;
{
    if ((badblk > 2) && (badblk < lastblk))
    {
        printf("Add to \".badblocks\"\n");
    }
}


int rdblk(bkadr,buffer)
long bkadr;
char *buffer;
{
    if(lseek(fd_sir,bkadr<<9L,0) == bkadr<<9L)
    {
			 if (read(fd_sir,buffer,512) == 512)
			 {
					return 0;
			 }
		}

	 printf(errrdgb,bkadr);
	 contin(bkadr);
	 return(ERR);
}


wtblk(bkadr,buffer)
long bkadr;
char *buffer;
{
    long lseek();

    modified = 1;
    if((lseek(fd_sir,bkadr<<9L,0) != bkadr<<9L) || (write(fd_sir,buffer,512) != 512)) 
    {
       printf("Error writing block %ld.\n",bkadr);
       contin(bkadr);
       return(ERR);
    } 
    else 
    {
        return(0);
    }
}


int rdfdn(fdn,buffer)
USHORT fdn;
char *buffer;
{
    long offset, lseek();

    offset = ((((fdn-1L)>>3L)+2L)<<9L) + (((fdn-1)&0x7)<<6L);
    if((lseek(fd_sir,offset,0) != offset) || (read(fd_sir,buffer,FDNSIZ) != FDNSIZ)) 
    {
       printf("Error reading fdn %u in block %u.\n",fdn,((fdn-1)>>3)+2);
       contin(((fdn-1)>>3)+2);
       return(ERR);
    } 
    else 
    {
    return(0);
    }
}


/***************************************************************/
void fxdbcnt()
{
    short i, j, k;

    for(i=0; i<dups_cnt; ++i) {
        k = 0;
        for(j=i-1; j>=0; --j)
            if(dups[i].dup_b == dups[j].dup_b) ++k;
        if(k==0) ++dupblks;
    }
}


void cnt_miss(cbits,twobyte)
short cbits;
short twobyte;
{
    short bit;

    for(bit=0; bit<cbits; ++bit) {
        if((twobyte & (1<<(15-bit)))==0) ++missbks;
    }
}

void chk_miss()
{
    short *endptr, *iptr, xtra;

    xtra = available%16L;
    endptr = mapbuf+((available-xtra)>>3L);
    for(iptr=mapbuf; iptr<endptr; ++iptr) {
        if(*iptr != (~0)) {
            cnt_miss(16,*iptr);
        }
    }
    if(xtra) {
        cnt_miss(xtra,*endptr);
    }
}


void do_free()
{
    char buffer[512];
    char *bptr;
    short count;
    long blkadr, do_frbk();

    bptr = sirbuf->blklist;
    count = sirbuf->blkptr;
    if((count < 0) || (count > CDBLKS)) {
        if(v_opt) printf("Bad in-core block count.\n");
        return;
    }
    if(count == 0) return;
    while((blkadr=do_frbk(count,bptr))) {
        if((blkadr>=minvld) && (blkadr<=maxvld)) {
            if(rdblk(blkadr,buffer)==ERR) {
                printf("Free list check aborted.\n");
                return;
            }
            bptr = buffer;
            count = 100;
        }
        else {
            if(v_opt) printf("Out-of-range pointer in free list.\n");
            return;
        }
    }
}




void reset()
{
    short i;

    udupcnt = 0;
    for(i=dups_cnt-1; i>=0; --i) {
        if(clrbit(dups[i].dup_b)) ++udupcnt;
    }
}

int findcontiguous(blockcount)
int blockcount;
{
    char* bytptr, *bytend;
    short j;
    short numblock8 = (blockcount + 7) >> 3;
    int newaddr = 0;

    bytptr = mapbuf;
    bytend = bytptr + mapsize;
    while (bytptr < bytend)
    {
        /* find a run of groups of 8 blocks that are free */
        j = numblock8;
        while (--j >= 0)
        {
            if (bytptr[j] != 0)
            {
                bytptr += j + 1;
                break;
            }
        }
        if (j == -1)
        {
					unsigned short i;
					
						/* NB offset map 'address' to first valid block minvld */
            newaddr = minvld + (bytptr - mapbuf) * 8;

            /* can we pack to last alocation? */
            i = bytptr[j];
            while (i && (i & 1) == 0)
            {
                newaddr--;
                i >>= 1;
            }

            break;
        }
    }

    /* assumes block0 always in used... */
    return newaddr;
}

/* count bits set in run */
int countbits(i, runlength)
int i, runlength;
{
    short val = 0;
    register short bitmsk;
    register char* bytptr;
    register short loop;

    loop = runlength;
    bytptr = mapbuf + (i >> 3);
    bitmsk = (0x80) >> (i & 7);
    while (loop > 0)
    {
        /* FIXME: use a 256 byte lookup */
        while (bitmsk && loop)
        {
            if ((*bytptr & bitmsk))
                val++;
            bitmsk >>= 1;
            loop--;
        }
        bytptr++;
        bitmsk = 0x80;
    }

    return val;
}

/* 1x1 */
void drawmap(mindex, val)
int mindex;
int val;
{
    unsigned char* dst;
    unsigned short abit;
    int x,y;

    y = mindex / 640;
    x = mindex % 640;

    y += 100;
    dst = framebuffer + (y * 128);
    abit = (0x80 >> (x & 7));
    if (val)
        dst[x >> 3] |= abit;
    else
        dst[x >> 3] &= ~abit;
}

/* 8x8 icon */
void drawcylinder(cid,val)
int cid,val;
{
    unsigned char* dst;
    int y = cid / 80;
    int x = cid % 80;

		if (y > 12)
			printf("y = %d\n", y);

    dst = framebuffer + (y * 8 * 128) + x;
    if (x > 79)
    {
        printf("x = %d\n", x);
        return;
    }
    dst[0] = dst[128] = dst[256] = dst[384] = dst[512] = dst[640] = dst[768] = 0;
    if (val >= BLOCKSPERCYLINDER)
    {
        dst[0]   |= 0xfe;
        dst[128] |= 0xfe;
        dst[256] |= 0xfe;
        dst[384] |= 0xfe;
        dst[512] |= 0xfe;
        dst[640] |= 0xfe;
        dst[768] |= 0xfe;
    }
    else
    if (val== 0)
    {
        dst[0] |= 0xfe;
        dst[128] |= 0x82;
        dst[256] |= 0x82;
        dst[384] |= 0x82;
        dst[512] |= 0x82;
        dst[640] |= 0x82;
        dst[768] |= 0xfe;
    }
    else
    {
        dst[0] |= 0xfe;
        dst[128] |= 0x82;
        dst[256] |= 0xc2;
        dst[384] |= 0xe2;
        dst[512] |= 0xf2;
        dst[640] |= 0xfa;
        dst[768] |= 0xfe;
    }
}


/***************************************************************/
int find_parentname(fdn, targetfdn, pathname)
USHORT fdn;
USHORT targetfdn;
char *pathname;
{
		struct inode buffer[FDNPB];
		struct inode* fdnptr = buffer;
    long blkadr;
		short i,j,fdi;
		char subpath[32];
		
		/* self */
		if (1 == targetfdn)
		{
			return 0;
		}

#if 0
		/* get containing inode address */
    blkadr = ((((fdn-1)>>3L)+2L));
		rdblk(blkadr, buffer);
		
		/* we know the parent dir is at ((fdn-1)&0x7) */
		//for(fdi=0; fdi<8; fdi++)
		fdi = ((fdn-1)&0x7);
#endif

		rdfdn(fdn, buffer);
		fdi = 0;
		{
			if ((buffer[fdi].fd_mod & S_IFMT) == S_IFDIR)
			{
				long blocks[13];
	
				/* just read direct blocks */
				l3tol(blocks,buffer[fdi].fd_blk,FMSZ);
				for(j=0;j<FMSZ;j++)
				{
					if (blocks[j])
					{
						struct dirblk dentries[32];
						
						/* read 32 dir entries */
						rdblk(blocks[j], dentries);
						for (i=0; i<32; i++)
						{
							dentries[i].d_fdn = ntohs(dentries[i].d_fdn);

							if (dentries[i].d_fdn == 0)
								continue;

							if (dentries[i].d_name[0] == '.' && dentries[i].d_name[1] == '.')
							{
								if (fdn != 1)
									find_parentname(dentries[i].d_fdn, fdn, pathname);
								else
								{
									subpath[0] = '/';	/* root */
									subpath[1] = '\0';
								}
							}
							else
							if (dentries[i].d_fdn == targetfdn)
							{
									/* long filename handling */
									if (dentries[i].d_name[0] & 0x80)
									{
										sprintf(subpath, "%c%.13s", dentries[i].d_name[0] & 0x7f, dentries[i].d_name+1);
										i++;
										sprintf(subpath, "%c%s/", dentries[i].d_name[0] & 0x7f,dentries[i].d_name+1);
									}
									else
									{
										sprintf(subpath, "%.14s/", dentries[i].d_name);
									}
									
									j = 20;	/* break out loop too */
									break;
							}
							
					  }
				  }
				}
			}
		}

		strcat(pathname, subpath);
		return 0;
}

checkrange(blkadr)
long blkadr;
{
    if(blkadr) 
    {
       if(blkadr<=maxvld)
           return(1);
       else
          set_oor();
    }

    return(0);
}

void mapblock(blkadr)
long blkadr;
{
    register short i,cid;

    ++blkcount;
    if((blkadr<minvld) || (blkadr>maxvld)) {
        if(badblocks && (fdnno == BBFDN)) {
            if((blkadr<3) || (blkadr>=lastblk)) set_oor();
            return;
        } else {
            set_oor();
            return;
        }
    }

    if (mappingmode == BUILDMAP)
    {
			if (filetype == S_IFDIR) ++dirblks; else  ++regblks;

			if (setbit(blkadr))
					set_dup(blkadr);
		}
		else
		{
			/* track block use */
			if (currfdlayout.numblocks < 16384)
			{
					currfdlayout.blocks[currfdlayout.numblocks++] = blkadr;
			}

			/* track used CYLINDERS */
			cid = blkadr / BLOCKSPERCYLINDER;
			i = currfdlayout.numcylinders;
			while (i--)
			{
					/* already in list */
					if (currfdlayout.cid[i] == cid)
							return;
			}

			/* else add it if space */
			if (currfdlayout.numcylinders < MAX_CYLINDERSET)
			{
					currfdlayout.cid[currfdlayout.numcylinders++] = cid;
			}
    }
}


mapindirectblock(blkadr)
long blkadr;
{
    if ((blkadr < minvld) || (blkadr > maxvld)) {
        set_oor();
        return(ERR);
    }
    if (mappingmode == BUILDMAP)
    {
        ++mapblks;
        if (setbit(blkadr))
            set_dup(blkadr);
    }
		else
		{
			/* track block use */
			if (currfdlayout.numblocks < 16384)
			{
					currfdlayout.blocks[currfdlayout.numblocks++] = blkadr;
			}
		}

   return(0);
}

do_sind(blkadr)
long blkadr;
{
    char sindblk[512];
    long blocks[SMSZ];
    short i;

    /* failed to read */
    if(mapindirectblock(blkadr))
        return(1);

    if(rdblk(blkadr,sindblk)==ERR) 
    {
       printf(errinmap,fdnno);
       return(0);
    }

    l3tol(blocks,sindblk,SMSZ);
    for(i=0; i<SMSZ; ++i) 
    {
       if(blocks[i])
           mapblock(blocks[i]);
    }

    return(0);
}

do_dind(blkadr)
long blkadr;
{
    char dindblk[512];
    long blocks[SMSZ];
    short i;

    /* failed to read */
    if(mapindirectblock(blkadr))
        return(1);

    if(rdblk(blkadr,dindblk)==ERR) 
    {
       printf(errinmap,fdnno);
       return(0);
    }

    l3tol(blocks,dindblk,SMSZ);
    for(i=0; i<SMSZ; ++i) 
    {
        if (checkrange(blocks[i]))
        {
            if (do_sind(blocks[i]))
                return(1);
        }
    }

    return(0);
}

do_tind(blkadr)
long blkadr;
{
    char tindblk[512];
    long block;
    short i;

    /* failed to read */
    if(mapindirectblock(blkadr))
       return(1);

    if(rdblk(blkadr,tindblk)==ERR) 
    {
       printf(errinmap,fdnno);
       return(0);
    }

    for(i=0; i<SMSZ; ++i) 
    {
       l3tol(&block,tindblk+(3*i),1);
       if (checkrange(block))
       {
           if (do_dind(block))
               return(1);
       }
    }

    return(0);
}


mark_in_map(fdnptr)
struct inode *fdnptr;
{
    long blocks[13];
    register int i;

    blkcount = 0;
    l3tol(blocks,fdnptr->fd_blk,13);

    /* track which cylinders are used */
    currfdlayout.numcylinders = 0;
    currfdlayout.numblocks = 0;

    /* 10 direct blocks */
    for(i=0; i<FMSZ; ++i) 
        if(blocks[i]) mapblock(blocks[i]);

    /* single indirection */
    if(checkrange(blocks[10]))
        if(do_sind(blocks[10]))
            return(1);

    /* double indirection */
    if(checkrange(blocks[11]))
        if(do_dind(blocks[11]))
            return(1);

    /* triple indirection */
    if(checkrange(blocks[12]))
        if(do_tind(blocks[12]))
            return(1);

    return(0);
}

void enum_files(fdnptr, fullpath)
struct inode *fdnptr;
char *fullpath;
{
	long blocks[13];
	int i,j,k;
	char subpath[128];
	char apath[128];
	
	/* just read direct blocks */
	l3tol(blocks,fdnptr->fd_blk,FMSZ);
	for(j=0;j<FMSZ;j++)
	{
		if (blocks[j])
		{
			struct dirblk dentries[32];
			int len;
			
			len = strlen(fullpath);
			
			/* read 32 dir entries */
			rdblk(blocks[j], dentries);
			for (i=0; i<32; i++)
			{
				struct inode currfdn;
				long real_filesize;
				
				if (dentries[i].d_fdn == 0)
					continue;

				if (dentries[i].d_name[0] == '.' && dentries[i].d_name[1] == '\0')
					continue;
					
				if (dentries[i].d_name[0] == '.' && dentries[i].d_name[1] == '.')
					continue;
					
				dentries[i].d_fdn = ntohs(dentries[i].d_fdn);

				/* get entry fdn (can be double incremented for long filenames) */
				rdfdn(dentries[i].d_fdn, &currfdn);

#if 0
				/* ignore linked files */
				if (currfdn.fd_cnt > 3)
					continue;
#endif

				/* ignore other types */
				if ((currfdn.fd_mod & S_IFMT) != S_IFDIR && (currfdn.fd_mod & S_IFMT) != S_IFREG)
					continue;

				/* cache some info */
				_l4tol(&real_filesize, currfdn.fd_siz, 1);
				currfdlayout.filetype = currfdn.fd_mod & S_IFMT;
				currfdlayout.real_filesize = real_filesize;
				
				/* long filename handling */
				if (dentries[i].d_name[0] & 0x80)
				{
					sprintf(subpath, "%c%.13s", dentries[i].d_name[0] & 0x7f, dentries[i].d_name+1);
					i++;
					/* FIXME: is it ALWAYS 14 if a long filename? */
					sprintf(subpath+14, "%c%s", dentries[i].d_name[0] & 0x7f,dentries[i].d_name+1);
				}
				else
				{
					sprintf(subpath, "%.14s", dentries[i].d_name);
				}

				/* follow blocks and mark in map */
				mark_in_map(&currfdn);

				printf("%5d blocks: %s%s\033[0J\033[1G\n", currfdlayout.numblocks, fullpath, subpath);

				if ((currfdn.fd_mod & S_IFMT) == S_IFDIR)
				{
					++dirfdns;

					/* recurse on subdir */
					strcat(fullpath, subpath);
					strcat(fullpath, "/");
					enum_files(&currfdn, fullpath);
					fullpath[len] = '\0';
				}
				else
				if ((currfdn.fd_mod & S_IFMT) == S_IFREG)
				{
					short minc, maxc, maxcylseek, fixedsize;
					short needsmoving = 0;

					++regfdns;

					/* best case cylinder requirement */
					k = (currfdlayout.real_filesize + (BLOCKSPERCYLINDER << 9) - 1) / (BLOCKSPERCYLINDER << 9);

					/* inefficient cylinder use: using >5 cylinders over expected for filesize  */
					needsmoving |= (currfdlayout.numcylinders - k > 5);

					/* spaced out cylinder use? find range */
					/* FIXME: we should sort and look for gaps in cylinder usage */
					minc = mapsize * 8 / BLOCKSPERCYLINDER;
					maxc = 0;
					maxcylseek = 0;
					for (k = 0; k < currfdlayout.numcylinders; k++)
					{
							if (k > 0)
							{
								short diff = currfdlayout.cid[k] - currfdlayout.cid[k-1];
								if (diff < 0) diff = -diff;
								if (diff > maxcylseek) maxcylseek = diff;
							}
							if (currfdlayout.cid[k] < minc) minc = currfdlayout.cid[k];
							if (currfdlayout.cid[k] > maxc) maxc = currfdlayout.cid[k];
					}

					/* spaced out use: using range >5 cylinders over minimum */
					//needsmoving |= ((minc + currfdlayout.numcylinders + 5) < (maxc - minc));
					
					/* seeking to cylinders far apart */
					needsmoving |= maxcylseek > 4;
					
					/* is read-only or executable => fixed length file */
					fixedsize = (currfdn.fd_prm &S_IPRM) & (S_IEXEC | S_IOEXEC);
					fixedsize |= !(currfdn.fd_prm &S_IPRM) & (S_IWRITE | S_IOWRITE);

					if (fixedsize && needsmoving && real_filesize > 16384)
					{
							/* keep track of inode for later */
							if (numcandidates < MAX_CANDIDATES)
							{
									strcpy(candidates[numcandidates].filepath, fullpath);
									strcat(candidates[numcandidates].filepath, subpath);
									candidates[numcandidates++].inode = currfdn;
							}
					}
				}
			}
		}
	}

}

void do_fdns(phase_a)
short phase_a;
{
    struct inode fdnbuf[FDNPB];
    struct inode* fdnptr;

		char fullpath[256];
    struct inode afdn;
    short fdnbno;
    long real_filesize, swapbeg, gfblks();
    int i, j, k;
    USHORT swapsize;

    /* track FD */
    freefdns = usedfdns = 0;
    blkfdns = chrfdns = dirfdns = regfdns = 0;

    /* track blocks */
    freeblks = 0;
    regblks = dirblks = mapblks = 0;

    /* track errors */
    dups_cnt = oors_cnt = 0;
    oorfbks = dupfbks = 0;
    dupblks = oorblks = missbks = 0;

    printf("reading %d file descriptors blocks for disk: %s\n", sszfdn,sirbuf->sfname);

		mappingmode = BUILDMAP;
    clear_map();

    /* should this use ROOTFDN ? */
    fdnno = 1;
    for (fdnbno = 1; fdnbno <= sszfdn; ++fdnbno)
    {
        /* read a block of 8 inodes */
        if (rdblk((long)fdnbno + 1L, fdnbuf) == ERR)
        {
            printf("Fdns %u-%u skipped.\n", fdnno, fdnno + FDNPB - 1);
            fdnno += FDNPB;
            continue;
        }

        /* walk inodes */
        for (fdnptr = fdnbuf; fdnptr < fdnbuf + FDNPB; ++fdnptr)
        {
            /* cache some info of current FD */
            _l4tol(&real_filesize, fdnptr->fd_siz, 1);
            filetype = fdnptr->fd_mod & S_IFMT;
            currfdlayout.filetype = filetype;
            currfdlayout.real_filesize = real_filesize;

            /* gather some stats */
            ++usedfdns;
            switch (filetype) {
            case 0:
                --usedfdns;
                ++freefdns;
                break;
            case S_IFREG:
                ++regfdns;
                break;
            case S_IFDIR:
                ++dirfdns;
                break;
            case S_IFCHR:
                ++chrfdns;
                break;
            case S_IFBLK:
                ++blkfdns;
                break;
            case S_IFPTY:
            case S_IFPIPE:
            case S_IFCON:
                break;
            default:
                printf("Unknown type (%02x) for fdn %u.\n", filetype, fdnno);
                break;
            }

            /* just regular files and directories */
            if ((filetype == S_IFREG) || (filetype == S_IFDIR))
            {
                /* follow blocks and mark in map */
                mark_in_map(fdnptr);
            }
				}
		}

		/* stop if FS not valid */
		if (dups_cnt || oors_cnt)
		{
			printf("disk corrupt: run diskrepair\n");
			exit(-1);
		}


    /* gather inode of each candidate */
		mappingmode = BUILDFILE;
    numcandidates = 0;
		fullpath[0] = '/';
		fullpath[1] = '\0';
		rdfdn(ROOTFDN, &afdn);
		enum_files(&afdn, fullpath);

    printf("checked %d files in %d directories; found %d candidates\n", regfdns, dirfdns, numcandidates);

    /* mark swap as used too */
    l3tol(&swapbeg, sirbuf->sswpbg, 1);
    if (swapbeg != 0)
    {
        _l2tos(&swapsize, sirbuf->sswpsz, 1);
        printf("%d blocks for swap\n", swapsize);
        for (i = 0; i < swapsize; i++)
        {
            setbit(swapbeg + i);
        }
    }

    /* draw whole map; 8 blocks for each byte in map */
    printf("\033[1;32r\033[1H\033[J\n");
    for (i = 0; i < mapsize * 8; i += BLOCKSPERCYLINDER)
    {
        /* convert block_id to cylinder_id */
        drawcylinder(i / BLOCKSPERCYLINDER, countbits(i, BLOCKSPERCYLINDER));
    }

    for (i = 0; i < mapsize * 8; i++)
    {
        char* bytptr;
        long blkoff;
        short bitmsk;

        blkoff = i;
        bytptr = mapbuf + (blkoff >> 3);
        bitmsk = (0x80) >> ((int)blkoff & 0x7);
        drawmap(i, *bytptr & bitmsk);
    }

		dumpframebuffer();
		
    /* show stats beneath map */
    i  = (mapsize * 8 / BLOCKSPERCYLINDER) / 80 * 8;
    i += (mapsize * 8 / 640);
    j = 1 + (i / 15);
    printf("\033[%dH", j + 1);
    printf("%d files,  %d directories\n", regfdns, dirfdns);
    printf("%d free Fdns  %d cylinders\n", freefdns, i);
    printf("\033[%d;32r", j + 1);

    /* find candidates to move and find space */
    for(i=0; i< numcandidates; i++)
    {
        short minc, maxc;
				int newaddr;

        /* gather block info but no map updating */
        mappingmode = BUILDFILE;
        regblks = dirblks = 0;
        mark_in_map(&candidates[i].inode);

        _l4tol(&currfdlayout.real_filesize, candidates[i].inode.fd_siz, 1);

				/* find a contiguous set of blocks */
				newaddr = findcontiguous(currfdlayout.numblocks);
				if (newaddr)
				{
						printf("%s: moving %d bytes to block [%d - %d]\n", candidates[i].filepath, currfdlayout.real_filesize, newaddr, newaddr + currfdlayout.numblocks - 1);

						/* free up current blocks */
						for (j = 0; j < currfdlayout.numblocks; j++)
						{
								int mindex = currfdlayout.blocks[j] - minvld;
								clrbit(currfdlayout.blocks[j]);
								drawcylinder(mindex / BLOCKSPERCYLINDER, countbits(mindex, BLOCKSPERCYLINDER));
								drawmap(mindex, 0);
						}
	dumpframebuffer();
	
						/* allocate new blocks */
						for (j = 0; j < currfdlayout.numblocks; j++)
						{
								int mindex = (newaddr + j) - minvld;
								if (setbit(newaddr + j))
										fprintf(stderr, "findcontiguous wrong\n");
								drawcylinder(mindex / BLOCKSPERCYLINDER, countbits(mindex, BLOCKSPERCYLINDER));
								drawmap(mindex, 1);
						}
	dumpframebuffer();
	
				}
    }
}

void free_check()
{
    short no_rebuild;
    char *badfree;

    no_rebuild = 0;
    badfree = "Free list bad.\n";
    if(v_opt) printf("*** Phase 5 - Check free list\n");
    if(must_newfree) {
        do_fdns(2);
        fxdbcnt();
    }

    if(must_newfree) {
        if(v_opt) printf("%s",badfree);
    }
    do_free();    /* check volume free list */
    chk_miss();      /* look for missing */
}


long do_frbk(count,blkptr)
short count;
char *blkptr;
{
    long blkadrs[FBPER];

    l3tol(blkadrs,blkptr,count);
    for(--count ; count>=0; --count) {
        if((blkadrs[count]>=minvld)&&(blkadrs[count]<=maxvld)) {
            ++freeblks;
            if(setbit(blkadrs[count])) {
                if(v_opt)
                    printf("Block %ld duplicated in free list.\n",blkadrs[count]);
                ++dupfbks;
            }
        }
        else {
            if(blkadrs[count] || count) ++oorfbks;
        }
    }
    return(blkadrs[0]);
}


/***************************************************************/


void init_gfb()
{
    fbptr = mapbuf+(available>>3);
    fbbit = available & 0x7;
    if(fbbit) *fbptr = *fbptr >> (8-fbbit);
    fbblock = maxvld+1;
    freeblks = 0;
}



long gnxtfb()
{

    do {
        while (fbbit) {
            --fbbit;
            --fbblock;
            if((*fbptr & 0x01) == 0) {
                *fbptr = *fbptr >> 1;
                ++freeblks;
                return(fbblock);
            }
            *fbptr = *fbptr >> 1;
        }
        if(*(--fbptr) != 0xff) fbbit = 8;
        else fbblock -= 8;
    } while (fbptr >= mapbuf);

    fbbit = 0;
    return(0);
}


/***************************************************
*  opnfbks - open file for retrieval of blocks
*
*  Reads fdn specified into structure at "map_ptr"
*  and sets up appropriate values in that structure.
*  Returns 0 if all ok, ERR if problem.
****************************************************/

opnfbks(fdn,map_ptr)
unsigned fdn;
struct mapstr *map_ptr;
{
  if(rdfdn(fdn,map_ptr->mapb) == ERR)
    return(ERR);
  l3tol(map_ptr->indrct, map_ptr->mapb+9+(FMSZ*3), 3);

  _l4tol(&(map_ptr->msize), map_ptr->mapb+5, 1);

  map_ptr->mtype = *(map_ptr->mapb);
  map_ptr->nxtblk = 0;
  return(0);
}


rdmapb(buffer,bkadr)
char *buffer;
long bkadr;
{
   long lseek();

   if((lseek(fd_sir,bkadr<<9L,0) != bkadr<<9L) || (read(fd_sir,buffer,SMSZ*3) != SMSZ*3))  
   {
      printf(errrdgb,bkadr);
      contin(bkadr);
      return(ERR);
   } 
   else
      return(0);
}


/****************************************************
*  gfblks - get file blocks one at a time
*
*  Returns address of next block in file.  Returns 0
*  if end of file.  Returns ERR if problem.  Skips over
*  block addresses of zero.  Currently does not handle
*  triple indirection.
*****************************************************/

long gfblks(map_ptr)
struct mapstr *map_ptr;
{
  long blkadr;

  while(map_ptr->nxtblk < FMSZ) {
    l3tol(&blkadr,map_ptr->mapb+9+(3*map_ptr->nxtblk),1);
    map_ptr->nxtblk += 1;
    if(blkadr)
      return(blkadr);
  }
  do {
    if((((map_ptr->nxtblk) - FMSZ) & 0x7f) == 0) {
      if(map_ptr->nxtblk == FMSZ) {
        if((blkadr=map_ptr->indrct[0]) == 0)
          map_ptr->nxtblk += SMSZ;
      }
      if((map_ptr->nxtblk > FMSZ) && (map_ptr->nxtblk < BGTMSZ)) {
        if((blkadr=map_ptr->indrct[1]) == 0) {
          map_ptr->nxtblk = BGTMSZ;
        } else {
          if(rdmapb(map_ptr->mapb,blkadr) == ERR)
            return(ERR);
          while(map_ptr->nxtblk < BGTMSZ) {
            l3tol(&blkadr,map_ptr->mapb+((((map_ptr->nxtblk)-FMSZ-SMSZ)/SMSZ)*3),1);
            if(blkadr)
              break;
            else
              map_ptr->nxtblk += SMSZ;
          }
        }
      }
      if(map_ptr->nxtblk >= BGTMSZ) {
        if((blkadr=map_ptr->indrct[2]) == 0) {
          return(0);
        } else {
          printf("Badblocks file too large.\n");
          exit(1);
        }
      }
      if(rdmapb(map_ptr->mapb,blkadr) == ERR)
        return(ERR);
    }
    l3tol(&blkadr,map_ptr->mapb+((((map_ptr->nxtblk)-FMSZ)&0x7f)*3),1);
    map_ptr->nxtblk += 1;
  } while (blkadr == 0);

  return(blkadr);
}


main(argc,argv)
int argc;
char *argv[];

{
   long freeblk, volsize, lseek();
   long fdnsize, swapbeg;
   long scratch;
   USHORT swapsize;
   char *rootname;
   struct stat statbuf;
   short rwmode, i, j;
   USHORT devno, sodevno;

#ifdef __clang__
#define phys(A)	malloc(128 * 480);
	devname = "/Users/adambillyard/projects/tek4404/mame/bar.chd";
#else
   if (argc > 1)
    devname = argv[1];
   sync();        /* update all buffers to disk */
#endif

   rwmode = (1) ? O_RDONLY : O_RDWR;
   fd_sir = open(devname,rwmode);
   if (fd_sir == ERR)
	 {
       printf("Can't open device.\n");
       exit(255);
   }
#ifndef __clang__
   fstat(fd_sir,&statbuf);
   
   if ((statbuf.st_mode & S_IFMT) != S_IFBLK) {
      printf("Not a block device.\n");
      exit(255);
   }
#endif

   if(rdblk(SIRADR,sir_area) == ERR) {
       printf("Can't read System Information Record.\n");
       exit(255);
   }
   sirbuf = sir_area;

#ifndef __clang__
   devno = statbuf.st_size & 0xffff;

   /* check stdout not using disk */
   if(fstat(STD_OUT,&statbuf) == ERR) {
       fprintf(stderr,"Can't stat std. output.\n");
       exit(255);
   }
   sodevno = statbuf.st_dev;
   if((statbuf.st_mode & S_IFMT) == S_IFCHR)
      sodevno = 0xffff; /* any character device is OK */
   if((statbuf.st_mode & S_IFMT) == S_IFBLK)
       sodevno = statbuf.st_size & 0xffff;

   if(devno == sodevno) {
       fprintf(stderr,odtdut);
       exit(255);
   }
#endif

   /* sanity check disk config */
   l3tol(&maxvld,sirbuf->ssizfr,1);
   _l2tos(&sszfdn, &sirbuf->sszfdn, 1);
   minvld = sszfdn + 2;
   available = maxvld-minvld+1L;
   if(available > MAXBLKS-4L) {
      printf("Disk too large or bad size in SIR.\n");
      exit(255);
   }

   fdnsize = (long)(sszfdn) & 0x0ffffL;
   if((fdnsize >= maxvld-2L) || (fdnsize > 8192L)) {
       printf("SIR fdn block count error: %ld\n",fdnsize);
       exit(255);
   }

   v_opt = 1;
   modified = 0;
   must_newfree = must_sir = 0;

   /* get swap space info */
   l3tol(&swapbeg, sirbuf->sswpbg, 1);
   if(swapbeg != 0)
   {
        _l2tos(&swapsize, sirbuf->sswpsz, 1);
        lastblk = swapbeg + swapsize;

        if (maxvld >= swapbeg) {
            if(v_opt) printf("Data overlaps swap space.\n");
            exit(255);
        }
        if(scratch = swapbeg-maxvld-1)
            printf("Warning: %ld unassigned blocks between data and swap space.\n",scratch);
   }
   else 
   {
        lastblk = maxvld +1L;
   }

   /* initialize memory buffer pointer to block map (1-bit per block) */
   mapsize = lastblk / 8;
   mapbuf = (char*)malloc(mapsize);

   framebuffer = phys(1);

    do_fdns(0); 

#if 0
    fdncheck();

    free_check();


    printf("\n\nDisk Summary\n\nFiles:\n");

    printf("%8ld total files:\n",regfdns+dirfdns+blkfdns+chrfdns);
    printf("%8ld regular files\n",regfdns);

    printf("%8ld directories\n",dirfdns);
    printf("%8ld block device files\n",blkfdns);
    printf("%8ld character device files\n",chrfdns);

    printf("\nBlock usage:\n");
    printf("%8d blocks reserved for swap space\n",swapsize);

    printf("%8ld unused blocks\n",freeblks);
    printf("%8ld total used blocks:\n",regblks+dirblks+mapblks+sirbuf->sszfdn+2);
    printf("%8ld regular blocks\n",regblks);
    printf("%8ld directory blocks\n",dirblks);
    printf("%8ld blocks used for file mapping\n",mapblks);
    printf("%8u fdn blocks\n",sirbuf->sszfdn);
    printf("%8d system blocks\n",2);

    printf("\nFdn usage:\n");
    printf("%8ld busy fdns\n",usedfdns);
    printf("%8ld free fdns\n",freefdns);

#endif

    free(mapbuf);

    close(fd_sir);

}

