/* TEKTRONIX UNIFLEX HEADERS */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <signal.h>


#include <net/config.h>
#include <net/std.h>
#include <net/btypes.h>
#include <net/select.h>
#include <net/enet.h>

#include <net/inet.h>

/* TEKTRONIX UNIFLEX HEADERS */

#ifndef __clang__

#define B42_COMPATIBLE
#include <net/netdev.h>
#include <net/in.h>
#include <net/socket.h>

#include "fdset.h"

typedef int socklen_t;

#else

#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netdb.h>

extern int open();
extern int wait();
extern int kill();

#define in_sockaddr sockaddr_in

#include "uniflexshim.h"

char buffer[1024*1024/8];
char *physXX(code)
int code;
{
	int fd;
	
	fd = open("/Users/adambillyard/projects/tek4404/development/UniflexEmu/screenshots/wmgr6.bmp", O_RDONLY);
	lseek(fd, 64, 0);
	read(fd, buffer, 1024*1024/8);
	close(fd);
	return buffer;
}

#endif

struct bmpheader {

  short bfType;
  int bfSize;
  short  bfReserved1;
  short  bfReserved2;
  int bfOffBits;

  int biSize;
  int  biWidth;
  int  biHeight;
  short  biPlanes;
  short  biBitCount;
  int biCompression;
  int biSizeImage;
  int  biXPelsPerMeter;
  int  biYPelsPerMeter;
  int biClrUsed;
  int biClrImportant;

  int palette[2];
};

#define REMOTEDUMP_PORT 7389

#define STOPPED 0
#define RUNNING 1
int sock = -1;
int state = STOPPED;

unsigned char bitmap[480 * 640/8];

void
cleanup_and_exit(sig)
int sig;
{
#ifdef DEBUG
  fprintf(stderr,"cleanup remotedumpd on %d\012\n",sig);
#endif
  close(sock);
  state = STOPPED;
}
void
cleanup_session(sig)
int sig;
{
  int rc,pid;
  
  pid = wait(&rc);
#ifdef DEBUG
  fprintf(stderr,"cleanup session: remotedumpd proc(%d)\012\n",pid);
#endif

  signal(SIGDEAD, cleanup_session);
}

void logtime()
{
  time_t timestamp;
  struct tm *ts;

    timestamp = time(NULL);
    ts = localtime(&timestamp);
    fprintf(stderr, "%2.2d-%2.2d-%4.4d %2.2d:%2.2d",
        ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900,
        ts->tm_hour, ts->tm_min);
}

int to_LE16(num)
int num;
{
#ifdef __clang__
    return num;
#else
    return (num>>8) | (num<<8);
#endif
}

int to_LE32(val)
int val;
{
#ifdef __clang__
    return val;
#else
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
#endif
}



int rdp_session(din,dout,from)
int din,dout;
char *from;
{
	struct bmpheader header;
  int i,bytes,x,width,y,height,framenum;
  char *dst,*src;
  int rc;
  
  unsigned int *fb;
  unsigned short *srcbits;
	unsigned short last;
	short run,count, runheader;

  /* write framebuffer RLE to newsock */
  fb = (int *)physXX(1);
  if (!fb)
  {
      return errno;
  }
  
  width = 640;
  height = 480;
  framenum = 0;

  header.bfType = to_LE16(0x4d42);
  header.bfSize = to_LE32(sizeof(struct bmpheader)+(width/8)*height);
  header.bfReserved1 = 0;
  header.bfReserved2 = 0;
  header.bfOffBits = to_LE32(sizeof(struct bmpheader));

  header.biSize = to_LE32(40);
  header.biWidth = to_LE32(width);
  header.biHeight = to_LE32(-height);	/* flipped */
  header.biPlanes = to_LE16(1);
  header.biBitCount = to_LE16(1);
  header.biCompression = 0;
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0;
  header.biYPelsPerMeter = 0;

  header.biClrUsed = to_LE32(2);
  header.biClrImportant = 0;
  /* must use palette */
  header.palette[0] = 0xffffffff;	/* white bg */
  header.palette[1] = 0x000000ff;	/* black fg */
  
  while (1)
  {
    /* TODO:  account for origin */
		/* make a a contiguous buffer */
    src = (char *)fb;
    for(y=0; y<height; y++)
    {
      memcpy(bitmap + y * width/8, src, width/8);
      src += 1024/8;
    }
    
    /* BMP header */
    rc = write(dout, &header, sizeof(header));
    if (rc != sizeof(header))
    {
      break;
    }

		/* compress width x height 1-bit bitmap */
    bytes = 0;
    srcbits = (unsigned short *)bitmap;
    last = srcbits[0];
    run = 0;
    count = 0;
    for (i=0; i<height*width/16; i++)
    {
      if (last == srcbits[i])
      {
        /* flush */
        if (count > 0)
        {
          runheader = count | 0x8000;
          rc = write(dout, &runheader, sizeof(runheader));
          rc = write(dout, srcbits + i - count, count * sizeof(last));
          bytes += 2 + count * sizeof(last);
          count = 0;
        }
      
        run++;
      }
      else
      {
        if (run > 0)
        {
          if (run > 4)
          {
            runheader = run;
            rc = write(dout, &runheader, sizeof(runheader));
            rc = write(dout, &last, sizeof(last));
            bytes += 2 + sizeof(last);
          }
          else
          {
            count += run;
          }
          run = 0;
        }
        
        last = srcbits[i];
        count++;
      }
    }

    framenum++;
    fprintf(stderr, "send frame %d in %d bytes\n", framenum,bytes);
    sleep(1);
  }

  physXX(-1);

  return 0;
}
  
int
main(argc,argv)
int argc;
char **argv;
{
  int newsock,rc;
  struct in_sockaddr serv_addr;
  struct in_sockaddr cli_addr;
  socklen_t cli_addr_len;
  int reuse = 1;
  int off = 1;

	/* listen for connections too? */
	if (argc > 1 && !strcmp(argv[1],"-s"))
	{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      fprintf(stderr, "socket: %s\n",strerror(errno));
      return 1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(REMOTEDUMP_PORT);
    rc = bind(sock, (struct sockaddr *) & serv_addr, sizeof serv_addr);
    if (rc < 0) {
      fprintf(stderr, "bind: %s\n",strerror(errno));
      close(sock);
      return errno;
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    signal(SIGTERM, cleanup_and_exit);
    signal(SIGDEAD, cleanup_session);

    rc = listen(sock, 5);
    if (rc < 0) {
      fprintf(stderr, "listen: %s\n",strerror(errno));
      close(sock);
      return errno;
    }

		/* wait to serve */
    state = RUNNING;
    while (state == RUNNING)
    {
      newsock = 0;
      while (state == RUNNING && newsock <= 0)
      {
        newsock = accept(sock, (struct sockaddr *) & cli_addr, &cli_addr_len);
        if (newsock < 0){
          if (errno == EINTR) continue;
          if (errno == ETIMEDOUT) continue;
          if (errno == ENETUNREACH) continue;
          if (errno == EHOSTUNREACH) continue;
          if (errno == ECONNRESET) continue;
          if (errno == ENETDOWN) continue;
          if (errno == ENOPROTOOPT) continue;

          fprintf(stderr, "error %d (%s) in accept\n", errno, strerror(errno));
          close(sock);
          return errno;
        }
      }
      
      if (state == STOPPED) break;

  #ifndef __clang__
      /* is turning off Nagle supported? */
  #else
      setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
  #endif

      /* Uniflex accept() doesn't fill this in.. */
      cli_addr_len = sizeof(cli_addr);
      rc = getpeername(newsock, (struct sockaddr *) &cli_addr, &cli_addr_len);

      logtime();
      fprintf(stderr, ": connect from %s\012\n", inet_ntoa(cli_addr.sin_addr.s_addr));

      rc = rdp_session(newsock, newsock, inet_ntoa(cli_addr.sin_addr.s_addr));
	    close(newsock);

      logtime();
      fprintf(stderr, ": disconnect from %s\012\n", inet_ntoa(cli_addr.sin_addr.s_addr));

    }

	  close(sock);
  }
  else
  {
  		/* use stdin/stdout */
      rdp_session(0, 1, "local");
  }
  
  return 0;
}
