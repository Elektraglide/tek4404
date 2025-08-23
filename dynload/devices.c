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

/* char device methods */
cd_open(a,b)
int a,b;
{
  struct userbl *userblk;
  
  save_reg_params();
  
   kprinthex("cd_open ",a,8);
   kprinthex(" ",b, 8);
   kprint("\n");

  userblk = (struct userbl *)get_userblk();
  kprinthex("usarg0 ",userblk->usarg0,8);
  kprinthex(" usarg1 ",userblk->usarg1,8);
  kprint("\n");

}
cd_close(a,b)
int a,b;
{
  save_reg_params();

   kprinthex("cd_close ",a,8);
   kprinthex(" ",b, 8);
   kprint("\n");
}
cd_read(a,b)
int a,b;
{
  struct userbl *userblk;

  save_reg_params();

   kprinthex("cd_read ",a,8);
   kprint("\n");

  userblk = (struct userbl *)get_userblk();
  kprinthex("uicnt ",userblk->uicnt,8);
  kprinthex(" uipos ",userblk->uipos,8);
  kprinthex(" uistrt ",userblk->uistrt,8);
  kprint("\n");
  
}
cd_write(a,b)
int a,b;
{

  struct userbl *userblk;
  char buffer[128];
  int len;
  
  save_reg_params();

  kprint("cd_write\n");

  userblk = (struct userbl *)get_userblk();
  kprinthex("uicnt ",userblk->uicnt,8);
  kprinthex(" uipos ",userblk->uipos,8);
  kprinthex(" uistrt ",userblk->uistrt,8);
  kprint("\n");

  len = kcpass(buffer, sizeof(buffer));
  buffer[len] = 0;
  kprint(buffer);  
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
 
