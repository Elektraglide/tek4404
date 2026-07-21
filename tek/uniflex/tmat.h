/* 'tmat' struct per task derived from Ghidra */

struct tmat
{
	ushort unknown1;
	ptr32 memqueue;		/* 0x02 */
	ptr32 unknown2;
	ptr32 pooledpages;	/* 0x0a */
	ptr32 privpages;	/* 0x0e */
	ushort tasksize; 	/* 0x12 */
	ushort taskordinal;	/* 0x14 */	
	ptr32 unknown3;		/* 0x16 */
};
