/* 'tmat' struct per task derived from Ghidra */

struct tmat
{
	ushort unknown1;
	ulong memqueue;		/* 0x02 */
	ulong unknown2;
	ulong pooledpages;	/* 0x0a */
	ulong privpages;	/* 0x0e */
	ushort tasksize; 	/* 0x12 */
	ushort taskordinal;	/* 0x14 */	
	ulong unknown3;		/* 0x16 */
};
