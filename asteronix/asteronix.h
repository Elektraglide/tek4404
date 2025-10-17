
extern struct FORM *screen;
extern struct BBCOM bb;
extern short pindex;
extern struct POINT page;

#define INT2FIX(A)  ((A)<<8)
#define FIX2INT(A)  ((A)>>8)

typedef long fixed;

typedef enum {
	DEAD = 0,
	DYING3,
	DYING2,
	DYING1 = 5,
	ALIVE
} sprstate;

typedef struct
{
	fixed x, y;
	fixed dx, dy;

	short oldx[2], oldy[2];
	sprstate state;
} sprite;

typedef struct
{
	sprite spr;
	short lifetime;
} bullet;


/* meteor_dat.c */
extern struct FORM big, big_m;
extern struct FORM medium, medium_m;
extern struct FORM* bigshift[16];
extern struct FORM* big_mshift[16];
extern struct FORM* mediumshift[16];
extern struct FORM* medium_mshift[16];
extern void makemeteorforms();
extern void makeshifted();
extern void blitfast();
extern void restoreunder();
extern void blitclear64();
extern void bigblitflash();
extern void bigblitmasked();
extern void bigblitfast();
extern void bigdrawfast();
extern void mediumdrawfast();
extern void drawsprite();

