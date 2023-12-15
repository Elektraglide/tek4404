/*
 * Pseudo-Terminal Control
 *
 */

/* mode flags */

#define PTY_PACKET_MODE    0x01
#define PTY_REMOTE_MODE    0x02
#define PTY_READ_WAIT      0x04 /* Block on non-satisfied reads */
#define PTY_WRITE_WAIT     0x08 /* Don't block on writes */
#define PTY_HANDSHAKE_MODE 0x10 /* Remote writes not satisfied until consumed */
#define PTY_SLAVE_HOLD     0x80

/* responses */

#define PTY_EOF            0x100 /* No more slave connections */
#define PTY_OUTPUT_QUEUED  0x200 /* Slave has some output queued */
#define PTY_INPUT_QUEUED   0x400 /* Slave has some input queued */

/* control commands */

#define PTY_INQUIRY       0
#define PTY_SET_MODE      1
#define PTY_FLUSH_READ    3
#define PTY_FLUSH_WRITE   4
#define PTY_STOP_OUTPUT   5
#define PTY_START_OUTPUT  6
