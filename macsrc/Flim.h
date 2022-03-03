#ifndef FLIM_INCLUDED__
#define FLIM_INCLUDED__

#include "Util.h"

//	-------------------------------------------------------------------
//	THE FLIM OBJECT AND ASSOCIATED FUNCTION
//	-------------------------------------------------------------------

/*
	File format: (NOT UP TO DATE)
		File header: 'FLIM' + 0x0a + 1013 characters of comments +
		
		2 character of checksum.

		Checksum is the fletcher16 of bytes 1024 to the end of file.

		Video header starts at 0x400 (1024)

		2 bytes version
		4 bytes toc offset
		4 bytes frame count
		2 bytes rowBytes
		2 bytes lines
		1 byte  silence bool
		53 bytes of filler

		frame_count times:
			2 bytes : ticks
			2 bytes : sound_size
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

		toc for each entry:
			2 bytes : size of frame
*/

struct FlimRec;
typedef struct FlimRec* FlimPtr;

//	-------------------------------------------------------------------
//	Opens a flim by name
//	-------------------------------------------------------------------

FlimPtr FlimOpenByName( Str255 fName, short vRefNum, long dirID, eFileAPI api );
FlimPtr FlimOpenByNameAnyVolumes( Str255 fName, short vRefNum, long dirID, eFileAPI api );

//	-------------------------------------------------------------------
//	Return last flim error (only used for open)
//	-------------------------------------------------------------------

OSErr FlimError( void );

//	-------------------------------------------------------------------
//	Deallocate associated data structures. Will close the file.
//	-------------------------------------------------------------------

void FlimDispos( FlimPtr flim );

//	-------------------------------------------------------------------
//	Unclear if needs to be exposed: seek flim to start (for replay)
//	-------------------------------------------------------------------

void FlimSeekStart( FlimPtr flim );

//	-------------------------------------------------------------------
//	Number of blocks in that flim
//	-------------------------------------------------------------------

Size FlimGetBlockCount( FlimPtr flim );

//	-------------------------------------------------------------------
//	Max block size of the flim
//	-------------------------------------------------------------------

Size FlimGetMaxBlockSize( FlimPtr flim );

//	-------------------------------------------------------------------
//	Number of frames in that block (for sync playback)
//	-------------------------------------------------------------------

int FlimGetBlockFrameCount( FlimPtr flim, int index );

//	-------------------------------------------------------------------
//	Size of the block (for sync playback)
//	-------------------------------------------------------------------

Size FlimGetBlockSize( FlimPtr flim, int index );

//	-------------------------------------------------------------------
//	Get file ref num (for playback) -- #### TO BE REMOVED
//	-------------------------------------------------------------------

int FlimGetFileRefNum( FlimPtr flim );

//	-------------------------------------------------------------------
//	Returns TRUE if flim is silent
//	-------------------------------------------------------------------

Boolean FlimGetIsSilent( FlimPtr flim );

//	-------------------------------------------------------------------
//	Allocate and creates a new poster for the flim
//	-------------------------------------------------------------------

PicHandle FlimCreatePoster( FlimPtr flim );

//	-------------------------------------------------------------------
//	Returns the (shared) poster for the flim
//	-------------------------------------------------------------------

PicHandle FlimGetPoster( FlimPtr flim );

//	-------------------------------------------------------------------
//	Returns the flim name
//	-------------------------------------------------------------------

const char *FlimGetName( FlimPtr flim );

struct FlimInfo
{
	short width;	//	yet unused in playback
	short height;	//	unused in playback
	Boolean dummy;	//	Padding
	Boolean silent;
	Size frameCount;
	Size ticks;
	short byterate;
	unsigned long codecs;	//	Bitmap of used codecs
};

struct FlimInfo *FlimGetInfo( FlimPtr flim );

#endif
