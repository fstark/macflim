#ifndef SCREEN_INCLUDED__
#define SCREEN_INCLUDED__

#include "Codec.h"

//	-------------------------------------------------------------------
//	A "Screen" suitable for playback
//	-------------------------------------------------------------------

struct ScreenRecord
{
		//	Screen-dependant data
	unsigned char *physAddr;		//	Real physical top of screen
	short width;		//	Width in pixels
	short height;		//	Height in pixels
	short rowBytes;		//	Number of bytes between a line and the next

		//	Playback-dependant data
	short playback_left;	//	Left of playback
	short playback_top;		//	Top of playback
	unsigned char *baseAddr;		//	Base addr of start of image (may be in the middle of the physical screen)

		//	Flim dependent data
	Boolean ready;		//	Ready to decode a flim

	short flim_width;
	short flim_height;

						//	The codec control block
	struct CodecControlBlock ccb;

	short stride4;		//	Number of longs from the end of a line to the beginning of the next
						//	stride4*4+[flim width] == rowBytes

						//	The codecs implementation to use
	DisplayProc procs[kCodecCount];
};

typedef struct ScreenRecord *ScreenPtr;

extern ScreenPtr gScreen;

//	-------------------------------------------------------------------
//	Places log at top left of screen
//	-------------------------------------------------------------------

void ScreenLogHome( ScreenPtr screen );

//	-------------------------------------------------------------------
//	Moves log to char position x, y
//	-------------------------------------------------------------------

void ScreenLogMoveTo( ScreenPtr screen, int x, int y );

//	-------------------------------------------------------------------
//	Logs a string
//	-------------------------------------------------------------------

void ScreenLogString( ScreenPtr screen, const char *s );

//	-------------------------------------------------------------------
//	Printf-style logging
//	-------------------------------------------------------------------

void ScreenLog( ScreenPtr screen, const char *format, ... );

//	-------------------------------------------------------------------
//	Displays an internal kill screen with the killcode
//	Clicking the Button will exit the application
//	Kill screen can be done from interrupt
//	-------------------------------------------------------------------

void KillScreen( ScreenPtr scrn, short killcode );

//	-------------------------------------------------------------------
//	Initialize a screen structure for playing on the current screen
//	-------------------------------------------------------------------

ScreenPtr ScreenInit( ScreenPtr scrn );
void ScreenDispos( ScreenPtr scrn );

//	-------------------------------------------------------------------
//	Clears screen to black
//	-------------------------------------------------------------------

void ScreenClear( ScreenPtr scrn );
void ScreenClearVideo( ScreenPtr scrn );

//	-------------------------------------------------------------------
//	Flashes a subset of the lines (useful for debugging)
//	-------------------------------------------------------------------

void ScreenFlash( ScreenPtr scrn, short from, short lines );

//	-------------------------------------------------------------------
//	Returns TRUE if screen is usable for playback
//	-------------------------------------------------------------------

Boolean ScreenVideoPlayable( ScreenPtr scrn, short width, short height );

//	-------------------------------------------------------------------
//	Prepares for video playback
//	width of the input (pixels)
//	height of the input (pixels)
//	codecs is the sets of codecs used by the flim
//	#### name is only passed to be displayed in errors. Errors should not be displayed this deep. Pascal string
//	#### codecs isn't used yet (remove?)
//	Returns TRUE if flim can play, FALSE if flim is not playable
//	-------------------------------------------------------------------

Boolean ScreenVideoPrepare( ScreenPtr scrn, short playback_left, short playback_top, short width, short height, unsigned long codecs, const char *name );

//	-------------------------------------------------------------------
//	Uncompress a video frame of data
//	-------------------------------------------------------------------

void ScreenUncompressFrame( ScreenPtr scrn, char *source );

#endif
