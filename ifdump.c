#include <stdio.h>
#include <errno.h> 
#include <signal.h>
#include <sys/fcntl.h>
#include <net/select.h>
#include <net/netdev.h>


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

int printif(ptr)
netdev *ptr;
{
  int i;

  printf("-----\nIF name: %s (%s)\n", 
    ptr->nd_name, ptr->nd_flags & F_N_ONLINE ? "ONLINE" : "");

  i = *(unsigned int *)ptr->nd_addrs[AF_INET].sa_data;
  printf("local addr: %s\n", inet_ntoa(i));

  /* ptr->nd_lladdr.a_len == 6 */
  printf("MAC: %2x:%2x:%2x:%2x:%02x:%02x\n",
      (unsigned char)ptr->nd_lladdr.a_ena.a6[0],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[1],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[2],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[3],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[4],
      (unsigned char)ptr->nd_lladdr.a_ena.a6[5]);

  printf("recv count: %d\n", ptr->nd_stat.sb_rcnt);
  printf("xmit count: %d\n", ptr->nd_stat.sb_xcnt);
  printf("recv error: %d\n", ptr->nd_stat.sb_recnt);
  printf("xmit error: %d\n", ptr->nd_stat.sb_xecnt);
  printf("-----\n");
  
  printf("nd_sop: %8x\n", ptr->nd_sop);
  printf("nd_ioctl: %8x\n", ptr->nd_ioctl);
  printf("nd_next: %8x\n", ptr->nd_next);
}

main(argc,argv)
int argc;
char **argv;
{
  int n,rc,i,j,nde_size;
  unsigned char *ptr;

  n = ldiddle("ndevsw");
  printf("network interface buffer size => %d\n", n);

  ptr = rdiddle("ndevsw", dbuffer, n);
  printf("read network interface buffer => %x\n", ptr);

  rdiddle("nde_size", &nde_size, sizeof(nde_size));
  printf("nde_size(0x%x) netdev_size(0x%x)\n", nde_size, sizeof(netdev));

  /* start of network interface blocks of nde_size bytes each */
  ptr = dbuffer;
  while (ptr - dbuffer < n)
  {
    printif(ptr);

    ptr = ptr + nde_size;
  }


  return 0;
}

