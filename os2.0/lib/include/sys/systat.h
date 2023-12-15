/*******************************************************************/
/*                                                                 */
/*                "systat" system call structure                   */
/*                                                                 */
/*******************************************************************/

#define systat_h

struct sstat {
   char ss_config;   /* configuration number */
   char ss_ver;      /* release number */
   char ss_vendor;   /* vendor number */
   char ss_dum;      /* reserved */
   short ss_sn;      /* serial number */
   char ss_hdwr[2];  /* hardware information flags */
   long ss_memsiz;   /* total # bytes of physical memory */
                     /* Make structure 32 bytes in size */
   char ss_spare[32-(sizeof(long)-sizeof(short)-(6*sizeof(char)))];
};

/*    Hardware configuration flags */

#define SS_PROT  0x80    /* protected memory */
#define SS_EMT2  0x40    /* EMT2 protection */
#define SS_VM    0x20    /* System has virtual memory */
#define SS_LNAM  0x10    /* System has long filenames */
#define SS_68881 0x08    /* System has a 68881 FPU */
#define SS_68020 0x04    /* System has a 68020 */
