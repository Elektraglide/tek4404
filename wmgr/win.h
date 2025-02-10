
#define WINTITLEBAR 16
#define WINBORDER 16
#define CLOSEBOX 8


typedef struct _win
{
    /* term emulator; NB always first so we can cast it to a Window */
    VTemu vt;

    struct _win* next;
    char title[64];
    struct RECT oldrect;
    struct RECT windowrect;
    struct RECT contentrect;
    int master, slave;
    int pid;
    int dirty;

} Window;
