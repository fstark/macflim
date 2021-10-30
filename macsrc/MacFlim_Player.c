//	-------------------------------------------------------------------
//	MacFlim 2 standalone player
//	-------------------------------------------------------------------
//	Features:
//		Selects a flim file
//		Plays the flim file in full screen with sound
//		Handle type/creators
//		Looping
//		next, previous, restart, abort, pause
//	Todo:
//		Set type creator at open
//		(none left)
//	Nice to have:
//		Single step
//		Error handling
//		Autoplay (plays data fork)
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Those #defines help debugging the player
//	-------------------------------------------------------------------

#define noVERBOSE			//	Enables verbose logs
#define noTESTING			//	Just execute the display code in synchronous mode for decoding debugging
#define noREFERENCE			//	Forces use of reference codec implementation (with support for non 512x342)
#define noREFERENCE_ASSERTS	//	Adds debug asserts in reference codecs
#define noDISPLAY_STATS		//	Displays playback stats at the end

//	-------------------------------------------------------------------
//	The block size must be kept in sync with the one from flimmaker
//	At some point, a new file format should be done with variable block size
//	-------------------------------------------------------------------

#define BUFSIZE 302000L


#include <stdio.h>
#include <Types.h>
#include <QuickDraw.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Memory.h>
#include <Retrace.h>
#include <Sound.h>
#include <string.h>

//	-------------------------------------------------------------------
//	KEYBOARD HANDLING ROUTINES
//	-------------------------------------------------------------------


//	-------------------------------------------------------------------
//	Helper to test a key
//	-------------------------------------------------------------------

static Boolean TestKey( unsigned char *keys, char k )
{
	return !!((keys[k>>3]>>(k&7))&1);
}
	
//	-------------------------------------------------------------------
//	Checks for escape (and others)
//	-------------------------------------------------------------------

static Boolean CheckEscape( unsigned char *keys )
{
	//	We start at true, so if the mouse or the return key
	//	is pressed at the start, we don't exit (we require
	//	at least one 'escaped = FALSE' before escaping)
	
	static Boolean escaped = TRUE;
	Boolean old_state = escaped;

	escaped = TestKey( keys, 0x35 )	//	ESC
		|| TestKey( keys, 0x32 )	//	`
		|| TestKey( keys, 0x33 )	//	Backspace
		|| TestKey( keys, 0x77 )	//	End
		|| TestKey( keys, 0x0c )	//	'q'
		|| Button();

	if (!old_state && escaped)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for TAB (and others)
//	-------------------------------------------------------------------

static Boolean CheckSkip( unsigned char *keys )
{
	static Boolean nexted = TRUE; //	See comment for ESCAPE
	Boolean old_state = nexted;

	nexted = TestKey( keys, 0x30 )		//	Tab
		|| TestKey( keys, 0x24 )		//	Return
		|| TestKey( keys, 0x05 )		//	Right Arrow (plus)
		|| TestKey( keys, 0x0D );		//	Left Arrow (plus)

	if (!old_state && nexted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for restart
//	-------------------------------------------------------------------

static Boolean CheckRestart( unsigned char *keys )
{
	static Boolean restarted = TRUE; //	See comment for ESCAPE
	Boolean old_state = restarted;

	restarted = TestKey( keys, 0x0f );		//	'r'

	if (!old_state && restarted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for help
//	-------------------------------------------------------------------

static Boolean CheckHelp( unsigned char *keys )
{
	static Boolean helped = TRUE; //	See comment for ESCAPE
	Boolean old_state = helped;

	helped = TestKey( keys, 0x04 );		//	'h'

	if (!old_state && helped)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for SPACE
//	-------------------------------------------------------------------

Boolean CheckPause( unsigned char *keys )
{
	static Boolean paused = TRUE;	//	See comment for ESCAPE      
	Boolean old_state = paused;
	
	paused = TestKey( keys, 0x31 );	//	Space bar

	if (!old_state && paused)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for mute
//	-------------------------------------------------------------------

Boolean CheckMute( unsigned char *keys )
{
#define kMute 	0x1	//	's' key

	static Boolean muted = TRUE;	//	See comment for ESCAPE      
	Boolean old_state = muted;
	
	muted = TestKey( keys, kMute );

	if (!old_state && muted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Check keys status
//	Sets the sEscape, sSkip, sPause and sMuted variable
//	-------------------------------------------------------------------

Boolean sEscape;
Boolean sSkip;
Boolean sPrevious;
Boolean sRestart;
Boolean sPause;
Boolean sMuted = FALSE;
Boolean sHelp;

void CheckKeys()
{
	unsigned char theKeys[16];

	GetKeys( theKeys );
	sEscape = CheckEscape( theKeys );
	if (CheckSkip( theKeys ))
	{
		if (TestKey( theKeys, 56 ))	//	Shift
		{
			sSkip = FALSE;
			sPrevious = TRUE;
		}
		else
		{
			sSkip = TRUE;
			sPrevious = FALSE;
		}
	}
	else
		sSkip = sPrevious = FALSE;
	sRestart = CheckRestart( theKeys );
	sPause = CheckPause( theKeys );
	sHelp = CheckHelp( theKeys );
	if (CheckMute( theKeys ))
		sMuted = !sMuted;
}

//	-------------------------------------------------------------------
//	GENERAL LOW LEVEL ROUTINES/MACROS
//	-------------------------------------------------------------------
//	Some re-implementation/variations of C library functions
//	and general macros
//	-------------------------------------------------------------------



#define RectHeight(r)		((r).bottom-(r).top)

//	-------------------------------------------------------------------
//	Generic assertion => break into debugger
//	-------------------------------------------------------------------

void assert( int v, const char *msg )
{
	if (!v)
	{
		char buffer[1024];
		strcpy( buffer+1, msg );
		buffer[0] = strlen( msg );
		DebugStr( buffer );
	}
}

//	-------------------------------------------------------------------
//	Concatenates a Pascal String and a C String
//	-------------------------------------------------------------------

void StrCatPC( Str255 p, const char *q )
{
	memcpy( p+p[0]+1, q, strlen( q ) );
	p[0] += strlen( q );
}


//	-------------------------------------------------------------------
//	GENERAL LOG FUNCTIONS
//	-------------------------------------------------------------------
//	Basic log functions to be able to log from within the interrupt handler
//	-------------------------------------------------------------------

static char *dlog_buffer;
static char *dlog_head;
static char *dlog_tail;
static char *dlog_end;

//	-------------------------------------------------------------------
//	Inits the log system
//	-------------------------------------------------------------------

void dinit_log()
{
#ifdef VERBOSE
	dlog_buffer = (char *)NewPtr( 50000 );
	if (dlog_buffer==NULL)
	{
		ExitToShell();
	}
	dlog_head = dlog_buffer;
	dlog_tail = dlog_head;
	dlog_end = dlog_buffer+50000;
#endif
}

//	-------------------------------------------------------------------
//	Custom memcpy
//	-------------------------------------------------------------------

static void my_memcpy( char *d, const char *f, int len )
{
	while (len--)
		*d++ = *f++;
}

//	-------------------------------------------------------------------
//	Custom memcmp (unused)
//	-------------------------------------------------------------------

static int my_memcmp( char *d, const char *f, int len )
{
	while (len--)
		if (*d++!=*f++)
			return d[-1]-f[-1];
	return 0;
}

//	-------------------------------------------------------------------
//	Logs raw bytes
//	-------------------------------------------------------------------

void dlog( const char *data, int len )
{
#ifdef VERBOSE
	if (len<dlog_end-dlog_head)
	{
		my_memcpy( dlog_head, data, len );
		dlog_head += len;
	}
#endif
}

//	-------------------------------------------------------------------
//	Logs c string
//	-------------------------------------------------------------------

void dlog_str( const char *str )
{
	int size = strlen( str );
	dlog( str, size );
}

//	-------------------------------------------------------------------
//	Logs number
//	-------------------------------------------------------------------

void dlog_int( long num )
{
	char buffer[255];
	NumToString( num, buffer );
	buffer[buffer[0]+1] = 0;
	dlog_str( buffer+1 );
}


//	-------------------------------------------------------------------
//	SCREEN HANDLING FUNCTIONS
//	-------------------------------------------------------------------
//	Save/restore screen
//	-------------------------------------------------------------------



//	-------------------------------------------------------------------
//	Allocated memory and saves screen
//	-------------------------------------------------------------------

void SaveScreen( Ptr *ptr )
{
	long count = ((long)screenBits.rowBytes)*RectHeight(screenBits.bounds);

	*ptr = NewPtr( count );

	if (*ptr)
		BlockMove( screenBits.baseAddr, *ptr, count );
}

//	-------------------------------------------------------------------
//	Restore saved screen and deallocates memory
//	-------------------------------------------------------------------

void RestoreScreen( Ptr *ptr )
{
	if (ptr)
	{
		long count = ((long)screenBits.rowBytes)*RectHeight(screenBits.bounds);
		BlockMove( *ptr, screenBits.baseAddr, count );
		DisposPtr( *ptr );
		*ptr = NULL;
	}
}










//	-------------------------------------------------------------------
//	CODEC DISPLAY FUNCTIONS
//	-------------------------------------------------------------------
//	Display functions
//	All codec display functions are implemented twice:
//	- A slow, C-based, reference implementation that works on
//	  screen larger than 512x342 (usedful for development and testing)
//	- A faster, 68x assembly version, suitable for real-world mac
//	-------------------------------------------------------------------



typedef void (*DisplayProc)( char *dest, char *source, int rowbytes );

//	-------------------------------------------------------------------
//	Codec 0x00 : null
//	-------------------------------------------------------------------

void Null_ref( char *dest, char *source, int rowbytes )
{
}

//	-------------------------------------------------------------------
//	Codec 0x01 : z16 (reference implementation)
//	  Copies series of vertical variable height 16 pixels information,
//	Format:
//	  A series of chunks
//	  2 byte header : (0x0000 to end)
//		9 bits offset from screen top
//		7 bits count of data
//	  1 byte        : offset (starts at top left of screen)
//	  1 byte        : count (0=>end of data)
//	  count words   : data to be copied at vertical 16 pixels line
//	-------------------------------------------------------------------

void UnpackZ16_ref( char *dest, char *source, int rowbytes )
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
//	Codec 0x01 : z16
//	-------------------------------------------------------------------

void UnpackZ16( char *dest, char *source, int rowbytes )
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
//	Codec 0x02 : z32 (reference implementation)
//	  Copies series of vertical variable height 32 pixels information,
//	Format:
//	  A series of chunks
//	  4 byte header : (0x00000000 to end)
//		2 bytes     : count of data to copy offset from screen top, minus 1
//		2 bytes     : offset from the top of the screen, plus 4
//	  count quads   : data to be copied at vertical 32 pixels line
//	-------------------------------------------------------------------

void UnpackZ32_ref( char *dest, char *source, int rowbytes )
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
		assert( copy<=342, "Bad Z32 count" );
#endif

		d = base+offset;

		while (copy--)
		{
			*d = *s++;
			d += rowlongs;
		}
	}
}

void UnpackZ32_80( char *dest, char *source, int rowbytes )
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
//	Codec 0x02 : z32
//	-------------------------------------------------------------------

void UnpackZ32( char *dest, char *source, int rowbytes )
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
//	Codec 0x03 : invert (reference implementation)
//	Inverts the whole screen
//	-------------------------------------------------------------------

void Invert_ref( char *dest, char *source, int rowbytes )
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
//	Codec 0x03 : invert
//	-------------------------------------------------------------------

void Invert( char *dest, char *source, int rowbytes )
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
//	-------------------------------------------------------------------

void CopyLines_ref( char *dest, char *source, int rowbytes )
{
	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	if (offset<0 || offset>=21888)
		ExitToShell();
	if (len<0 || len>21888)
		ExitToShell();
	if ((len%64)!=0)
		ExitToShell();
	if (offset+len>21888)
		ExitToShell();

	source += 4;
	dest += offset;
	while (len)
	{
		BlockMove( source, dest, 64 );
		source += rowbytes;
		dest += 64;
		len -= 64;
	}
}

//	-------------------------------------------------------------------
//	Codec 0x04 : copy lines
//	Copies a serie of horizontal lines
//	2 bytes     : count of bytes to copy
//	2 bytes     : offset to copy to
//	count bytes : data to copy
//	-------------------------------------------------------------------

void CopyLines( char *dest, char *source, int rowbytes )
{
	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	BlockMove( source+4, dest+offset, len );
}




//	-------------------------------------------------------------------
//	SCREEN HANDLING FUNCTION
//	-------------------------------------------------------------------
//	Dispatches to the right codec functions
//	Provides a few direct access routines
//	-------------------------------------------------------------------


//	-------------------------------------------------------------------
//	A "Screen" suitable for playback
//	-------------------------------------------------------------------

#define kCodecCount 6	//	Maximum of 6 codecs

struct ScreenRecord
{
	char *physAddr;		//	Real physical top of screen
	size_t height;		//	Number of lines

	char *baseAddr;		//	Base addr of start of image (may be in the middle of the physical screen)
	size_t rowBytes;	//	Number of bytes between a line and the next
	size_t stride4;		//	Number of longs from the end of a line to the beginning of the next
						//	stride4*4+64 == rowBytes

						//	The codecs implementation to use
	DisplayProc procs[kCodecCount];
};

typedef struct ScreenRecord *ScreenPtr;

//	-------------------------------------------------------------------
//	Fills scrn with information to display on the physical screen
//	-------------------------------------------------------------------

ScreenPtr ScreenInit( ScreenPtr scrn )
{
	int i;

	int w = screenBits.bounds.right-screenBits.bounds.left;
	int h = screenBits.bounds.bottom-screenBits.bounds.top;

	assert( scrn!=NULL, "Screen not created" );

	scrn->physAddr = screenBits.baseAddr;
	scrn->height = h;

	scrn->rowBytes = screenBits.rowBytes;

	scrn->stride4 = (scrn->rowBytes-64)/4;

	scrn->baseAddr = scrn->physAddr + ((w-512)/2)/8;
	scrn->baseAddr += scrn->rowBytes*((h-342)/2);

	for (i=0;i!=kCodecCount;i++)
		scrn->procs[i] = Null_ref;

		//	This is slow, but work on every reasonable B&W macs
	scrn->procs[0] = Null_ref;
	scrn->procs[1] = UnpackZ16_ref;
	scrn->procs[2] = UnpackZ32_ref;
	scrn->procs[3] = Invert_ref;
	scrn->procs[4] = CopyLines_ref;

#ifndef REFERENCE
	if (scrn->rowBytes==64)				//	Vintage macs
	{
		scrn->procs[1] = UnpackZ16;
		scrn->procs[2] = UnpackZ32;
		scrn->procs[3] = Invert;
		scrn->procs[4] = CopyLines;
	}

	if (scrn->rowBytes==80)				//	Macintosh Portable
	{
		scrn->procs[2] = UnpackZ32_80;
	}
#endif
	return scrn;
}

//	-------------------------------------------------------------------
//	Clear full physical screen to black (slow)
//	-------------------------------------------------------------------

void ScreenClear( ScreenPtr scrn )
{
	long *p = (long *)scrn->physAddr;

	long n = scrn->height*(long)scrn->rowBytes/4+1;
	while (--n)
	{
		*p++ = 0xffffffffL;
	}
}

//	-------------------------------------------------------------------
//	Flashes the screen (slow)
//	-------------------------------------------------------------------

void ScreenFlash( ScreenPtr scrn, size_t from, size_t lines )
{
	long *p = (long *)(scrn->baseAddr+from*scrn->rowBytes);
	size_t stride4 = scrn->stride4;

	int y = lines+1;
	while (--y)
	{
		int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += stride4;
	}
}

//	-------------------------------------------------------------------
//	Uncompress a frame on the screen
//	-------------------------------------------------------------------

void ScreenUncompressFrame( ScreenPtr scrn, char *source )
{
	char codec = source[3];
	if (codec<0 || codec>=kCodecCount)
		ExitToShell();
	(scrn->procs[codec])( scrn->baseAddr, source+4, scrn->rowBytes );
}

#define noSLOW





//	-------------------------------------------------------------------
//	FLIM FILE PARSING FUNCTIONS
//	-------------------------------------------------------------------
//	All routines for parsing the flim files
//	and provide "Block" data structures for playback
//	-------------------------------------------------------------------


/*
	File format:
		File header: 'FLIM' + 0x0a + 1017 characters of comments + 2 character of checksum.

		Checksum is the fletcher16 of bytes 1024 to the end of file.

		First video block starts at 0x400 (1024)
	
		block:
			4 bytes : block_size (big endian)
			          next block will start in block_size-4 bytes
			4 bytes : magic 'FLIM'
			2 bytes : frame_count (big endian)
			          number of sound+video 'frames' in the block

		frame_count times:
			2 bytes : sound_size (big endian)
					  including 2 bytes sound_size and header,
					  eg: 378 if only a single tick of data)
			6 bytes : sound header
			n bytes : sound_size-8 bytes of sound data
					  8 bits unsigned mono 22200Hz
			          size multiple of 370
			2 bytes : video_size (big endian)
			          size of encoded video for this frame
			          including 2 bytes of video_size
			n bytes : video_size-2 bytes of video data
*/





//	-------------------------------------------------------------------
//	Utility function for file parsing
//	-------------------------------------------------------------------

unsigned short as2( unsigned char *p )
{
	return p[0]*256+p[1];
}

//	-------------------------------------------------------------------

unsigned long as4( unsigned char *p )
{
	return as2(p)*65536L+as2(p+2);
}

//	-------------------------------------------------------------------
//	Each frame is composed of FrameDataRecords
//	First one is sound
//	Second is video
//	...then iterates back
//	-------------------------------------------------------------------

struct FrameDataRecord
{
	unsigned short data_size;		//	Size of the data, including header
	unsigned char  data[1];			//	Data
};

typedef struct FrameDataRecord *FrameDataPtr;

//	-------------------------------------------------------------------
//	Skips to the next frame data ptr
//	-------------------------------------------------------------------

FrameDataPtr NextDataPtr( FrameDataPtr ptr )
{
	return (FrameDataPtr)(((char *)ptr)+ptr->data_size);
}

//	-------------------------------------------------------------------
//	Block states
//	When in-memory, blocks go via different states in their lifecycle
//
//	unused -> reading -> ready -> playing -> played -> closed
//				^                              |
//				+------------------------------+
//
//	-------------------------------------------------------------------

enum
{
	blockUnused = 0,		//	Nothing to be done
	blockReading = 1,		//	A PBRead is active on the block
	blockReady = 2,			//	The block have data in read from the disk
	blockPlaying = 3,		//	Content of the block is currently used by the sound driver
	blockPlayed = 4,		//	The content of the block have been played
	blockClosed = 5			//	We don't use this block anymore, buffer deallocated
};

//	-----------------------------¬--------------------------------------
//	In-memory block record
//	-------------------------------------------------------------------

struct BlockRecord
{
	short status;			//	The status of this block
	Ptr buffer;				//	The data buffer we use

	FrameDataPtr sound;		//	The current sound
	FrameDataPtr video;		//	The current video

	short frames_left;		//	The number of frames left to play
};

typedef struct BlockRecord *BlockPtr;

//	-------------------------------------------------------------------
//	Creates an in-memory block
//	-------------------------------------------------------------------

void BlockInit( BlockPtr blk )
{
	blk->status = blockUnused;
	blk->buffer = NewPtr( BUFSIZE );
	assert( blk->buffer!=NULL, "Buffer allocation failed" );
}

//	-------------------------------------------------------------------
//	Parses a block content (private)
//	-------------------------------------------------------------------

static long BlockParse( BlockPtr blk, long size )
{
	unsigned char *data = (unsigned char *)blk->buffer;

	long next_size = as4( data+size-4 );

	assert( next_size<=BUFSIZE, "BlockParse: next_size too large for buffer" );

		//	The raw data from the disk
	if (as4( data )!='FLIM')
		ExitToShell();

	data = data+4;

	blk->frames_left = as2( data );
	data += 2;

	blk->sound = (FrameDataPtr)data;
	blk->video = NextDataPtr( blk->sound );

		//	Block is now ready to be played
	blk->status = blockReady;

	return next_size;
}

//	-------------------------------------------------------------------
//	Sync read a block from disk
//	-------------------------------------------------------------------

long BlockRead( BlockPtr blk, short fRefNum, long read_size )
{
	OSErr err;
	long ret_val;

	blk->status = blockReading;
	assert( read_size<=BUFSIZE, "BlockRead: read_size too large" );

	err = FSRead( fRefNum, &read_size, blk->buffer );
	
	ret_val = BlockParse( blk, read_size );

	if (err!=noErr)
		return 0;	//	EOF
	return ret_val;
}

//	-------------------------------------------------------------------
//	Set the block state to playing
//	-------------------------------------------------------------------

void BlockWillPlay( BlockPtr blk )
{
	assert( blk->status==blockReady, "BlockWillPlay: block is not ready" );
	blk->status = blockPlaying;
}

//	-------------------------------------------------------------------
//	Set the block state to played
//	-------------------------------------------------------------------

void BlockDidPlay( BlockPtr blk )
{
	assert( blk->status==blockPlaying, "BlockDidPlay: block still playing" );
	blk->status = blockPlayed;
}

//	-------------------------------------------------------------------
//	Global variables
//	#### MOVE ME
//	-------------------------------------------------------------------

//	The screen
struct ScreenRecord gScreenRec;
ScreenPtr gScreen = &gScreenRec;

//	Dual blocks for read and play

struct BlockRecord gBlocks[2];	//	Must never be accessed other than by GetFirstBlock and GetOtherBlock

//	Silence
FFSynthPtr silence;

//	Per counters
long gStalledTicks = 0;
long gReentryCount = 0;
long gPlayedFrames = 0;
long gSpinCount = 0;

//	The nested interuption level, to count reentries
static int gInter = 0;

//	Global state of player
enum State
{
	playingState = 0,
	stopRequestedState,
	stoppedState,
	pausedState
};

short gState;

//	The param block used to send work to the sound driver

ParamBlockRec pb;

//	The current block getting read
BlockPtr gReadBlock;

//	The current block getting played
BlockPtr gPlaybackBlock;

typedef enum
{
	kError,
	kDone,
	kAbort,
	kSkip,
	kPrevious,
	kRestart
}	ePlayResult;

//	-------------------------------------------------------------------
//	Displays help panel
//	-------------------------------------------------------------------

void DisplayHelp()
{
	enum State state = gState;
	Ptr savePtr;
	long ticks = Ticks;
	DialogPtr theDialog;
	DialogPtr theDialog1;
	DialogPtr theDialog2;

	gState = pausedState;

	while (Ticks<ticks+5)
		;

	SaveScreen( &savePtr );

	theDialog1 = GetNewDialog( 128, NULL, (WindowPtr)-1 );
	theDialog2 = GetNewDialog( 134, NULL, (WindowPtr)-1 );

	if (!theDialog1 || !theDialog2)
		goto done ;

	theDialog = theDialog1;

	if (savePtr==NULL)
		goto done;

	ShowWindow( theDialog );
	DrawDialog( theDialog );
	ticks = Ticks + 60*5;

	while (!Button())
	{
		CheckKeys();
		if (sHelp)
			break;
		if (ticks<Ticks)
		{
			HideWindow( theDialog );
			if (theDialog==theDialog1)
				theDialog = theDialog2;
			else
				theDialog = theDialog1;
			ShowWindow( theDialog );
			DrawDialog( theDialog );
			ticks = Ticks + 60*5;
		}
	}
	
done:
	if (theDialog1)
		DisposDialog( theDialog1 );
	if (theDialog2)
		DisposDialog( theDialog2 );

	while (Button())
		;

	RestoreScreen( &savePtr );

	gState = state;
}

//	-------------------------------------------------------------------
//	Wait for a block to be played
//	Iterates until the block is ready to be played
//	Dumps the logs to the console
//	Returns:
//	kError, kDone, kAbort, kSkip or kRestart
//	-------------------------------------------------------------------

ePlayResult BlockWaitPlayed( BlockPtr blk )
{
	short status;
	do
	{
		status = blk->status;
		assert( status==blockPlaying || status==blockPlayed, "BlockWaitPlayed: wrong status" );

		while (dlog_tail!=dlog_head)
		{
			printf( "%c", *dlog_tail++ );
			fflush( stdout );
		}	

		CheckKeys();
		if (sEscape)
			return kAbort;
		if (sSkip)
			return kSkip;
		if (sPrevious)
			return kPrevious;
		if (sRestart)
			return kRestart;
		if (sHelp)
			DisplayHelp();

		if (sPause)
		{
			enum State state = gState;

			gState = pausedState;

			do
			{
				CheckKeys();
				if (sEscape)
					return kAbort;
				if (sSkip)
					return kSkip;
				if (sPrevious)
					return kPrevious;
				if (sRestart)
					return kRestart;
				if (sHelp)
					DisplayHelp();
			}
			while (!sPause);

            gState = playingState;
		}

		gSpinCount++;

	}	while (status==blockPlaying);

	return kDone;
}

//	-------------------------------------------------------------------
//	Block deallocate
//	-------------------------------------------------------------------

void BlockKill( BlockPtr blk )
{
	blk->status = blockClosed;
	DisposPtr( blk->buffer );
	blk->buffer = NULL;
}

//	-------------------------------------------------------------------
//	Returns the first buffer
//	-------------------------------------------------------------------

BlockPtr GetFirstBlock()
{
	return gBlocks;
}

//	-------------------------------------------------------------------
//	Returns the other buffer
//	-------------------------------------------------------------------

BlockPtr GetOtherBlock( BlockPtr blk )
{
	if (blk==gBlocks)
		return &gBlocks[1];
	return gBlocks;
}

//	-------------------------------------------------------------------
//	Generates a tick of silence
//	-------------------------------------------------------------------

static void DoSilence( ProcPtr callback )
{
	OSErr sound_err;
	assert( silence!=NULL, "Silence buffer unallocated" );
	pb.ioParam.ioRefNum = -4;
	pb.ioParam.ioBuffer = (Ptr)silence;
	pb.ioParam.ioReqCount = 370;
	pb.ioParam.ioCompletion = callback;
	sound_err = PBWrite(&pb, TRUE);
	assert( sound_err==noErr, "DoSilence: PBWrite failed" );
}

//	-------------------------------------------------------------------
//	Callback to play a frame of sound and display a frame of video
//	-------------------------------------------------------------------

static pascal void DoFrame()
{
	OSErr err;

		//	Find globals for A5
	asm
	{
		move.l a5, -(a7)
		move.l 0x904, a5
	}

#ifdef VERBOSE
	dlog_str( "CALLBACK #" );
	dlog_int( gPlaybackBlock->frames_left );
	dlog_str( " " );
#endif

	if (gState==pausedState)
	{
	    DoSilence( (ProcPtr)DoFrame );
		goto end;
	}

	if (gInter==1)
	{
		gReentryCount++;
	    DoSilence( (ProcPtr)DoFrame );
		goto end;
	}


		//	If request to stop, do not play sound, no more callbacks, and notify new state
	if (gState==stopRequestedState)
	{
		gState = stoppedState;
		goto end;	//	So? Sue me.
	}

		//	We have displayed all the frames from this block
		//	We can now release the block
	if (gPlaybackBlock->frames_left==0)
	{
		if (gPlaybackBlock->status==blockPlaying)
		{
			gPlaybackBlock->status = blockPlayed;	//	Now, the main loop can start filling this block again
		}

			//	We want to switch to the other block
		if (GetOtherBlock( gPlaybackBlock )->status==blockReady)
		{
#ifdef VERBOSE
			dlog_str( "\nSWITCHING " );
#endif
			gPlaybackBlock = GetOtherBlock( gPlaybackBlock );
			gPlaybackBlock->status = blockPlaying;
		}
		else
		{
#ifdef VERBOSE
			dlog_str( " STALLED " );
#endif
			//	We have no available blocks
			//	We don't have enough resources
			DoSilence( (ProcPtr)DoFrame );
			gStalledTicks++;
			goto end;
		}
	}

		//	Play one frame of sound
	gPlayedFrames++;

	gInter++;

	pb.ioParam.ioRefNum = -4;
	pb.ioParam.ioBuffer = (Ptr)gPlaybackBlock->sound->data;
	if (sMuted)
	{
		pb.ioParam.ioBuffer = (Ptr)silence;
	}
	pb.ioParam.ioReqCount = gPlaybackBlock->sound->data_size-8 /* size+header */;
	pb.ioParam.ioCompletion = (ProcPtr)DoFrame;

/*	if (muted)		//	more than 5 ticks of silence are needed...
	{
		register int i;
		register int c = pb.ioParam.ioReqCount;
		register char *p = pb.ioParam.ioBuffer+6;
		for (i=0;i!=c;i++)
			*p++ = 128;
	}
*/
	err = PBWrite(&pb, TRUE);
	ScreenUncompressFrame( gScreen, (char *)gPlaybackBlock->video->data );

		//	Compute next frame
	gPlaybackBlock->frames_left--;
	gPlaybackBlock->sound = NextDataPtr( gPlaybackBlock->video );
	gPlaybackBlock->video = NextDataPtr( gPlaybackBlock->sound );

	gInter--;

end:

		//	Restore previous A5
	asm
	{
		move.l (a7)+, a5
	}
}

//	-------------------------------------------------------------------
//	Starts sound
//	-------------------------------------------------------------------

void FlimSoundStart()
{
	long i;
	int n = 0;
	int k = 1;

	assert( silence==NULL, "Silence already allocated" );
	silence = (FFSynthPtr)NewPtr(2+4+370*12);
	assert( silence!=NULL, "Silence allocation failed" );
	silence->mode = ffMode;
	silence->count = FixRatio(1,1);
	for (i=0;i!=370*12;i++)
		silence->waveBytes[i] = 128;

	gPlaybackBlock->status = blockPlaying;

	gState = playingState;

	DoFrame();
}

//	-------------------------------------------------------------------
//	Stops the flim
//	-------------------------------------------------------------------

void FlimSoundStop()
{
	gState = stopRequestedState;

	while (gState==stopRequestedState)
		;

//	Does not work. Unsure why
//	KillIO(-4);  /* stops all pending StartSounds, see Sound Manager in IM2 */

	assert( silence!=NULL, "Silence unallocated" );
	DisposPtr( (Ptr)silence );
	silence = NULL;
}

//	-------------------------------------------------------------------
//	Plays movie synchronously, with no sound
//	Alternates disk reads and displays
//	Usefull to debug display code or measure raw performance
//	-------------------------------------------------------------------

void FlimSyncPlay( short fRefNum )
{
	unsigned char *data;
	long read_size;
	struct BlockRecord blockRecord;
	BlockPtr blk = &blockRecord;
	long tick;
	int block = 0;
	int slow_count = 0;

	gScreen = ScreenInit( gScreen );
	ScreenClear( gScreen );

	read_size = 4;
	FSRead( fRefNum, &read_size, data );
	read_size = as4( data );

	BlockInit( blk );

	tick = TickCount();

	while (read_size)
	{
		int frame,j;
	
		read_size = BlockRead( blk, fRefNum, read_size );
		
		if (Button())
			goto end;
		
		for (frame=0;frame!=blk->frames_left;frame++)
		{
			ScreenUncompressFrame( gScreen, (char *)blk->video->data );
			blk->sound = NextDataPtr( blk->video );
			blk->video = NextDataPtr( blk->sound );
		}
		block++;
	}

end:
	
	tick = TickCount()-tick;

	while (!Button())
		;
	while (Button())
		;
	
	printf( "Ticks = %ld\n", tick );

	BlockKill( blk );

	while (!Button())
		;
}

//	-------------------------------------------------------------------
//	Plays a file
//	-------------------------------------------------------------------

ePlayResult PlayFlimFile( Str255 fName, short vRefNum )
{
	OSErr err;
	short fRefNum;
	long read_size;
	long next_read_size;
	long tick = 0;
	ePlayResult theResult;


begin:
	theResult = kDone;

		//	Open flim
	err = FSOpen( fName, vRefNum, &fRefNum );
	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return kError;
	}

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1024 );

#ifdef TESTING
	FlimSyncPlay( fRefNum );
	goto close;
#endif

		//	Set up screen
	gScreen = ScreenInit( gScreen );
	ScreenClear( gScreen );

		//	We read the first 4 bytes of the file into read_size
	read_size = 4;
	FSRead( fRefNum, &read_size, &next_read_size );
	read_size = next_read_size;
	if (read_size>BUFSIZE)
	{
		printf( "THIS IS BAD : read_size=%ld\n", read_size );
		theResult = kError;
		goto end;
	}
	
		//	We init the two blocks
	BlockInit( GetFirstBlock() );
	BlockInit( GetOtherBlock(GetFirstBlock()) );

		//	We read the first one from disk
	gReadBlock = GetFirstBlock();
	read_size = BlockRead( gReadBlock, fRefNum, read_size );

		//	This will be the first playback block
	gPlaybackBlock = gReadBlock;

		//	We read the second block from disk
	gReadBlock = GetOtherBlock( gReadBlock );
	read_size = BlockRead( gReadBlock, fRefNum, read_size );

		//	This is the block we would love to read from
	gReadBlock = GetOtherBlock( gReadBlock );

		//	Start flim sound
	while (dlog_tail!=dlog_head)
	{
		printf( "%c", *dlog_tail++ );
		fflush( stdout );
	}	

	FlimSoundStart();

	tick = TickCount();

	do
	{
		dlog_str( "\n MAIN LOOP WILL WAIT\n" );
		theResult = BlockWaitPlayed( gReadBlock );
		if (theResult!=kDone)
		{
			goto end;	//	User abort
		}

		dlog_str( "\n MAIN WILL READ\n" );

			//	We sync load data into the next buffer into the one we just played
		read_size = BlockRead( gReadBlock, fRefNum, read_size );

			//	This is the next we would love to read from
		gReadBlock = GetOtherBlock( gReadBlock );

	}	while (read_size!=0);

		//	Start last block
	theResult = BlockWaitPlayed( gReadBlock );
	
		//	Wait until finished
	while (gPlaybackBlock->status==blockPlaying)
		;

end:

	FlimSoundStop();

	tick = TickCount()-tick;
	BlockKill( GetFirstBlock() );
	BlockKill( GetOtherBlock(GetFirstBlock()) );

#ifdef DISPLAY_STATS
	while (!Button())
		;
	while (Button())
		;

	printf( "Stats:\n" );
	printf( "# of re-entries  (code was in callback when callback completed) : %ld\n", gReentryCount );
	printf( "# of stalled ticks (callback was not able to provide sound)     : %ld\n", gStalledTicks );
	printf( "# of played frames (sound was provided)                         : %ld\n", gPlayedFrames );
	printf( "# of elapsed ticks (measured from the outside)                  : %ld\n", tick );
	printf( "# of wait spings (busy loop count)                              : %ld\n", gSpinCount );
	printf( "\n  --- press mouse to exit MacFlim Player Demo ---\n" );

	while (Button())
		;
	while (!Button())
		;
#endif

close:
	FSClose( fRefNum );

	if (theResult==kRestart)
		goto begin;

	return theResult;
}

//	-------------------------------------------------------------------
//	Autoplays all the flims selected from the Finder in a loop
//	Returns FALSE if there were no such files
//	-------------------------------------------------------------------

Boolean AutoPlayFlims()
{
	short doWhat = appOpen;
	short fileCnt = 0;
	AppFile theAppFile;
	int i;

	CountAppFiles(&doWhat,&fileCnt);

		//	No files, nothing to do
	if (fileCnt==0)
		return FALSE;

	for (;;)
	{
			//	Multiple files, we play each of them in turn
		for (i=0;i!=fileCnt;i++)
		{
			ePlayResult theResult;
			GetAppFiles( i+1, &theAppFile );
			//	If a file is cancelled, we abort everything
			//	(note: this means that a bad file will abort the looping too)
			
			theResult = PlayFlimFile( theAppFile.fName, theAppFile.vRefNum );
			if (theResult==kError || theResult==kAbort)
				goto end;
			if (theResult==kRestart)
				i = i-1;
			//	kSkip or kDone, nothing to do
			if (theResult==kPrevious)
			{
				i = i-2;
				if (i==-2)
					i = fileCnt-2;
			}
			ClrAppFiles( i+1 );
		}
	}

end:
	return TRUE;
}

//	-------------------------------------------------------------------
//	Filter function : returns TRUE if file is not a FLIM
//	-------------------------------------------------------------------

pascal Boolean FileFilter( FileParam *pbp )
{
	OSErr err;
	short fRefNum;
	long read_size = 5;
	char buffer[522];
	static char magic[] = { 'F', 'L', 'I', 'M', 0x0a };
	Boolean theResult = TRUE;

		//	Constructing an HFS param block
	HParamBlockRec pb;
	pb.ioParam.ioNamePtr = pbp->ioNamePtr;
	pb.ioParam.ioVRefNum = pbp->ioVRefNum;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.ioParam.ioMisc = 0;
	pb.ioParam.ioVersNum = 0;
	pb.fileParam.ioDirID = CurDirStore;
	
		//	Open file
	err = PBHOpen( &pb, FALSE );
	if (err!=noErr)
	{
		return TRUE;
	}

		//	Read 5 chars
	pb.ioParam.ioReqCount = 5;
	pb.ioParam.ioPosMode = fsFromStart;
	pb.ioParam.ioPosOffset = 0;
	pb.ioParam.ioBuffer = buffer;
	err = PBRead( &pb, FALSE );
	if (err==noErr)
	{
		if (memcmp(buffer, magic, 5)==0)
			theResult = FALSE;
	}

		//	Close file
	PBClose( &pb, 0 );

	return theResult;
}

//	-------------------------------------------------------------------
//	Returns TRUE is flim file type is correct
//	-------------------------------------------------------------------

Boolean IsFlimTypeCorrect( Str255 fName, short vRefNum )
{
	FInfo fInfo;
	if (GetFInfo( fName, vRefNum, &fInfo )==noErr && fInfo.fdType=='FLIM')
		return TRUE;
	return FALSE;
}

//	-------------------------------------------------------------------
//	Sets the flim type
//	-------------------------------------------------------------------

void SetFlimTypeCreator( Str255 fName, short vRefNum )
{
	FInfo fInfo;
	if (GetFInfo( fName, vRefNum, &fInfo )==noErr)
	{
		fInfo.fdType='FLIM';
		fInfo.fdCreator='FLPL';
		SetFInfo( fName, vRefNum, &fInfo );
	}
}

//	-------------------------------------------------------------------
//	Returns TRUE is flim file contains a checksum
//	-------------------------------------------------------------------

unsigned short FlimChecksum( Str255 fName, short vRefNum )
{
	unsigned fletcher = 0xffff;
	OSErr err;
	short fRefNum;

		//	Open flim
	err = FSOpen( fName, vRefNum, &fRefNum );
	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return;
	}

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1022 );
	if (err==noErr)
	{
			//	Read Fletcher-16
		long read_size = 2;
		FSRead( fRefNum, &read_size, &fletcher );
	}


	FSClose( fRefNum );
	
	return fletcher;
}

//	-------------------------------------------------------------------
//	Reference implementation of fletcher16 checksum
//	Adds all words, modulo 65535
//	-------------------------------------------------------------------

unsigned short fletcher16_ref( register unsigned short fletcher, register unsigned short *buffer, long count )
{
	register long i;
	register long f = fletcher;

	for (i=0;i!=count;i++)
	{
		f += buffer[i];
		if (f>=65535)
			f -= 65535;
	}
	
	return f;
}

//	-------------------------------------------------------------------
//	Assembly implemetation of fletcher16
//	Works on buffer of 1 to 65536 words (pass 0 for count)
//	-------------------------------------------------------------------

unsigned short fletcher16_short( unsigned short fletcher, unsigned short *buffer, unsigned short count )
{
	asm{
	
		move.w fletcher,d0
		movea.l buffer,a0
		move.w count,d1
		subq.w #1,d1

@loop:
		add.w (a0)+,d0
		bcc @skip
		addq.w #1,d0
@skip:
		dbra d1,@loop
		move.w d0,fletcher
	}
	

	return fletcher;
}

//	-------------------------------------------------------------------
//	Wrapper around assembly version of fletcher
//	-------------------------------------------------------------------

unsigned short fletcher16( unsigned short fletcher, unsigned short *buffer, long count )
{
	while (count>65536	)
	{
		fletcher = fletcher16_short( fletcher, buffer, 65536 );	//	Note: 65536 is passed as 0
		count -= 65536;
		buffer += 65536;
	}
	return fletcher16_short( fletcher, buffer, count );
}

//	-------------------------------------------------------------------
//	Performs the integrity check, with skippable dialog
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrity( Str255 fName, short vRefNum, unsigned short fletcher_ref )
{
	OSErr err;
	short fRefNum;
	long read_size;
	unsigned short *theBuffer = NULL;
	unsigned short fletcher;
	long total_size = 0;
	long size = 1024;	//	The header we skip
	int percent;
	int last_percent = -1;
	DialogPtr theProgress = NULL;
	Handle thePercentText;
	short dummy0;
	Rect dummy1;
	Boolean skip = false;

	//	We use the same BUFSIZE to avoid a memory fragmentation issue
#define BUFFER_SIZE BUFSIZE
#define kProgressDlog 129
#define kProgressItem 1

		//	Open flim
	err = FSOpen( fName, vRefNum, &fRefNum );
	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return;
	}

		//	Get file size
	err = GetEOF( fRefNum, &total_size );

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1024 );

	theBuffer = (unsigned short *)NewPtr( BUFFER_SIZE );

	fletcher = 0;
	
	while (Button())
		;

	theProgress = GetNewDialog( kProgressDlog, NULL, (WindowPtr)-1 );
	GetDItem( theProgress, kProgressItem, &dummy0, &thePercentText, &dummy1 );
	SetIText( thePercentText, "" );
	ShowWindow( theProgress );
	DrawDialog( theProgress );

	while (1)
	{
		register long i;
	
		read_size = BUFFER_SIZE;
		err = FSRead( fRefNum, &read_size, theBuffer );
		if (err!=noErr && err!=eofErr)
		{
			printf( "READ ERROR=%d\n", err );
			goto done;
		}

		size += read_size;

		if (read_size!=BUFFER_SIZE)	//	Truncate last byte is needed
			read_size -= read_size&1;

		percent = size*100.0/total_size;
		if (percent!=last_percent)
		{
			Str255 theBuffer;
			last_percent = percent;
			NumToString( percent, theBuffer );
			StrCatPC( theBuffer, "% complete" );
			SetIText( thePercentText, theBuffer );
		}

		fletcher = fletcher16( fletcher, theBuffer, read_size/2 );

		if (err==eofErr)
			break;
			
		if (Button())
		{
			skip= true;
			break;
		}
	};

done:
	FSClose( fRefNum );

	if (theBuffer) DisposPtr( theBuffer );

	if (theProgress) DisposDialog( theProgress );

	if (!skip)
	{
		if (fletcher_ref==0xffff)
		{
			ShowCursor();
			ParamText( "\pFlim have no checksum", "", "", "" );
			NoteAlert( 130, NULL );
			HideCursor();
		}
		else if (fletcher!=fletcher_ref)
		{
			ShowCursor();
			ParamText( "\pFlim is corrupted", "", "", "" );
			StopAlert( 131, NULL );
			HideCursor();
	
			return FALSE;
		}
	}

	return TRUE;
}

//	-------------------------------------------------------------------
//	Checks flim integrity if possible and accepted by user
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrityIfNeeded( Str255 fName, short vRefNum )
{
	unsigned short checksum = FlimChecksum( fName, vRefNum );
	if (checksum!=0xffff)
	{
		DialogPtr theCheckDialog = NULL;
		short itemHit;

		theCheckDialog = GetNewDialog( 131, NULL, (WindowPtr)-1 );
		ShowWindow( theCheckDialog );
		ModalDialog( NULL, &itemHit );
		DisposDialog( theCheckDialog );
		if (itemHit==2)
		{
			if (!CheckFlimIntegrity( fName, vRefNum, checksum ))
				return FALSE;
		}
	}
	
	return TRUE;
}

//	-------------------------------------------------------------------
//	Sets flim creator if needed and accepted by user
//	Does an optional integrity check
//	Return FALSE if file should not be played
//	-------------------------------------------------------------------

Boolean SetFlimTypeCreatorIfNeeded( Str255 fName, short vRefNum )
{
	DialogPtr theSetTypeDialog = NULL;
	short itemHit;

	if (IsFlimTypeCorrect( fName, vRefNum ))
	{
		unsigned char theKeys[16];
		GetKeys( theKeys );
		if (TestKey( theKeys, 0x3a ))	//	Option
			return CheckFlimIntegrityIfNeeded( fName, vRefNum );
		
		return TRUE;
	}

	theSetTypeDialog = GetNewDialog( 130, NULL, (WindowPtr)-1 );
	ShowWindow( theSetTypeDialog );
	ModalDialog( NULL, &itemHit );
	DisposDialog( theSetTypeDialog );	
	if (itemHit==1)
	{
		if (CheckFlimIntegrityIfNeeded( fName, vRefNum ))
			SetFlimTypeCreator( fName, vRefNum );
		else
			return FALSE;
	}

	return TRUE;
}

//	-------------------------------------------------------------------
//	Main function
//	-------------------------------------------------------------------

int main()
{
	Ptr savePtr;
	OSErr err;
	SFReply theReply;
	Point where;

		//	Mac toolbox init
	InitGraf( &thePort );
	InitFonts();
	FlushEvents( everyEvent, 0 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( NULL );
	InitCursor();
	MaxApplZone();

		//	Log subsystem
	dinit_log();

	HideCursor();

	SaveScreen( &savePtr );

		//	If we have flims to autoplay, let's do exactly that
	if (!AutoPlayFlims())
	{
			//	Ask user for a file
		where.h = 20;
		where.v = 90;
		ShowCursor();
		SFGetFile( where, 0, FileFilter, -1, NULL, 0, &theReply );
		
		if (theReply.good)
		{
			if (SetFlimTypeCreatorIfNeeded( theReply.fName, theReply.vRefNum ))
			{
				HideCursor();
				PlayFlimFile( theReply.fName, theReply.vRefNum );
			}
		}
	}

done:
	RestoreScreen( &savePtr );

	ShowCursor();

	FlushEvents( everyEvent, 0 );

	return 0;
}
