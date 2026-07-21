#include <stdio.h>
#include <errno.h> 
#include <signal.h>
#include <sys/fcntl.h>
#include <net/select.h>
#include <net/netdev.h>
#include <net/fr.h>

#ifndef __clang__
#include <net/inet.h>
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

typedef int socklen_t;

void setsid() {}

#else

extern int open();

#define in_sockaddr sockaddr_in

#include "uniflexshim.h"

#endif

extern char *memset();



/************************************************/
unsigned char dbuffer[2048];
char filepath[80];

char * readmem(dst, src, structlen)
char *dst;
char *src;
int structlen;
{
  int fd;
  
 fd = open("/dev/pmem", O_RDONLY);
 if (fd > 0)
 {
   lseek(fd, (int)src, 0);

   read(fd, dst, structlen);

   close(fd); 	
 }

 return dst;
}


int printif(ptr)
netdev *ptr;
{
  int i;
  frent afrent, *fr;
  
  printf("IF name: %s (%s)\n", 
    ptr->nd_name, ptr->nd_flags & F_N_ONLINE ? "ONLINE" : "OFFLINE");

  i = *(unsigned int *)(ptr->nd_addrs[AF_INET].sa_data + 2);
  printf("\tinet: %s\n", inet_ntoa(i));

  /* ptr->nd_lladdr.a_len == 6 */
  printf("\tether: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
      (unsigned char)ptr->nd_lladdr.a_ena.a6[0],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[1],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[2],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[3],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[4],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[5]);

  printf("\trecv count: %d\n", ptr->nd_stat.sb_rcnt);
  printf("\txmit count: %d\n", ptr->nd_stat.sb_xcnt);
  printf("\trecv error: %d\n", ptr->nd_stat.sb_recnt);
  printf("\txmit error: %d\n", ptr->nd_stat.sb_xecnt);
#if 0  
  printf("nd_sop: %8x\n", ptr->nd_sop);
  printf("nd_ioctl: %8x\n", ptr->nd_ioctl);
  printf("nd_next: %8x\n", ptr->nd_next);
#endif

  /* walk fusion router entries (NB 1st is null entry) */
  fr = ptr->nd_re;
  if (fr)
  {
    /* skip */
    fr = readmem(&afrent, fr, sizeof(frent));
    fr = fr->re_ndcU.rpu_p;

    printf("\t%-8s%-8s%-8s%-8s%-14s%-14s\n", 
    	"flags","hops","rate","delay","rte_net","rte_gwy");
    while (fr)
    {
       fr = readmem(&afrent, fr, sizeof(frent));

       printf("\t%-8.4x%-8d%-8d%-8d", 
   	(int)fr->re_flags,(int)fr->re_hops, (int)fr->re_rate, fr->re_delay);

       i = *(unsigned int *)(fr->re_rte.rte_net.sa_data + 2);
       printf("%-14s", inet_ntoa(i));

       i = *(unsigned int *)(fr->re_rte.rte_gwy.sa_data + 2);
       printf("%-14s\n", inet_ntoa(i));
  
      fr = fr->re_ndcU.rpu_p;
    }
  }

}

void setting(name, value)
char *name;
int value;
{
int rc;

  rc = wdiddle(name, &value, sizeof(value));
  printf("%s(%d) => %d ", name, value, rc);
}

void printsetting(name)
char *name;
{
int value;
int rc;

  value = 0;
  rc = rdiddle(name, &value, sizeof(value));
  printf("%s(%d) => %d ", name, value, rc);
}

void printsettingstring(name)
char *name;
{
char value[128];

  rdiddle(name, value, sizeof(value));
  printf("%s(%s) ", name, value);
}

main(argc,argv)
int argc;
char **argv;
{
  int value,dbufferlen,rc,i,j,nde_size;
  unsigned char *ptr;
  char filepath[256];
  
  dbufferlen = ldiddle("ndevsw"); 			/* buffer len */
  ptr = rdiddle("ndevsw", dbuffer, dbufferlen);		/* buffer data */
  rdiddle("nde_size", &nde_size, sizeof(nde_size));	/* ndev struct size */

  rdiddle("fusion_db_name", filepath, sizeof(filepath), 0);
  printf("fusion_db = %s\n",filepath);

  /* attempt to write value */
  if (argc > 2)
  {
    value = atoi(argv[2]);
    setting(argv[1], value);	
  }


  printf("\nkernel settings\n");
  printsettingstring("sock_prefix");
  printsetting("so_cnt");
  printf("\n");
  printsetting("nc_hsize");
  printsetting("t_resolution");
  printsetting("re_cnt");
  printf("\n");
  printsetting("tcp_sq_max");
  printsetting("tcp_try_max");
  printsetting("tcp_ttl");
  printf("\n");

  printsetting("tcp_taccept");
  printsetting("tcp_tbind");
  printsetting("tcp_tclose");
  printsetting("tcp_tlinger");
  printf("\n");

  printsetting("tcp_debug");
  printsetting("tcp_trace");
  printf("\n");
  printsetting("udp_debug");
  printsetting("udp_trace");
  printf("\n");
  printsetting("ip_debug");
  printsetting("ip_trace");
  printf("\n");
  printsetting("icmp_debug");
  printsetting("icmp_trace");
  printf("\n");
  printsetting("fr_debug");
  printsetting("fr_trace");
  printf("\n");

  /* start of network interface blocks of nde_size bytes each */
  ptr = dbuffer;
  while (ptr - dbuffer < dbufferlen)
  {
    printif(ptr);

    ptr = ptr + nde_size;
  }


  return 0;
}

