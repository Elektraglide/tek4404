#include <stdio.h>
#include <graphics.h>

int main(argc, argv)
int argc;
char **argv;
{
struct POINT p;
	
    InitGraphics(FALSE);
    p.x = p.y = 0;
    SetViewport(&p);
    EventDisable();
    SetKBCode(1);

  return 0;      	
}
