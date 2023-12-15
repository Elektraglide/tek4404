/*
    Definitions for virtual-memory page_monitoring
*/

/*
    Information returned by the return read/write information
    functions.
    The data is stored right-justified in the "data" longword.
*/

struct fault_information {
    char *fault_address;
    short data_size;
    long data;
};

/*  Function codes  */

#define SET_READ_MONITOR 0
#define SET_WRITE_MONITOR 1
#define CLEAR_READ_MONITOR 2
#define CLEAR_WRITE_MONITOR 3
#define RETURN_READ_INFORMATION 4
#define RETURN_WRITE_INFORMATION 5

#define ROUND_INWARD 32 /* added to set/clear functions, if desired */

