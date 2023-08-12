//#define B42_COMPATIBLE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <signal.h>


#include <net/std.h>
#include <net/btypes.h>
#include <net/select.h>
#include <net/enet.h>
#define ETHER_ADDR_LEN 6
#define ETHERTYPE_IP            0x0800  /* IP protocol */
#define ETH_P_IP                0x0800
#define ETH_P_ALL               0x0003

#include <net/inet.h>
#include <net/ip.h>
#include <net/udp.h>
//#include <net/netdb.h>

#ifndef __clang__

#define B42_COMPATIBLE
#include <net/in.h>

typedef int fd_set;
#define FD_SET(A,SET)	*SET |= (1<<A)
#define FD_CLR(A,SET)	*SET &= ~(1<<A)
#define FD_ZERO(SET)	*SET = 0
#define FD_ISSET(A,SET)	(*SET & (1<<A))

void setsid() {}

#else

#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netinet/ip.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
//#include <netdb.h>

#include "uniflexshim.h"

// undo Tek4404 aliases
#undef gsbyname
#undef gsbyport


#endif

/*

native: cc +v pty_test.c +l=netlib

clang: cc -std=c89 -Wno-extra-tokens -DB42 -Itek_include -o dhcp dhcp.c

*/

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
    unsigned char    bp_options[0];
} dhcp_t;

#define DHCP_BOOTREQUEST                    1
#define DHCP_BOOTREPLY                      2

#define DHCP_HARDWARE_TYPE_10_ETHERNET      1

#define MESSAGE_TYPE_PAD                    0
#define MESSAGE_TYPE_REQ_SUBNET_MASK        1
#define MESSAGE_TYPE_ROUTER                 3
#define MESSAGE_TYPE_DNS                    6
#define MESSAGE_TYPE_DOMAIN_NAME            15
#define MESSAGE_TYPE_REQ_IP                 50
#define MESSAGE_TYPE_DHCP                   53
#define MESSAGE_TYPE_PARAMETER_REQ_LIST     55
#define MESSAGE_TYPE_END                    255

#define DHCP_OPTION_DISCOVER                1
#define DHCP_OPTION_OFFER                   2
#define DHCP_OPTION_REQUEST                 3
#define DHCP_OPTION_PACK                    4

#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define DHCP_MAGIC_COOKIE   0x63825363


int port;
int sock = -1;
int ip;

/*
 * Get MAC address of given link(dev_name)
 */
static int
get_mac_address(char *dev_name, unsigned char *mac)
{

#ifndef __clang__
    unsigned char address[] = {0xf4,0xd4,0x88,0x83,0x02,0xff };
    brd_host_addr(&address); /* MAC */
    memcpy((void *)mac, address, ETHER_ADDR_LEN);
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
            fprintf(stderr, "checking AF_LINK %s\n", p->ifa_name);
        
            struct sockaddr_dl* sdp;

            sdp = (struct sockaddr_dl*) p->ifa_addr;
            memcpy((void *)mac, sdp->sdl_data + sdp->sdl_nlen, ETHER_ADDR_LEN);
            break;
        }
    }
    freeifaddrs(ifap);
#endif

    fprintf(stderr,"MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    return 0;
}

/*
 * Ethernet output handler - Fills appropriate bytes in ethernet header
 */
static void
ether_output(eframe, mac, len)
enh *eframe;
uint8_t *mac;
int *len;
{
  *len += sizeof(enh);

  memcpy(&eframe->enh_source, mac, ETHER_ADDR_LEN);
  memset(&eframe->enh_dest, -1,  ETHER_ADDR_LEN);
  *(u16 *)&eframe->enh_type = htons(ETHERTYPE_IP);  // could this use EN_IP?
}

/*
 * Return checksum for the given data.
 * Copied from FreeBSD
 */
static unsigned short
in_cksum(addr, len)
unsigned short *addr;
int len;
{
    register int sum = 0;
    unsigned short answer = 0;
    register unsigned short *w = addr;
    register int nleft = len;
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *) w;
        sum += answer;
    }
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);             /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}

/*
 * IP Output handler - Fills appropriate bytes in IP header
 */
static void
ip_output(ip_header, len)
struct ip *ip_header;
int *len;
{
    *len += sizeof(struct ip);

// TODO: use iph_t which matches modern declaration

    ip_header->ip_hl = 5;
    ip_header->ip_v = IPVERSION;
    ip_header->ip_tos = 0x10;           /* IP_DELAY */
    ip_header->ip_len = htons(*len);
    ip_header->ip_id = htons(54321);
    ip_header->ip_off = 0;
    ip_header->ip_ttl = 16;
    ip_header->ip_p = IPPROTO_UDP;
    ip_header->ip_sum = 0;
    ip_header->ip_src.s_addr = 0x00000000;
    ip_header->ip_dst.s_addr = 0xffffffff;

    ip_header->ip_sum = in_cksum((unsigned short *) ip_header, sizeof(struct ip));
}

/*
 * UDP output - Fills appropriate bytes in UDP header
 */
static void
udp_output(udp_header, len)
udph_t *udp_header;
int *len;
{
    if (*len & 1)
        *len += 1;
    *len += sizeof(udph_t);

    *(u16 *)&udp_header->udph_sport = htons(DHCP_CLIENT_PORT);
    *(u16 *)&udp_header->udph_dport = htons(DHCP_SERVER_PORT);
    *(u16 *)&udp_header->udph_length = htons(*len);
    *(u16 *)&udp_header->udph_checksum = 0;
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
    /* NB clearing from top of structure leaving bp_options[] intact */
    *len += sizeof(dhcp_t);
    memset(dhcp, 0, sizeof(dhcp_t));

    dhcp->opcode = DHCP_BOOTREQUEST;
    dhcp->htype = DHCP_HARDWARE_TYPE_10_ETHERNET;
    dhcp->hlen = ETHER_ADDR_LEN;
    memcpy(dhcp->chaddr, mac, DHCP_CHADDR_LEN);

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
    unsigned char parameter_req_list[] = {MESSAGE_TYPE_REQ_SUBNET_MASK, MESSAGE_TYPE_ROUTER, MESSAGE_TYPE_DNS, MESSAGE_TYPE_DOMAIN_NAME};
    unsigned char option;

    option = DHCP_OPTION_DISCOVER;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_DHCP, &option, sizeof(option));
    req_ip = htonl(0xc0a8010a); /* 192.168.0.10 */
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_REQ_IP, (unsigned char *)&req_ip, sizeof(req_ip));
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_PARAMETER_REQ_LIST, (unsigned char *)&parameter_req_list, sizeof(parameter_req_list));
    option = 0;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_END, &option, sizeof(option));

    return len;
}

/*
 * Send DHCP DISCOVERY packet
 */
static int
dhcp_discovery(mac)
unsigned char *mac;
{
    int len = 0;
    unsigned char buffer[4096];
    unsigned char *packet;
    enh *en_header;
    struct ip *ip_header;
    udph_t *udp_header;
    dhcp_t *dhcp;

    fprintf(stderr, "Sending DHCP_DISCOVERY\n");

    en_header = (enh *)buffer;
    ip_header = (struct ip *)((char *)en_header + sizeof(enh));
    udp_header = (udph_t *)(((char *)ip_header) + sizeof(struct ip));
    dhcp = (dhcp_t *)(((char *)udp_header) + sizeof(udph_t));

    len = fill_dhcp_discovery_options(dhcp);
    dhcp_output(dhcp, mac, &len);
    udp_output(udp_header, &len);
    ip_output(ip_header, &len);
    //ether_output(en_header, mac, &len);
// do we need ether header or not?
    packet = (unsigned char *)ip_header;
    
    /* actually send it to DHCP server */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(DHCP_SERVER_PORT);
    server.sin_addr.s_addr = (unsigned int)inet_addr("192.168.0.1");
    server.sin_addr.s_addr = htonl ( INADDR_BROADCAST );

    fprintf(stderr, "Sending %d bytes to %s : %d\n", len, inet_ntoa(server.sin_addr.s_addr), DHCP_SERVER_PORT);
    if (sendto(sock, packet, len, 0, (struct sockaddr *)&server, sizeof(server)) != len)
    {
      return errno;
    }

    return 0;
}

int
main(argc,argv)
int argc;
char **argv;
{
  int rc;
  char *dev;
  unsigned char mac[6];
  unsigned char buffer[4096];
  

  /* make a raw socket,  NB needs elevated permission */
  sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);  //htons(ETH_P_ALL));
  if (sock < 0) {
    return -1;
  }

#ifndef __clang__

#else
  // we will be making IP header
  int hdrincl = 0;
  if (setsockopt(sock,IPPROTO_IP,IP_HDRINCL,&hdrincl,sizeof(hdrincl))==-1) {
    return -2;
  }
  
int broadcast=0;
if (setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast))==-1) {
    return -4;
}
#endif

  struct sockaddr_in client;
  memset(&client, 0, sizeof(client));
  client.sin_family = AF_INET;
  client.sin_port = htons(DHCP_CLIENT_PORT);
  client.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&client, sizeof(client)) < 0)
  {
    return -3;
  }

  /* Get the MAC address of the interface */
  dev = argv[1] ? argv[1] : "en0";
  rc = get_mac_address(dev, mac);
  if (rc)
  {
      return rc;
  }

  /* Send DHCP DISCOVERY packet */
  rc = dhcp_discovery(mac);
  if (rc)
  {
      return rc;
  }

  /* wait for an answer */
  //int replylen = sizeof(client);
  struct sockaddr_in src_addr;
  
  while (1)
  {
    socklen_t src_addr_len = sizeof(src_addr);
    memset(buffer, 0, sizeof(buffer));
    rc = recvfrom(sock, buffer, (int)sizeof(buffer), 0, (struct sockaddr *)&src_addr, &src_addr_len);
    printf("recvfrom: len=%d src=%s\n", rc, inet_ntoa(src_addr.sin_addr.s_addr));

    struct ip *ip_header = (struct ip *)buffer;
    printf("ip_header: len=%d protocol=%d\n", ip_header->ip_len, ip_header->ip_p);
    printf("ip_header: src=%s\n", inet_ntoa(ip_header->ip_src.s_addr));
    printf("ip_header: dst=%s\n", inet_ntoa(ip_header->ip_dst.s_addr));
    
    udph_t *udp_header = (udph_t *)((char *)ip_header + 48);  // why is sizeof(struct ip) != 68
    printf("udp_header: len=%d sport=%d dport=%d\n",
      ntohs(*(u16 *)&udp_header->udph_length),
      ntohs(*(u16 *)&udp_header->udph_sport),
      ntohs(*(u16 *)&udp_header->udph_dport));

    if (ntohs(*(u16 *)&udp_header->udph_sport) == DHCP_SERVER_PORT)
    {
      dhcp_t *dhcp = (dhcp_t *)((char *)udp_header + sizeof(udph_t));

      ip = ntohl(dhcp->yiaddr);

      printf("dhcp->opcode %d %s\n", dhcp->opcode, inet_ntoa((dhcp->yiaddr)));
      
      if (dhcp->opcode == DHCP_OPTION_OFFER)
      {
          ip = ntohl(dhcp->yiaddr);
          break;
      }
    }
  }
  
  exit(0);

}

