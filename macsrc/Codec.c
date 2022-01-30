#include "Codec.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Util.h"

//	-------------------------------------------------------------------
//	Display functions
//	All codec display functions are implemented several times:
//	- A slow, C-based, reference implementation that works on
//	  screen larger than 512x342 (usedful for development and testing)
//	- A faster, 68x assembly version, suitable for real-world macs
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Codec 0x00 : null
//	On-disk format: empty
//	-------------------------------------------------------------------

static void Null_ref( char *dest, char *source, int rowbytes )
{
}

//	-------------------------------------------------------------------
//	Codec 0x01 : z16
//	  Copies series of vertical variable height 16 pixels information
//	On-disk format:
//	  list of chunks:
//	  2 byte header : (0x0000 to end)
//		9 bits offset in bytes from precedent chunk (starts top-left)
//		7 bits count of data
//	  count words   : data to be copied at vertical 16 pixels line
//	-------------------------------------------------------------------

static void UnpackZ16_ref( char *dest, char *source, int rowbytes )
{
	register unsigned char *d = (unsigned char *)dest;
	register unsigned char *dmax = d+64;

	register unsigned short *s = (unsigned short *)source;
	register unsigned short header;
	register unsigned short rowshorts = rowbytes/2;

	while (header=*s++)
	{
		register unsigned short offset;
		register int count = 0;
		register unsigned short *adrs;

		offset = header>>7;
		
		d += offset;

			//	Adjust for screen larger than 512 pixels
		while (d>dmax)
		{
			d += rowbytes-64;
			dmax += rowbytes;
		}
		
		count = header&0x7f;

		adrs = (unsigned short*)d;

		while (count--)
		{
			*adrs = *s++;
			adrs += rowshorts;
		}
	}
}

//	-------------------------------------------------------------------
//	Assembly version for 512 pixels source, and 512 pixels destinations
//	-------------------------------------------------------------------

static void UnpackZ16_64( char *dest, char *source, int rowbytes )
{

	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screen address
		movea.l source,a3			;	a3 == source data

@chunk:
		move.w	(a3)+,d7			;	header
		beq.s	@exit				;	0x0000 => end of frame
									
		move.w	d7,d6
		lsr.w	#7,d6				;	High 9 bits of header is offset
		add		d6,a4				;	new screen address

		movea.l	a4,a2				

		and.w	#0x7f,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#1,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#2,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#3,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#4,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#5,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#6,d7
		beq 	@chunk

		sub 	#7,d7
loop:
        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
@endloop:
		dbra d7,@loop
		bra @chunk

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Codec 0x02 : z32
//	  Copies series of vertical variable height 32 pixels information,
//	Format:
//	  A series of chunks
//	  4 byte header : (0x00000000 to end)
//		2 bytes     : count of data to copy, minus 1
//		2 bytes     : offset from the top of the screen, plus 4
//	  count quads   : data to be copied at vertical 32 pixels line
//	-------------------------------------------------------------------

static void UnpackZ32_ref( char *dest, char *source, int rowbytes )
{
	register unsigned long *base = (unsigned long *)dest;
	register unsigned long *s = (unsigned long *)source;
	register long header;
	register long rowlongs = rowbytes/4;

	while (header=*s++)
	{
		register unsigned int offset;
		register unsigned int copy;
		register unsigned long *d;
		
		offset = (header&0xffff)/4-1;
		
#ifdef REFERENCE_ASSERTS
		if (offset>=21888/4)
			assert( false, "Bad Z32 Offset" );
#endif
		
		offset = (offset/16)*rowlongs+(offset%16);
		
		copy = (header>>16)+1;
#ifdef REFERENCE_ASSERTS
		assert( copy>0, "Bad Z32 count (neg)" );
		assert( copy<=342, "Bad Z32 count" );
#endif

#ifdef REFERENCE_ASSERTS
		if (offset<0 || offset>=7524) // Lisa Hack
			printf( "BAD OFFSET %d HEADER %lx\n", (int)offset, (int)header );
#endif

		d = base+offset;

		while (copy--)
		{
			*d = *s++;
			d += rowlongs;
		}
	}
}

//	-------------------------------------------------------------------
//	Assembly 512 pixels => 512
//	-------------------------------------------------------------------

static void UnpackZ32_64( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		subq.l #4,a4				;	minus4, as all offets are +4
		movea.l source,a3			;	a3 == source data

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		add.w	d7,a2				;	Low end of d7 is offset

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	#64,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Assembly 512 => 640 (Macintosh Portable)
//	-------------------------------------------------------------------

static void UnpackZ32_80( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		movea.l source,a3			;	a3 == source data

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		subq.w #4,d7				;	offsets are +4
		move.w d7,d0
		and.w #0x3f,d0
		add.w d0,a2
		lsr.w #6,d7

				;	d0 = d7*80
//		move.w #80,d0				;	Explicit multiplication is 3% slower
//		mulu d7,d0

		move.w d7,d0				;	d0 = d7
		lsl.w #2,d0					;	d0 = d7 * 4
		add.w d7,d0					;	d0 = d7 * 5
		lsl.w #4,d0					;	d0 = d7 * 80

		add.w d0,a2

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	#80,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Assembky 512 => any
//	-------------------------------------------------------------------

void UnpackZ32_asm( char *dest, char *source, int rowbytes );
void UnpackZ32_asm( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		movea.l source,a3			;	a3 == source data
		move.w rowbytes,d1			;	d1 == rowbytes

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		subq.w #4,d7				;	offsets are +4
		move.w d7,d0
		and.w #0x3f,d0
		add.w d0,a2
		lsr.w #6,d7

				;	d0 = d7*rowbytes
		move.w d7,d0
		mulu d1,d0
		add.l d0,a2					;	.l so we can add offsets > 32767

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	d1,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Codec 0x03 : invert
//	Inverts the whole screen
//	On-disk format:
//		(no data)
//	-------------------------------------------------------------------

static void Invert_ref( char *dest, char *source, int rowbytes )
{
	int stride = (rowbytes-64)/4;
	register long *p = (long *)dest;

	register int y = 343;
	while (--y)
	{
		register int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += stride;
	}
}

//	-------------------------------------------------------------------
//	Assembly 512 pixels => 512
//	-------------------------------------------------------------------

static void Invert_64( char *dest, char *source, int rowbytes )
{
	register long *p = (long *)dest;

	register int y = 343;
	while (--y)
	{
		register int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
	}
}

//	-------------------------------------------------------------------
//	Codec 0x04 : copy lines (reference implementation)
//	Copies a serie of horizontal lines
//	On-disk data:
//		2 bytes     : count of bytes to copy
//		2 bytes     : offset to copy to (multiple of 64)
//		count bytes : data to copy (multiple of 64)
//	-------------------------------------------------------------------

static void CopyLines_ref( char *dest, char *source, int rowbytes )
{
	unsigned short len = ((unsigned short*)source)[0];
	unsigned long offset = ((unsigned short*)source)[1];	//	Long in case where the fb size is > 65536

	if (offset<0 || offset>=21888)
		ExitToShell();
	if (len<0 || len>21888)
		ExitToShell();
	if ((len%64)!=0)
		ExitToShell();
	if (offset+len>21888)
		ExitToShell();

	source += 4;

//	offset = (offset/16)*(rowbytes/4)+(offset%16);
	offset = (offset/16)*(rowbytes/2)+(offset%16);	//	Fix for Lisa, rowbytes is not mult of 4
	offset>>=1;										//	Fix for Lisa, we are back to normal

	dest += offset;
	while (len)
	{
		BlockMove( source, dest, 64 );
		source += 64;
		dest += rowbytes;
		len -= 64;
	}
}

//	-------------------------------------------------------------------
//	512 = > 512
//	-------------------------------------------------------------------

static void CopyLines_64( char *dest, char *source, int rowbytes )
{
	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	BlockMove( source+4, dest+offset, len );
}

//	-------------------------------------------------------------------
//	Registry functions
//	The registry is responsible to select the appropriate functions
//	to play flims
//	-------------------------------------------------------------------

typedef struct
{
	DisplayProc displayProc;
	int codec;
	int type;			//	0 = ref, 1 = asm
	int inputWidth;
	int outputWidth;
}	RegistryEntry;

#define MAX_REGISTRY_ENTRIES 32

static int registryCount = 0;
static RegistryEntry gRegistry[MAX_REGISTRY_ENTRIES];

//	-------------------------------------------------------------------
//	Adds an entry to the registry
//	-------------------------------------------------------------------

static void CodecAdd( DisplayProc dp, int codec, int inputWidth, int outputWidth, int type )
{
	RegistryEntry *r;

	assert( registryCount<MAX_REGISTRY_ENTRIES, "RegistryFull" );
	r = gRegistry+registryCount;
	
	r->displayProc = dp;
	r->codec = codec;
	r->type = type;
	r->inputWidth = inputWidth;
	r->outputWidth = outputWidth;
	
	registryCount++;
}

//	-------------------------------------------------------------------

void CodecInit( void )
{
	CodecAdd( Null_ref, 		0x00, 512, -1, 0 );

	CodecAdd( UnpackZ16_ref, 0x01, 512, -1, 0 );
	CodecAdd( UnpackZ16_64, 	0x01, 512, 64, 1 );

	CodecAdd( UnpackZ32_ref, 0x02, 512, -1, 0 );
	CodecAdd( UnpackZ32_64, 	0x02, 512, 64, 1 );
	CodecAdd( UnpackZ32_80, 	0x02, 512, 80, 1 );
	CodecAdd( UnpackZ32_asm, 0x02, 512, -1, 1 );

	CodecAdd( Invert_ref,	0x03, 512, -1, 0 );
	CodecAdd( Invert_64, 	0x03, 512, 64, 1 );

	CodecAdd( CopyLines_ref,	0x04, 512, -1, 0 );
	CodecAdd( CopyLines_64, 	0x04, 512, 64, 1 );
}

//	-------------------------------------------------------------------

DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type )
{
	int i;
	RegistryEntry *r = gRegistry;
	for (i=0;i!=registryCount;i++,r++)
		if (r->codec==codec && r->type==type)
			if ((r->inputWidth ==inputWidth  || r->inputWidth ==-1)
			 && (r->outputWidth==outputWidth || r->outputWidth==-1))
				return r->displayProc;

		//	Fallback to reference codecs
	if (type>0)
		return CodecGetProc( codec, inputWidth, outputWidth, 0 );

	return NULL;
}
