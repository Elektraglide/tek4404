
#info 68xxx UniFLEX (R) blockcheck
#info Version 2.07:0; Released October 1, 1985
#info Copyright (c) 1983, 1984, 1985 by
#info Technical Systems Consultants, Inc.
#info All rights reserved.
#tstmp

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

#define MAX_RUNS 255
struct blockrun {
    USHORT start, end;
};
struct blockrun runs[MAX_RUNS+1];
struct blockrun* runptr;
short run_cnt;

#define MAX_HISTOGRAM 18
short run_histogram[MAX_HISTOGRAM];

char sir_area[512];  /* buffer area for SIR */
struct sir *sirbuf;

short v_opt;      /* verbose option */
short fatal_error;   /* nonzero if fatal error */

long real_filesize; /* true filesize for size checking purposes */

short modified;      /* disk has been modified */
short must_sir;      /* must update SIR */
short must_frfdn; /* must fix free fdn count */
short must_fix;      /* disk is bad - needs fix */
short must_newfree;  /* must run newfree flag */

char *fbptr;      /* new free block pointer */
short fbbit;      /* new free block bit value */
long fbblock;     /* new free block address */

short phase;      /* 0=phase 1, 1=phase 1b, 2=redo fdns */
char *devname;
short fd_sir;
short filetype;
long freefdns, usedfdns, blkfdns;
long chrfdns, dirfdns, regfdns;
long regblks, dirblks, mapblks;
long freeblks,oorfbks,dupfbks;
long dupblks, oorblks, missbks;
long maxvld, minvld, available, *lptr;
long maxblk1, blkcount, filesize;
/*#* following should be unsigned short *#*/
unsigned short fdnno;

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

char *mapbuf;    /* map of used blocks */
int mapsize;

char* framebuffer;

main(argc,argv)
int argc;
char *argv[];

{
   long freeblk, volsize, lseek(), l3tol();
   long fdnsize, swapbeg;
   long scratch;
   USHORT swapsize;
   char *rootname;
   struct stat statbuf;
   short rwmode, i, j;
   USHORT devno, sodevno;

   if (argc > 1)
    devname = argv[1];

   sync();        /* update all buffers to disk */

   rwmode = (1) ? O_RDONLY : O_RDWR; 
   if (((fd_sir=open(devname,rwmode))==ERR)||(fstat(fd_sir,&statbuf)==ERR)) {
       printf("Can't open device.\n");
       exit(255);
   }
   if ((statbuf.st_mode & S_IFMT) != S_IFBLK) {
      printf("Not a block device.\n");
      exit(255);
   }
   if(rdblk(SIRADR,sir_area) == ERR) {
       printf("Can't read System Information Record.\n");
       exit(255);
   }
   sirbuf = sir_area;

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

   /* sanity check disk config */
   l3tol(&maxvld,sirbuf->ssizfr,1);
   minvld = sirbuf->sszfdn + 2;
   available = maxvld-minvld+1L ;
   if(available > MAXBLKS-4L) {
      printf("Disk too large or bad size in SIR.\n");
      exit(255);
   }

   fdnsize = (long)(sirbuf->sszfdn) & 0x0ffffL;
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
        maxblk1 = swapbeg + swapsize;

        if (maxvld >= swapbeg) {
            if(v_opt) printf("Data overlaps swap space.\n");
            exit(255);
        }
        if(scratch = swapbeg-maxvld-1)
            printf("Warning: %ld unassigned blocks between data and swap space.\n",scratch);
   }
   else 
   {
        maxblk1 = maxvld +1L;
   }

   /* initialize memory buffer pointer to block map (1-bit per block) */
   mapsize = maxblk1 / 8;
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

/***************************************************************/

free_check()
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



do_free()
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


reset()
{
    short i;

    udupcnt = 0;
    for(i=dups_cnt-1; i>=0; --i) {
        if(clrbit(dups[i].dup_b)) ++udupcnt;
    }
}

fxdbcnt()
{
    short i, j, k;

    for(i=0; i<dups_cnt; ++i) {
        k = 0;
        for(j=i-1; j>=0; --j)
            if(dups[i].dup_b == dups[j].dup_b) ++k;
        if(k==0) ++dupblks;
    }
}

/* count bits set in run */
int checkcylinder(i, cylsize)
int i, cylsize;
{
    short val = 0;
    register short bitmsk;
    register char* bytptr;
    register short loop;

    loop = cylsize;
    bytptr = mapbuf + (i >> 3);
    bitmsk = (0x80) >> (i & 7);
    while (loop > 0)
    {
        while (bitmsk)
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


/* 4x4 icon */
void drawcylinder(i,val)
int i,val;
{
    unsigned char* dst;
    short y = i / 160;
    short x = i % 160;

    dst = framebuffer + (y * 4 * 128) + (x / 2);
    if (x & 1)
    {
        dst[128] |= 0x04;
        if (val)
        {
            dst[0]   |= 0x0e;
            dst[128] |= (val == BLOCKSPERCYLINDER) ? 0x0e : 0x0a;
            dst[256] |= 0x0e;
        }
    }
    else
    {
        dst[128] |= 0x40;
        if (val)
        {
            dst[0]   |= 0xe0;
            dst[128] |= (val == BLOCKSPERCYLINDER) ? 0xe0 : 0xa0;
            dst[256] |= 0xe0;
        }
    }
}

/******************************************************************
*  do_fdns(phase)
*
*  Single argument is a phase number which implies the following:
*  phase:  Function:
*    0       Build allocated block table; everything cleared
*    1       Rescan for initial dups; returns soon as all initial
*            dups have been found; nothing cleared
*    2       Rebuild allocated table for newfree operation;
*            everything cleared
*    3       Read thru fdns to redo fdn counts only; does not build
*            allocated table; only fdn counts cleared
*    4       Rebuild allocated table for reconstruction of contiguous
*            file free.  Everything cleared except free regular block count
********************************************************************/

do_fdns(phase_a)
short phase_a;
{
    char fdnbuf[512];
    struct inode *fdnptr;
    short fdnbno;
    long tmp, swapbeg, gfblks();
    int i,j,k;
    USHORT swapsize,numcylinders;

    /* track FD */
    freefdns = usedfdns = 0;
    blkfdns = chrfdns = dirfdns = regfdns = 0;

    /* track blocks */
    freeblks = 0;
    regblks = dirblks = mapblks = 0;

    clear_map();

    /* track errors */
    dups_cnt = oors_cnt = 0;
    oorfbks = dupfbks = 0;
    dupblks = oorblks = missbks = 0;

    i = MAX_HISTOGRAM;
    while (i--)
        run_histogram[i] = 0;

    printf("reading %d file descriptors..\n", sirbuf->sszfdn);
    /* should this use ROOTFDN ? */
    fdnno = 1;
    for(fdnbno=1; fdnbno <= sirbuf->sszfdn; ++fdnbno)
    {
        /* read a block of 8 inodes */
        if (rdblk((long)fdnbno+1L,fdnbuf)==ERR) 
        {
            printf("Fdns %u-%u skipped.\n",fdnno,fdnno+7);
            fdnno += 8;
            continue;
        }

        /* walk inodes */
        fdnptr = (struct inode *)fdnbuf;
        for(fdnptr=fdnbuf; fdnptr<fdnbuf+512;++fdnptr) 
        {
            /* cache some info of current FD */
            _l4tol(&tmp, fdnptr->fd_siz, 1);
            real_filesize = tmp;
            filesize = (tmp + 511L) >> 9;
            filetype = fdnptr->fd_mod & S_IFMT;

            /* gather some stats */
            {
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
            }

            /* just regular files and directories */
            if((filetype==S_IFREG)||(filetype==S_IFDIR)) 
            {
                /* follow blocks and mark in map */
                mark_in_map(fdnptr);
            }

            ++fdnno;
        }
    }

    /* mark swap as used too */
    l3tol(&swapbeg, sirbuf->sswpbg, 1);
    if (swapbeg != 0)
    {
        _l2tos(&swapsize, sirbuf->sswpsz, 1);
        printf("%d block for swap\n", swapsize);
        for (i = 0; i < swapsize; i++)
        {
            setbit(swapbeg + i);
        }
    }

    /* clear screen and show map */
    numcylinders = (mapsize * 8) / BLOCKSPERCYLINDER;
    printf("\033[1H\033[J\033[%dH", 1 + ((numcylinders / 160) * 4) / 15  + 1);
    printf("%d files,  %d directories\n", regfdns, dirfdns);
    printf("%d free Fdns  %d cylinders\n", freefdns, numcylinders);

    printf("runs: ");
    for (i = 1; i < MAX_HISTOGRAM; i++)
        printf("%3d ", run_histogram[i]);
    printf("\n");

    for (i = 0; i < mapsize * 8; i += BLOCKSPERCYLINDER)
    {
        /* convert block_id to cylinder_id */
        drawcylinder(i / BLOCKSPERCYLINDER, checkcylinder(i, BLOCKSPERCYLINDER));
    }

}

mark_in_map(fdnptr)
struct inode *fdnptr;
{
    long blocks[13];
    register int i;

    blkcount = 0;
    l3tol(blocks,fdnptr->fd_blk,13);

    /* build contiguous runs  */
    runptr = runs;
    runptr->start = 0;
    runptr->end = 0;
    run_cnt = 0;

    /* 10 direct blocks */
    for(i=0; i<10; ++i) 
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

    /* histogram */
    if (run_cnt < MAX_HISTOGRAM)
        run_histogram[run_cnt]++;

    /* show stats of contiguous runs */
    if (0)
    if (run_cnt > 5 && run_cnt < 8)
    {
        int j;

        printf("%d: ", fdnno);
        j = 0;
        for (i = 0; i < run_cnt; i++)
        {
            printf("[%d - %d]", runs[i].start, runs[i].end);
            j += runs[i].end - runs[i].start + 1;
        }
        printf("\nNeeds contiguous %d blocks\n", j);
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


/***************************************************************/

chk_miss()
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


cnt_miss(cbits,twobyte)
short cbits;
short twobyte;
{
    short bit;

    for(bit=0; bit<cbits; ++bit) {
        if((twobyte & (1<<(15-bit)))==0) ++missbks;
    }
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

mapblock(blkadr)
long blkadr;
{
    ++blkcount;
    if((blkadr<minvld) || (blkadr>maxvld)) {
        if(badblocks && (fdnno == BBFDN)) {
            if((blkadr<3) || (blkadr>=maxblk1)) set_oor();
            return;
        } else {
            set_oor();
            return;
        }
    }

    if(filetype == S_IFDIR) ++dirblks; else ++regblks;
    if(setbit(blkadr)) set_dup(blkadr);

    /* track runs */
    if (runptr->end + 1 == blkadr)
    {
        /* extend current run */
        runptr->end = blkadr;
    }
    else
    {
        /* push new run */
        if (run_cnt < MAX_RUNS)
        {
            /* dont increment first time */
            if (run_cnt++)
                runptr++;
        }
        runptr->start = runptr->end = blkadr;
    }
}


mapindirectblock(blkadr)
long blkadr;
{
   if((blkadr<minvld) || (blkadr>maxvld)) {
       set_oor();
       return(ERR);
   }
   ++mapblks;
   if(setbit(blkadr))
       set_dup(blkadr);

   return(0);
}

/***************************************************************/

clear_map()
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

    blkoff = blkadr-minvld;
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

    blkoff = blkadr-minvld;
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

set_dup(blkadr)
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


set_oor()
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

rdblk(bkadr,buffer)
long bkadr;
char *buffer;
{
    long lseek();

    if((lseek(fd_sir,bkadr<<9L,0) != bkadr<<9L) || (read(fd_sir,buffer,512) != 512)) 
    {
       printf(errrdgb,bkadr);
       contin(bkadr);
       return(ERR);
    } 
    else 
    {
        return(0);
    }
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


rdfdn(fdn,buffer)
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



contin(badblk)
long badblk;
{
    if ((badblk > 2) && (badblk < maxblk1))
    {
        printf("Add to \".badblocks\"\n");
    }
}



init_gfb()
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
