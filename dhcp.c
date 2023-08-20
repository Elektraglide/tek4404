#define USE_SOCK_RAWxx
#define PACKET_TYPE IPPROTO_UDP /* IPPROTO_RAW */
#define DHCP_SERVER "192.168.0.1"

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

/* TEKTRONIX UNIFLEX HEADERS */

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
#include <netdb.h>

#include "uniflexshim.h"

// undo Tek4404 aliases
#undef gsbyname
#undef gsbyport
#undef getservbyname

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
    unsigned char    bp_options[0];
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

int verbose = 0;
int sock = -1;
int ip;
int packet_xid;

static void
print_buffer(unsigned char *buffer, int len)
{
  if (!verbose)
    return;
  
  int i;
  for (i = 0; i < min(128, len); i++)
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
  *(u16 *)&eframe->enh_type = htons(ETHERTYPE_IP);  /* could this use EN_IP? */
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

/* TODO: use iph_t which matches modern declaration */

    ip_header->ip_hl = 5;
    ip_header->ip_v = IPVERSION;
    ip_header->ip_tos = 0x10;               /* IP_DELAY */
    ip_header->ip_len = (*len);
    ip_header->ip_id = 0xa45a;                   /* OS fills for us */
    ip_header->ip_off = 0;
    ip_header->ip_ttl = 64;
    ip_header->ip_p = PACKET_TYPE;
    ip_header->ip_sum = 0;
    ip_header->ip_src.s_addr = 0;           /* OS fills for us */
    ip_header->ip_dst.s_addr = inet_addr(DHCP_SERVER);

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
    memcpy(dhcp->chaddr, mac, ETHER_ADDR_LEN); /* mac[] is smaller than DHCP_CHADDR_LEN */

    dhcp->siaddr = inet_addr(DHCP_SERVER);
    packet_xid = htonl(rand());
    dhcp->xid = packet_xid;
    dhcp->secs = 0xFF;
    /* tell server it should broadcast its response */
    dhcp->flags = htons(DHCP_BROADCAST_FLAG);

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
    req_ip = htonl(0xc0a80040); /* 192.168.0.64 */
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
static int
dhcp_discovery(mac)
unsigned char *mac;
{
    int i,len = 0;
    unsigned char buffer[4096];
    unsigned char *packet;
    enh *en_header;
    struct ip *ip_header;
    udph_t *udp_header;
    dhcp_t *dhcp_payload;

    if (verbose)
      fprintf(stderr, "Sending DHCP_DISCOVERY\n");

    memset(buffer, 0, sizeof(buffer));
    en_header = (enh *)buffer;
    ip_header = (struct ip *)((char *)en_header + sizeof(enh));
    udp_header = (udph_t *)(((char *)ip_header) + sizeof(struct ip));
    dhcp_payload = (dhcp_t *)(((char *)udp_header) + sizeof(udph_t));

    /* build payload */
    len = fill_dhcp_discovery_options(dhcp_payload);
    dhcp_output(dhcp_payload, mac, &len); packet = (unsigned char *)dhcp_payload;

#ifdef USE_SOCK_RAW
    /* and wrappers */
    udp_output(udp_header, &len); packet = (unsigned char *)udp_header;
    ip_output(ip_header, &len); packet = (unsigned char *)ip_header;
    
    /* ether_output(en_header, mac, &len);  packet = (unsigned char *)en_header;  */
#endif

    /* actually send it to DHCP server */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(DHCP_SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(DHCP_SERVER);

    if (verbose)
      fprintf(stderr, "Sending %d bytes to %s : %d\n", len, inet_ntoa(server.sin_addr.s_addr), ntohs(server.sin_port));
    print_buffer(packet, len);

    if (sendto(sock, packet, len, 0, (struct sockaddr *)&server, sizeof(server)) != len)
    {
      fprintf(stderr, "sendto: %s\n",strerror(errno));
      return errno;
    }
    if (verbose)
      fprintf(stderr, "SENT --------\n");
    return 0;
}

int
main(argc,argv)
int argc;
char **argv;
{
  int i,rc;
  char *dev;
  unsigned char mac[6];
  unsigned char buffer[4096];
  struct sockaddr_in client;

  /* make a raw socket,  NB needs elevated permission */
#ifdef USE_SOCK_RAW
  sock = socket(AF_INET, SOCK_RAW, PACKET_TYPE);
#else
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
  if (sock < 0) {
    fprintf(stderr, "socket: %s\n",strerror(errno));
    return -1;
  }

  char reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
  char broadcast = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

#ifdef USE_SOCK_RAW
  int headers  = 1;
  setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &headers, sizeof(headers));
#endif

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
    socklen_t src_addr_len = sizeof(client);
    
    memset(buffer, 0, sizeof(buffer));
    rc = (int)recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &src_addr_len);
    if (verbose)
      fprintf(stderr, "recvfrom: len=%d src=%s port=%d\n", rc, inet_ntoa(client.sin_addr.s_addr),ntohs(client.sin_port));
    print_buffer((unsigned char *)buffer, rc);
    if (rc <= 0)
    {
      fprintf(stderr, "recvfrom: %s\n",strerror(errno));
      exit(-5);
    }

    dhcp_t *dhcp = (dhcp_t *)buffer;
#ifdef USE_SOCK_RAW
    struct ip *ip_header = (struct ip *)buffer;
    if (verbose)
    {
      fprintf(stderr,"ip_header: len=%d protocol=%d header=%d\n", ip_header->ip_len, ip_header->ip_p, ip_header->ip_hl*4);
      fprintf(stderr,"ip_header: src=%s\n", inet_ntoa(ip_header->ip_src.s_addr));
      fprintf(stderr,"ip_header: dst=%s\n", inet_ntoa(ip_header->ip_dst.s_addr));
    }
    
    if (ip_header->ip_p != PACKET_TYPE)
    {
      continue;
    }

    udph_t *udp_header = (udph_t *)((char *)ip_header + ip_header->ip_hl * sizeof(int) );
    if (verbose)
    {
      fprintf(stderr,"udp_header: len=%d sport=%d dport=%d\n",
        ntohs(*(u16 *)&udp_header->udph_length),
        ntohs(*(u16 *)&udp_header->udph_sport),
        ntohs(*(u16 *)&udp_header->udph_dport));
    }

    if (ntohs(*(u16 *)&udp_header->udph_sport) != DHCP_SERVER_PORT)
    {
      continue;
    }

    dhcp = (dhcp_t *)((char *)udp_header + sizeof(udph_t));
#endif

    if (dhcp->opcode == DHCP_OPTION_OFFER && dhcp->xid == packet_xid)
    {
      ip = ntohl(dhcp->yiaddr);
      printf("%s\n", inet_ntoa(dhcp->yiaddr));
      break;
    }
  }
  
  close(sock);
  exit(0);

}

