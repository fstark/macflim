#include "Codec.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------invert


#include "Util.h"

#include <stdio.h>

//	-------------------------------------------------------------------
//	To display when the video is different form the actual screen,
//	we use an offset table, that is exactly like a video buffer that
//	contains a pointer to the corresponding physical screen
//	-------------------------------------------------------------------
//	There are less than 2047 (65532/32) pixels locations
//	but they can map to offsets larget than 65536 (in case of large screens)
//	-------------------------------------------------------------------

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

//	-------------------------------------------------------------------
//	Codec 0x02 : z16
//	obsolete
//	-------------------------------------------------------------------

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

static void UnpackZ32_same_ref( char *source, struct CodecControlBlock *ccb )
{
	register unsigned char *baseAddr = ccb->baseAddr;
	register unsigned long *s = (unsigned long *)source;
	register long header;
	register long rowlongs = ccb->output_width32;

	while (header=*s++)
	{
		register unsigned int offset;
		register unsigned int copy;
		register unsigned long *d;
		
			//	This is the "source" offset on the original screen
		offset = header&0xffff-4;
	
			//	We convert it to the "destination" offset
		d = (unsigned long *)(baseAddr+offset);

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

static void UnpackZ32_all_ref( char *source, struct CodecControlBlock *ccb )
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

static void UnpackZ32_same( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->baseAddr;
	register unsigned short rowbytes = ccb->output_width8;

	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		subq.l 	#4,a4				;	minus4, as all offets are +4
		movea.l source,a3			;	a3 == source data
		move    rowbytes,d5			;	d5 == rowbytes

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		add.w	d7,a2				;	Low end of d7 is offset

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	d5,a2				;	Add rowbytes
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------

static void UnpackZ32_all( char *source, struct CodecControlBlock *ccb )
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

//	-------------------------------------------------------------------
//	Codec 0x03 : invert
//	Inverts the whole screen
//	On-disk format:
//		(no data)
//	-------------------------------------------------------------------

static void Invert_same_ref( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *p = (unsigned long *)ccb->baseAddr;
	register int input_width_long = (ccb->source_width32)+1;

	register int y = ccb->source_height;
	while (--y)
	{
		register int x = input_width_long;
		while (--x)
			*p++ ^= 0xffffffffL;
	}
}

//	-------------------------------------------------------------------

static void Invert_all_ref( char *source, struct CodecControlBlock *ccb )
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

static void CopyLines_same_ref( char *source, struct CodecControlBlock *ccb )
{
	register unsigned long *dest = (unsigned long *)ccb->baseAddr;

	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	BlockMove( source+4, dest+offset, len );
}

//	-------------------------------------------------------------------

static void CopyLines_all_ref( char *source, struct CodecControlBlock *ccb )
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

DisplayProc sProcs[2][2][kCodecCount] = {
	{		//	non-ref, all sizes
		{ Null_ref, Null_ref, UnpackZ32_all, Invert_all_ref, CopyLines_all_ref },
			//	non-ref, same size
		{ Null_ref, Null_ref, UnpackZ32_same, Invert_same_ref, CopyLines_same_ref },
	},
	{
			//	ref, all sizes
		{ Null_ref, Null_ref, UnpackZ32_all_ref, Invert_all_ref, CopyLines_all_ref },
			//	ref, same size
		{ Null_ref, Null_ref, UnpackZ32_same_ref, Invert_same_ref, CopyLines_same_ref },
	},
};

//	-------------------------------------------------------------------

DisplayProc CodecGetProc( eCodec codec, Boolean same, Boolean ref )
{
	return sProcs[ref][same][codec];
}
