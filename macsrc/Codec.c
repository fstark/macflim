#include "Codec.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------invert


#include "Util.h"

#include <stdio.h>

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

static void Null_ref( char *source, struct CodecControlBlock *ccb )
{
}

/*
static void UnpackZ16_64( char *dest, char *source, int rowbytes, short input_width )
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
*/

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

static void UnpackZ32_ref( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long **offsets = (unsigned long **)ccb->offsets32;
	register unsigned long *s = (unsigned long *)source;
	register long header;
	register long rowlongs = ccb->output_width32;

	while (header=*s++)
	{
		register unsigned int offset;
		register unsigned int copy;
		register unsigned long *d;
		
			//	This is the "source" offset on the original screen
		offset = (header&0xffff)/4-1;
	
			//	We convert it to the "destination" offset
		d = offsets[offset];

			//	The number of bytes to copy		
		copy = (header>>16)+1;

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

static void UnpackZ32_64( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->offsets32[0];
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

static void UnpackZ32_80( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->offsets32[0];
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

static void UnpackZ32_asm( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->offsets32[0];
	register unsigned short rowbytes = ccb->output_width8;

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
		add.l d0,a2					;	".l" so we can add offsets > 32767

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
//	Assembly any => any
//	-------------------------------------------------------------------

//	Mapping table from possible pixels locations
//	To offsets
//	There are less than 1024 (32768/32) pixels locations,
//	but they can map to offsets larget than 65536 (in case of large screens)

long *gOffsets;

Boolean CreateOffsetTable(
	unsigned char ***offsets,
	unsigned char *base_addr,
	unsigned short input_width,
	unsigned short input_height,
	unsigned short output_width
	)
	{
	unsigned short output_row_bytes = output_width/8;
	long input_width_long = input_width/8/sizeof(long);
		//	How many offset entries do we need?
		//	(number of source pixels/32)
	long total_offset_count = input_width_long*(long)input_height;
	unsigned char **p;
	int x,y;
	int i;

	if (*offsets)
		DisposPtr( *offsets );

	*offsets = (unsigned char **)NewPtr( total_offset_count*sizeof(long) );
	if (!*offsets)
		return FALSE;

	for (i=0;i!=total_offset_count;i++)
		(*offsets)[i] = base_addr;

	p = *offsets;
	for (y=0;y!=input_height;y++)
	{
		unsigned char *line_start = base_addr+y*(long)output_row_bytes;

		for (x=0;x!=input_width_long;x++)
		{
			*p++ = line_start;
			line_start += 4;
		}
	}

	return TRUE;
}

static void UnpackZ32_any_any_asm( char *source, struct CodecControlBlock *ccb )
{
	register unsigned short rowbytes = ccb->output_width8;
	register unsigned long *offsets = (unsigned long *)ccb->offsets32;

	asm
	{
			;	Save registers
		movem.l D5-D7/A1-A4,-(A7)

			;	Get parameters
		movea.l offsets,a4			;	a4 == offsets table
		movea.l source,a3			;	a3 == source data
		move.w 	rowbytes,d1			;	d1 == rowbytes

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		subq.w #4,d7				;	offsets are +4
		add d7,a2					;	entry in the offset table
		move.l (a2), a2				;	offset for the output

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	d1,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A1-A4
	}
}

static void UnpackZ32_any_any_xxx( char *dest, char *source, int rowbytes, short input_width )
{
	register unsigned long *base = (unsigned long *)dest;
	register unsigned long *s = (unsigned long *)source;
	register long header;
	register long rowlongs = rowbytes/4;

	register short flim_width_long = input_width/32;

	if (!gOffsets)
	{
		return ;
//		CreateOffsetTable( &gOffsets, 0, input_width, 430, rowbytes*8 );
	}

	while (header=*s++)
	{
		register unsigned int offset;
		register unsigned long offset_output;
		register unsigned int copy;
		register unsigned long *d;
		
		offset = (header&0xffff)/4-1;
		offset_output = gOffsets[offset];
		offset_output = offset_output/4;

//		offset = (offset/flim_width_long)*rowlongs+(offset%flim_width_long);
	
		copy = (header>>16)+1;
		d = base+offset_output;

		while (copy--)
		{
			*d = *s++;
			d += rowlongs;
		}
	}
}

//	-------------------------------------------------------------------
//	Codec 0x03 : invert
//	Inverts the whole screen
//	On-disk format:
//		(no data)
//	-------------------------------------------------------------------

static void Invert_ref( char *source, struct CodecControlBlock *ccb )
{
	register int stride32 = ccb->output_width32-ccb->source_width32;
	register unsigned long *p = (unsigned long *)ccb->offsets32[0];
	register int source_width32_1 = ccb->source_width32 + 1;

	register int y = ccb->source_height;
	while (--y)
	{
		register int x = source_width32_1;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += stride32;
	}
}

//	-------------------------------------------------------------------
//	Assembly 512 pixels => 512
//	-------------------------------------------------------------------

static void Invert_64( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *p = (unsigned long *)ccb->offsets32[0];

	register int y = ccb->source_height;
	while (--y)
	{
		register int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
	}
}

static void Invert_any_any( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *p = (unsigned long *)ccb->offsets32[0];
	register int input_width_long = (ccb->source_width32)+1;
	register int row_bytes_offset_long = ccb->output_width32 - ccb->source_width32;

	register int y = ccb->source_height+1;

	while (--y)
	{
		register int x = input_width_long;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += row_bytes_offset_long;
	}
}

//	-------------------------------------------------------------------
//	Codec 0x04 : copy lines (reference implementation)
//	Copies a serie of horizontal lines
//	On-disk data:
//		2 bytes     : count of bytes to copy
//		2 bytes     : offset to copy to
//		count bytes : data to copy
//	-------------------------------------------------------------------

static void CopyLines_ref( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long **offsets = (unsigned long **)ccb->offsets32;
	register unsigned short input_rowbytes = ccb->source_width8;
	register unsigned short output_rowbytes = ccb->output_width8;
	register unsigned char *dest;

	unsigned short len = ((unsigned short*)source)[0];
	unsigned short offset = ((unsigned short*)source)[1];

	source += 4;

	dest = (unsigned char *)offsets[offset/4];

	while (len)
	{
		BlockMove( source, dest, input_rowbytes );
		source += input_rowbytes;
		dest += output_rowbytes;

		len -= input_rowbytes;
	}
}

//	-------------------------------------------------------------------
//	512 = > 512
//	-------------------------------------------------------------------

static void CopyLines_64( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->offsets32[0];

	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	BlockMove( source+4, dest+offset, len );
}

static void CopyLines_any_any( char *source, struct CodecControlBlock *ccb )
{
	register unsigned char *dest = ccb->offsets32[0];
	register unsigned short rowbytes = ccb->output_width8;
	int width = ccb->source_width8;

	unsigned short len = ((unsigned short*)source)[0];
	unsigned long offset = ((unsigned short*)source)[1];	//	Long in case where the fb size is > 65536

	int x = 0;

	source += 4;

	dest = ccb->offsets32[offset/4];

	while (len)
	{
		BlockMove( source, dest, width );
		source += width;
		dest += rowbytes;
		len -= width;
	}
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
	CodecAdd( Null_ref, 				kNull,		-1, -1, 0 );

		//	Z16 is not supported anymore
	CodecAdd( Null_ref, 				kZ16,		-1, -1, 0 );

	CodecAdd( UnpackZ32_ref, 			kZ32,		-1, -1, 0 );
	CodecAdd( UnpackZ32_64, 			kZ32,		512, 64, 1 );
	CodecAdd( UnpackZ32_80, 			kZ32,		512, 80, 1 );
	CodecAdd( UnpackZ32_asm, 			kZ32,		512, -1, 1 );

	CodecAdd( UnpackZ32_any_any_asm,	kZ32,		-1, -1, 1 );
	
	CodecAdd( Invert_ref,				kInvert,	-1, -1, 0 );
	CodecAdd( Invert_64, 				kInvert,	512, 64, 1 );

	CodecAdd( Invert_any_any, 			kInvert,	-1, -1,  1 );

	CodecAdd( CopyLines_ref,			kCopy,		-1, -1, 0 );
	CodecAdd( CopyLines_64, 			kCopy,		512, 64, 1 );

	CodecAdd( CopyLines_any_any, 		kCopy,		-1, -1,  1 );
}

//	-------------------------------------------------------------------

DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type )
{
	int i;
	RegistryEntry *r = gRegistry;

//	CreateOffsetTable( &gOffsets, 0, 608, 430, outputWidth );

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
