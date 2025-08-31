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
  maxval = units[0];
  while (maxval > 0)
  {
    i = 0;
    while(a >= maxval)
    {
      a -= maxval;
      i++;
    }
    if (i)
      *buffer++ = '0' + i;
      
    maxval = units[++column];
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
int rd_open()
{
  int minor = get_majmin();
  struct userbl *userblk = get_userblk();

  kprinthex("rd_open: dev=",minor,4);
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
cd_open()
{
  struct userbl *userblk;
  struct task *atask;
  char tmp[8];
     
  save_reg_params();
  kprint("cd_open\n");

  userblk = (struct userbl *)get_userblk();

  kprinthex("fd=", get_fd(),8);
  kprint("\n");

  /* make ascii device major.minor */
  kitoa(devname, (get_majmin() >> 8));
  kstrcat(devname, ".");
  kitoa(tmp, (get_majmin() & 255));
  kstrcat(devname, tmp);
}
cd_close()
{
  save_reg_params();
  kprint("cd_close\n");
}
static long newrnd;
cd_read()
{
  struct userbl *userblk;
  struct task *atask;
  char mesg[64],tmp[8];
  int len;
  char c;
   
  save_reg_params();

  userblk = (struct userbl *)get_userblk();
  c = get_majmin() & 255;
  if (c==0)
  {
    if (userblk->uipos == 0)
    {
      mesg[0] = 0;
      kstrcat(mesg, "This device is: ");
      kstrcat(mesg, devname);

	  atask = (struct task *)userblk->utask;
      kstrcat(mesg, " called from task:");
	  kitoa(tmp, (int)atask->tstid);
	  kstrcat(mesg, tmp);
      kstrcat(mesg, "\n");

      kpassc(mesg, kstrlen(mesg));
    }
  }    
  if (c==1)
  {
	/* read stiml? */
    kpassc(&newrnd, 1);
  }   
  if (c==7)
  {
    newrnd = (newrnd << 1) ^ 0x88888eef;
    kpassc(&newrnd, 1);
  }   
  
}
cd_write()
{

  struct userbl *userblk;
  char buffer[128];
  int len;
  
  save_reg_params();
  kprint("cd_write\n");

  userblk = (struct userbl *)get_userblk();
  kprinthex("uicnt=",userblk->uicnt,8);
  kprinthex(" uipos=",userblk->uipos,8);
  kprinthex(" uistrt=",userblk->uistrt,8);
  kprint("\n");

  len = kcpass(buffer, sizeof(buffer));
  buffer[len] = 0;
  kprint(buffer);  
  kprint("\n");
  
  kprint("AFTER\n");
  kprinthex("uicnt=",userblk->uicnt,8);
  kprinthex(" uipos=",userblk->uipos,8);
  kprinthex(" uistrt=",userblk->uistrt,8);
  kprint("\n");
}
cd_special()
{
  save_reg_params();
  kprint("cd_special\n");
}

void installCD(chrtab, major)
CDfuncs *chrtab;
int major;
{
  CDfuncs *chrdev = chrtab + major;
  
  chrdev->open = cd_open;
  chrdev->close = cd_close;
  chrdev->read = cd_read;
  chrdev->write = cd_write;
  chrdev->special = cd_special;

  kprinthex("char device is major(0x",major, 2);
  kprint(")\n");  
}


int init(chrtab,blktab)
CDfuncs *chrtab;
BDTable *blktab;
{

  kprint("Installing devices\n");

  installCD(chrtab, 10);
  installBD0(blktab, 3);
  
  return(0);
}
 
