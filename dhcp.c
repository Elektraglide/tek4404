#define USE_SOCK_RAWxx
#define PACKET_TYPE IPPROTO_RAW /* IPPROTO_RAW */

/*

native: cc +v dhcp.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o dhcp  dhcp.c

*/


/* TEKTRONIX UNIFLEX HEADERS */

#define B42_COMPATIBLE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <signal.h>


#include <net/config.h>
#include <net/std.h>
#include <net/btypes.h>
#include <net/select.h>
#include <net/enet.h>
#include <net/netioc.h>
#define ETHER_ADDR_LEN 6
#define ETHERTYPE_IP            0x0800  /* IP protocol */
#define ETH_P_IP                0x0800
#define ETH_P_ALL               0x0003

#include <net/inet.h>
#include <net/ip.h>
#include <net/udp.h>

/* TEKTRONIX UNIFLEX HEADERS */

#ifndef __clang__

#define B42_COMPATIBLE
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

#define SO_BROADCAST 0x20

typedef int socklen_t;

void setsid() {}

#else

#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netinet/ip.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "uniflexshim.h"

// undo Tek4404 aliases
#undef gsbyname
#undef gsbyport
#undef getservbyname

#define IPPR_UDP IPPROTO_UDP

#endif

extern int rand();

/***********************************/
typedef unsigned int ip4_t;

#define DHCP_CHADDR_LEN 16
#define DHCP_SNAME_LEN  64
#define DHCP_FILE_LEN   128

/*
 * http://www.tcpipguide.com/free/t_DHCPMessageFormat.htm
 */
typedef struct dhcp
{
    unsigned char    opcode;
    unsigned char    htype;
    unsigned char    hlen;
    unsigned char    hops;
    unsigned int   xid;
    unsigned short   secs;
    unsigned short   flags;
    ip4_t       ciaddr;
    ip4_t       yiaddr;
    ip4_t       siaddr;
    ip4_t       giaddr;
    unsigned char    chaddr[DHCP_CHADDR_LEN];
    char        bp_sname[DHCP_SNAME_LEN];
    char        bp_file[DHCP_FILE_LEN];
    unsigned int    magic_cookie;
    unsigned char    bp_options[4];
} dhcp_t;

#define DHCP_BOOTREQUEST                    1
#define DHCP_BOOTREPLY                      2

#define DHCP_HARDWARE_TYPE_10_ETHERNET      1

#define MESSAGE_TYPE_PAD                    0
#define MESSAGE_TYPE_REQ_SUBNET_MASK        1
#define MESSAGE_TYPE_ROUTER                 3
#define MESSAGE_TYPE_DNS                    6
#define MESSAGE_TYPE_HOSTNAME                12
#define MESSAGE_TYPE_DOMAIN_NAME            15
#define MESSAGE_TYPE_REQ_IP                 50
#define MESSAGE_TYPE_DHCP                   53
#define MESSAGE_TYPE_PARAMETER_REQ_LIST     55
#define MESSAGE_TYPE_END                    255

#define DHCP_OPTION_DISCOVER                1
#define DHCP_OPTION_OFFER                   2
#define DHCP_OPTION_REQUEST                 3
#define DHCP_OPTION_PACK                    4


#define DHCP_BROADCAST_FLAG 32768

#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define DHCP_MAGIC_COOKIE   0x63825363

char DHCP_SERVER[20] = "255.255.255.255";

int verbose = 0;
int sock = -1;
int ip;
int packet_xid;

static void
print_buffer(buffer,len)
unsigned char *buffer;
int len;
{
  int i;
  if (!verbose)
    return;
 
 
  for (i = 0; i < len; i++)
  {
      if (i % 0x10 == 0)
          fprintf(stderr,"%04x :: ", i);
      fprintf(stderr,"%02x ", buffer[i]);
      if (i % 0x10 == 15)
          fprintf(stderr,"\n");
  }
  if (i % 0x10 != 0)
    fprintf(stderr,"\n");
}


/*
 * Get MAC address of given link(dev_name)
 */
static int
get_mac_address(dev_name, mac)
char *dev_name;
unsigned char *mac;
{
#ifndef __clang__
int n;
char *ifbuffer;

    /* behold this crazy magic NRC voodoo */

    n = ldiddle("ndevsw");
    ifbuffer = malloc(n);
    rdiddle("ndevsw", ifbuffer, n);
    memcpy((void *)mac, ifbuffer + 0x26e, ETHER_ADDR_LEN);
    free(ifbuffer);

#else
    struct ifaddrs *ifap, *p;

    if (getifaddrs(&ifap) != 0)
        return -1;

    for (p = ifap; p; p = p->ifa_next)
    {
        /* Check the device name */
        if ((strcmp(p->ifa_name, dev_name) == 0) &&
            (p->ifa_addr->sa_family == AF_LINK))
        {
            if (verbose)
              fprintf(stderr, "checking AF_LINK %s\n", p->ifa_name);
        
            struct sockaddr_dl* sdp;

            sdp = (struct sockaddr_dl*) p->ifa_addr;
            memcpy((void *)mac, sdp->sdl_data + sdp->sdl_nlen, ETHER_ADDR_LEN);
            break;
        }
    }
    freeifaddrs(ifap);
#endif

  if (verbose)
    fprintf(stderr,"MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    return 0;
}

/*
 * DHCP output - Just fills DHCP_BOOTREQUEST
 */
static void
dhcp_output(dhcp, mac, len)
dhcp_t *dhcp;
unsigned char *mac;
int *len;
{
 struct in_addr ipaddr;

    /* NB clearing from top of structure leaving bp_options[] intact */
    *len += sizeof(dhcp_t)-4;
    memset(dhcp, 0, sizeof(dhcp_t)-4);

    dhcp->opcode = DHCP_BOOTREQUEST;
    dhcp->htype = DHCP_HARDWARE_TYPE_10_ETHERNET;
    dhcp->hlen = ETHER_ADDR_LEN;
    memcpy(dhcp->chaddr, mac, ETHER_ADDR_LEN); /* mac[] is smaller than DHCP_CHADDR_LEN */

#ifndef __clang__
    /* inet_addr() adds +2 to location of IP result! */ 
    str_2_ipa(DHCP_SERVER, (char *)(&dhcp->siaddr) - 2);
#else
    dhcp->siaddr = inet_addr(DHCP_SERVER);
#endif

    packet_xid = htonl(rand());
    dhcp->xid = packet_xid;
    dhcp->secs = htons(0xFF);
#ifdef __clang__
    /* tell server it should broadcast its response */

    /* cannot get Uniflex to do BROADCAST, so disable */

    dhcp->flags = htons(DHCP_BROADCAST_FLAG);
#endif

    dhcp->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
}

/*
 * Adds DHCP option to the bytestream
 */
static int
fill_dhcp_option(packet, code, data, len)
unsigned char *packet;
unsigned char code;
unsigned char *data;
unsigned char len;
{
    packet[0] = code;
    packet[1] = len;
    memcpy(&packet[2], data, len);

    return len + (sizeof(unsigned char) * 2);
}

/*
 * Fill DHCP options
 */
static int
fill_dhcp_discovery_options(dhcp)
dhcp_t *dhcp;
{
    int len = 0;
    unsigned int req_ip;
    unsigned char parameter_req_list[4];
    unsigned char option;

    parameter_req_list[0] = MESSAGE_TYPE_REQ_SUBNET_MASK;
    parameter_req_list[1] = MESSAGE_TYPE_ROUTER;
    parameter_req_list[2] = MESSAGE_TYPE_DNS;
    parameter_req_list[3] = MESSAGE_TYPE_DOMAIN_NAME;

    option = DHCP_OPTION_DISCOVER;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_DHCP, &option, sizeof(option));

    req_ip = htonl(0xc0a8013d); /* 192.168.0.64 */
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_REQ_IP, (unsigned char *)&req_ip, sizeof(req_ip));

    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_PARAMETER_REQ_LIST, (unsigned char *)&parameter_req_list, sizeof(parameter_req_list));

    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_HOSTNAME, (unsigned char *)"TEK4404", 8);

    option = 0;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_END, &option, sizeof(option));

    return len;
}

/*
 * Send DHCP DISCOVERY packet
 */
int
dhcp_discovery(mac)
unsigned char *mac;
{
    int i,len = 0;
    unsigned char buffer[4096];
    unsigned char *packet;
    dhcp_t *dhcp_payload;
    struct sockaddr_in server;
    struct in_addr ipaddr;

    if (verbose)
      fprintf(stderr, "Sending DHCP_DISCOVERY\n");

    memset(buffer, 0, sizeof(buffer));
    dhcp_payload = (dhcp_t *)buffer;

    /* build payload */
    len = fill_dhcp_discovery_options(dhcp_payload);
    dhcp_output(dhcp_payload, mac, &len); packet = (unsigned char *)dhcp_payload;

    /* actually send it to DHCP server */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(DHCP_SERVER_PORT);
#ifndef __clang__
    /* inet_addr() adds +2 to location of IP result! */ 
    str_2_ipa(DHCP_SERVER, (char *)(&server.sin_addr.s_addr) - 2);
#else
    server.sin_addr.s_addr = inet_addr(DHCP_SERVER);
#endif

      if (verbose)
      fprintf(stderr, "Sending %d bytes to %s : %d\n", len, inet_ntoa(server.sin_addr.s_addr), ntohs(server.sin_port));
    print_buffer(packet, len);

    if (sendto(sock, packet, len, MSG_FDBROADCAST, (struct sockaddr *)&server, sizeof(server)) != len)
    {
      fprintf(stderr, "sendto: %s\n",strerror(errno));
      return errno;
    }
    if (verbose)
      fprintf(stderr, "SENT --------\n");
    return 0;
}

unsigned char buffer[4096];

int
main(argc,argv)
int argc;
char **argv;
{
  int i,rc,reuse;
  char *dev;
  unsigned char mac[6];
  struct sockaddr_in client;
  socklen_t src_addr_len;
  dhcp_t *dhcp;

  /* use this DHCP server address */
  if (argc > 1)
    strcpy(DHCP_SERVER, argv[1]);

  /* make a raw socket,  NB needs elevated permission */
  sock = socket(AF_INET, SOCK_DGRAM, IPPR_UDP);
  if (sock < 0) {
    fprintf(stderr, "socket: %s\n",strerror(errno));
    return -1;
  }

  reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  rc = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &reuse, sizeof(reuse));
  fprintf(stderr,"BROADCAST %d\n", rc);

  memset(&client, 0, sizeof(client));
  client.sin_family = AF_INET;
  client.sin_port = htons(DHCP_CLIENT_PORT);
  client.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&client, sizeof(client)) < 0)
  {
    fprintf(stderr, "bind: %s\n",strerror(errno));
    exit(errno);
  }

  /* Get the MAC address of the interface */
  dev = argv[1] ? argv[1] : "en0";
  rc = get_mac_address(dev, mac);
  if (rc)
  {
      exit(rc);
  }

  /* Send DHCP DISCOVERY packet */
  rc = dhcp_discovery(mac);
  if (rc)
  {
      exit(rc);
  }

  /* wait for an answer */
  while (1)
  {
    client.sin_family = AF_INET;
    client.sin_port = htons(DHCP_CLIENT_PORT);
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    src_addr_len = sizeof(client);
   

    memset(buffer, 0, sizeof(buffer));
    rc = (int)recvfrom(sock, buffer, sizeof(buffer), MSG_FDBROADCAST, (struct sockaddr *)&client, &src_addr_len);
    if (verbose)
      fprintf(stderr, "recvfrom: len=%d src=%s port=%d\n", rc, inet_ntoa(client.sin_addr.s_addr),ntohs(client.sin_port));
    print_buffer((unsigned char *)buffer, rc);
    if (rc <= 0)
    {
      fprintf(stderr, "recvfrom: %s\n",strerror(errno));
      exit(-5);
    }

    dhcp = (dhcp_t *)buffer;
    
    if (dhcp->opcode == DHCP_OPTION_OFFER && dhcp->xid == packet_xid)
    {
      ip = ntohl(dhcp->yiaddr);
      printf("%s", inet_ntoa(dhcp->yiaddr));
      break;
    }
  }
  
  close(sock);
  exit(0);

}

