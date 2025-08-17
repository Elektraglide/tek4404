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
  int minor = kgetD7();

  kprinthex("rd_open:",minor,4);
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
cd_open()
{
   kprint("cd_open\n");
	
}
cd_close()
{
   kprint("cd_close\n");
}
cd_read()
{
   kprint("cd_read\n");
}
cd_write()
{
   kprint("cd_write\n");
}
cd_special()
{
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
  kprinthex("Loading at address: 0x", init, 8);
  kprint("\n");

  installCD(chrtab, 10);
  installBD0(blktab, 3);
  
  return(0);
}
 
