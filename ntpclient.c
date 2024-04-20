/*
 *
 * (C) 2014 David Lettier.
 *
 * http://www.lettier.com/
 *
 * NTP client.
 *
 * Compiled with gcc version 4.7.2 20121109 (Red Hat 4.7.2-8) (GCC).
 *
 * Tested on Linux 3.8.11-200.fc18.x86_64 #1 SMP Wed May 1 19:44:27 UTC 2013 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc main.c -o ntpClient.out
 *
 * Usage: $ ./ntpClient.out
 *
 */

/*
	requires line to be added /etc/netHosts.db:
	
	ntp.org :NTP            :ethernet::51.145.123.29:
	
	and IP route in /etc/net.db
 */
 
#define B42_COMPATIBLE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <net/config.h>
#include <net/std.h>
#include <net/btypes.h>
#include <net/in.h>
#include <net/socket.h>
#include <net/netdb.h>

#define uint8_t unsigned char
#define uint32_t unsigned int

#define NTP_TIMESTAMP_DELTA (2524608000L   )

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) /* (li   & 11 000 000) >> 6 */
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) /* (vn   & 00 111 000) >> 3 */
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) /* (mode & 00 000 111) >> 0 */

/* Structure that defines the 48 byte NTP packet protocol. */

typedef struct
{

	uint8_t li_vn_mode;      /* Eight bits. li, vn, and mode. */
													 /* li.   Two bits.   Leap indicator.*/
													 /* vn.   Three bits. Version number of the protocol.*/
													 /* mode. Three bits. Client will pick mode 3 for client.*/

	uint8_t stratum;         /* Eight bits. Stratum level of the local clock.*/
	uint8_t poll;            /* Eight bits. Maximum interval between successive messages.*/
	uint8_t precision;       /* Eight bits. Precision of the local clock.*/

	uint32_t rootDelay;      /* 32 bits. Total round trip delay time.*/
	uint32_t rootDispersion; /* 32 bits. Max error aloud from primary clock source.*/
	uint32_t refId;          /* 32 bits. Reference clock identifier.*/

	uint32_t refTm_s;        /* 32 bits. Reference time-stamp seconds.*/
	uint32_t refTm_f;        /* 32 bits. Reference time-stamp fraction of a second.*/

	uint32_t origTm_s;       /* 32 bits. Originate time-stamp seconds. */
	uint32_t origTm_f;       /* 32 bits. Originate time-stamp fraction of a second. */

	uint32_t rxTm_s;         /* 32 bits. Received time-stamp seconds. */
	uint32_t rxTm_f;         /* 32 bits. Received time-stamp fraction of a second. */

	uint32_t txTm_s;         /* 32 bits and the most important field the client cares about. Transmit time-stamp seconds. */
	uint32_t txTm_f;         /* 32 bits. Transmit time-stamp fraction of a second. */

} ntp_packet;              /* Total: 384 bits or 48 bytes. */

int verbose = 0;
int setclock = 0;

void error( msg )
char* msg;
{
    perror( msg );

    exit( 0 );
}

int main( argc, argv )
int argc;
char **argv;
{
  int sockfd, n; /* Socket file descriptor and the n return result from writing/reading from the socket. */
  struct tm *ntptime;
  
  int portno = 123; /* NTP UDP port number. */

  char* host_name = "ntp.org"; /* NTP server host-name. */

  struct sockaddr_in serv_addr;
  struct hostent *server;
  time_t txTm;
  ntp_packet packet;
  int i,verbose;

  verbose = 0;
  setclock = 0;
  for(i=1; i<argc; i++)
  {
    if (argv[i][0] == '+' || argv[i][0] == '-')
    {  
      if (argv[i][1] == 'v')
        verbose = 1;
      if (argv[i][1] == 's')
      {
      	if (geteuid() == 0)
      	{
	        setclock = 1;
	    }
		else
		{
			fprintf(stderr,"Only the system manager may change the clock.\n");
			exit(-1);
		}
	  }
    }
  }

  memset( &packet, 0, sizeof( ntp_packet ) );
  packet.li_vn_mode = 0x1b;

  sockfd = socket( AF_INET, SOCK_DGRAM, IPPR_UDP );
  if ( sockfd < 0 )
    error( "ERROR opening socket" );

  server = gethostbyname( host_name );
  if ( server == NULL )
    error( "ERROR, no such host" );

  memset( ( char* ) &serv_addr, 0, sizeof( serv_addr ) );
  serv_addr.sin_family = AF_INET;

  memcpy( ( char* )&serv_addr.sin_addr.s_addr, ( char* )server->h_addr, server->h_length );
  serv_addr.sin_port = htons( portno );
  if (verbose)
    fprintf(stderr, "Sending to %s : %d\n", inet_ntoa(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

  if ( connect( sockfd, ( struct sockaddr * ) &serv_addr, sizeof( serv_addr) ) < 0 )
    error( "ERROR connecting" );

  n = send( sockfd, ( char* ) &packet, sizeof( ntp_packet ), 0 );
  if ( n < 0 )
    error( "ERROR send to socket" );

#if 0
  n = write( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );
  if ( n < 0 )
    error( "ERROR writing to socket" );
#endif

  alarm(5);
  
  n = read( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );
  if ( n < 0 )
    error( "ERROR reading from socket" );

  /* These two fields contain the time-stamp seconds as the packet left the NTP server. */
  /* The number of seconds correspond to the seconds passed since 1900.*/
  /* ntohl() converts the bit/byte order from the network's to host's "endianness". */

  packet.txTm_s = ntohl( packet.txTm_s );
  packet.txTm_f = ntohl( packet.txTm_f );

  /* Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
     Subtract 80 years worth of seconds from the seconds since 1900.
     This leaves the seconds since the UniFLEX epoch of 1980.
     (1900)------------------(1980)**************************************(Time Packet Left the Server)
	*/
	
  txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );

  /* adjust to our pre-millenium timebase */
  txTm -= (365L*24L*60L*60L * 28L);
  txTm -= (24*60*60 * 6L);

  /* print in format for 'date -s' */
  ntptime = localtime(&txTm);

  if(setclock)
  {
	stime(&txTm);
  }
  else
  {
   printf("%2.2d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n", 
     ntptime->tm_mon+1,ntptime->tm_mday,ntptime->tm_year + 1900, 
     ntptime->tm_hour,ntptime->tm_min,ntptime->tm_sec);
  	
  }
  
  return 0;
}
