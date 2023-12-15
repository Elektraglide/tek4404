/*
 * Tektronix 4400 Communications Port Interface definitions
 *
 * file: /lib/include/sys/comm.h
 *
 * Control interface -- Values used in ttyset/ttyget call
 *
 *
 *     The following structure (6 byte structure) describes the command and
 * parameters values used in "ttyset/ttyget" interface calls to the
 * communications port. The "c_comm" field is set to one of the following
 * "Control Commands" for a "ttyset" call. A "ttyget" call is passed this
 * buffer,  and returns that buffer with the current state of communications
 * driver described with in these fields. The number of characters currently
 * waiting within the communications driver buffer, is described within the
 * "c_com" and "c_value" fields, with the MSB (in c_com) and the LSB
 * (in c_value).
 *
 */

struct commbuf
  {
    unsigned char c_com;     /* Command field   (6 byte buffer ! ) */
    unsigned char c_value;   /* Additional parameters */
    unsigned char c_parity;  /* Parity selection */
    unsigned char c_flag;    /* Flow control "flag" method */
    unsigned char c_ospeed;  /* Output baud rate */
    unsigned char c_ispeed;  /* Input baud rate */
  };

#define COMM_DEVICE "/dev/comm"

/*
 * Communications pseudo-commands
 */

#define RESET_COMM   1
#define SETUP_COMM   2
#define EXCL_COMM    3
#define BREAK_COMM   4
#define NOBLOCK_COMM 5
#define BLOCK_COMM   6
#define DTRLOW_COMM  7
#define DTRHIGH_COMM 8
#define RTSLOW_COMM  9
#define RTSHIGH_COMM 10

/*
 * CTS mode
 */

#define CTSNONE_COMM	0x00
#define CTSTRUE_COMM	0x40
#define CTSDEFAULT	CTSTRUE_COMM
			  /* True, Wait for CTS (True) before transmitting */



/*
 * Parity mode
 */

#define LOW_PARITY  0
#define HIGH_PARITY 1
#define EVEN_PARITY 2
#define ODD_PARITY  3
#define NO_PARITY   4
#define DFLT_PARITY LOW_PARITY

#define ONE_STOP_BIT   0x00
#define TWO_STOP_BITS  0x80
#define DFLT_STOP_BITS ONE_STOP_BIT

#define PARITY_MASK 0x0F
#define STOP_MASK   0x80
#define CTS_MASK    0x40

/*
 * Flow control flag mode
 */

#define NO_FLAG     0
#define INPUT_FLAG  1
#define OUTPUT_FLAG 2
#define TANDEM_FLAG 3
#define DTR_FLAG    4
#define DFLT_FLAG   TANDEM_FLAG

/*
 * Baud rate selectors
 */

#define EXTERNAL  0
#define C50       1
#define C75       2
#define C110      3
#define C134      4
#define C150      5
#define C300      6
#define C600      7
#define C1200     8
#define C1800     9
#define C2400     10
#define C4800     11
#define C9600     12
#define C19200    13
#define C38400    14
#define DFLT_BAUD_RATE C9600
