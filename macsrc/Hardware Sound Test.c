#define vBase 0xEFE1FE
#define vBufA (512*15)
#define vBufB 0
#define vSndEnb 7

#include <stdio.h>

main()
{
	long i = 0;
//	char *soundBuffer = *(char **)0x266;
	long soundBufferAdrs = *(long *)0x266;

	soundBufferAdrs &= 0x00ffffff;

	printf( "%lx\n", soundBufferAdrs );

	asm
	{
		bclr.b #vSndEnb, vBase+vBufB;
	}

	getchar();

	asm
	{
		bset.b #vSndEnb, vBase+vBufB;
	}
}
