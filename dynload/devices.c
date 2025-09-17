#include "uniflex/userbl.h"
#include "uniflex/task.h"

/* char devices are device major */
typedef struct
{
  char *open;
  char *close;
  char *read;
  char *write;
  char *special;	
} CDfuncs;

/* block devices are function major, device minor */
typedef struct
{
  char *minorfunc[9];
} BDfuncs;

typedef struct
{
  BDfuncs *BDopen;	
  BDfuncs *BDclose;	
  BDfuncs *BDio;	
} BDTable;

/* itoa without using mul or div! */
int units[] = {10000,1000,100,10,1,0};
void kitoa(buffer, a)
char *buffer;
int a;
{
  int i;
  int maxval,column;

  column = 0;
  while (maxval = units[column++])
  {
    i = 0;
    while(a >= maxval)
    {
      a -= maxval;
      i++;
    }
    if (i)
      *buffer++ = '0' + i;

  }
  /* special case for last digit being zero */
  if (i == 0)  
    *buffer++ = '0';

  *buffer++ = '\0';
	
}

void kprintd(fmt, a)
char *fmt;
int a;
{
  char output[16];

  kprint(fmt);
  kitoa(output, a);
  kprint(output);
}


/* block device methods */
int rd_open(userblk,majmin)
struct userbl *userblk;
int majmin;
{
  kprinthex("rd_open: dev=",majmin & 255,4);
  kprinthex(" userblk=",userblk, 8);
  kprint("\n");  

  return 0;
}

int rd_close()
{
   kprint("rd_close\n");

  return 0;
}

int rd_io(rw)
int rw;
{
  kprinthex("rd_io: ", rw,4);
  kprint("\n");  

  return 0;
}

void installBD0(blktab,minor)
BDTable *blktab;
int minor;
{
  blktab->BDopen->minorfunc[minor] = rd_open;
  blktab->BDclose->minorfunc[minor] = rd_close;
  blktab->BDio->minorfunc[minor] = rd_io;
	
  kprinthex("block device is major(0x00) minor(0x",minor, 2);
  kprint(")\n");  
}

char devname[8];
/* char device methods */
cd_open(userblk,majmin)
struct userbl *userblk;
int majmin;
{
  struct task *atask;
  char tmp[8];
     
  kprint("cd_open\n");

  /* make ascii device major.minor */
  kitoa(devname, (majmin >> 8));
  kstrcat(devname, ".");
  kitoa(tmp, (majmin & 255));
  kstrcat(devname, tmp);
}
cd_close(userblk,majmin)
struct userbl *userblk;
int majmin;
{
  kprint("cd_close\n");
}
static unsigned long newrnd = 291964;
cd_read(userblk,majmin)
struct userbl *userblk;
int majmin;
{
  struct task *atask;
  char mesg[80],tmp[8];
  int len,i;
  char c;
   
  c = majmin & 255;
  if (c==0)
  {
    if (userblk->uipos == 0)
    {
      mesg[0] = 0;
      kstrcat(mesg, "This device is: ");
      kstrcat(mesg, devname);

	  atask = (struct task *)userblk->utask;
      kstrcat(mesg, "\nCalled from task:");
	  kitoa(tmp, (int)atask->tstid);
	  kstrcat(mesg, tmp);
      kstrcat(mesg, "\n");

      kpassc(mesg, kstrlen(mesg));
    }
  }    
  if (c==1)
  {
    if (userblk->uipos == 0)
    {
    	int swapsize;
		char *tty_struct;
		struct mt *arun;
		unsigned int *page_entry;
		unsigned int *paddr;
		unsigned int argcee;
		unsigned char *argvee;
		
		atask = (struct task *)userblk->utask;

		mesg[0] = 0;
		kstrcat(mesg, "pid:");
		kitoa(tmp, (int)atask->tstid);
	 	kstrcat(mesg, tmp);

		swapsize = 4 * (userblk->usizet+userblk->usized+userblk->usizes);
		kstrcat(mesg," size:");
		kitoa(tmp, swapsize);
	 	kstrcat(mesg, tmp);
		kstrcat(mesg,"K");

		tty_struct = (char *)atask->tstty + 0x1c;
		kstrcat(mesg," term:tty");
		kitoa(tmp, (int)tty_struct[2]);
	 	kstrcat(mesg, tmp);

		/* read user stack page entry for oldest page in chunk */
		arun = (struct mt *)(userblk->umem);		
		page_entry = arun[2].paddr + (arun[2].numpages - 1) * 4;
		
		/* remove permissions bits */
		paddr = (*page_entry & 0xfffff) << 12;

		argcee = paddr[0];
		kstrcat(mesg," cmd:");
		for(i=0; i<argcee; i++)
		{
			argvee = (char *)paddr + (paddr[1+i] & 0xfff);
			kstrcat(mesg, argvee);
			kstrcat(mesg, " ");
        }    	
		kstrcat(mesg, "\n");
		kpassc(mesg, kstrlen(mesg));
	}
  }   
  if (c==7)
  {
    newrnd ^= (newrnd << 13);
    newrnd ^= (newrnd >> 17);
    newrnd ^= (newrnd << 5);
    c = newrnd >> 24;
    kpassc(&c, 1);
  }   
  
}
cd_write(userblk, majmin)
struct userbl *userblk;
int majmin;
{

  char buffer[128];
  int len;
  
  kprint("cd_write\n");

  len = kcpass(buffer, sizeof(buffer));
  buffer[len] = 0;
  kprint(buffer);  
  kprint("\n");
  
}
cd_special()
{
  kprint("cd_special\n");
}

#define CDMAJOR 10
int cdinit(chrdev)
CDfuncs *chrdev;
{

  chrdev->open = cd_open;
  chrdev->close = cd_close;
  chrdev->read = cd_read;
  chrdev->write = cd_write;
  chrdev->special = cd_special;

  kprinthex("char device is major(0x",CDMAJOR, 2);
  kprint(")\n");  
  
  return(CDMAJOR);
}
 
