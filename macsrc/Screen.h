#ifndef SCREEN_INCLUDED__
#define SCREEN_INCLUDED__

#include "Codec.h"

//	-------------------------------------------------------------------
//	A "Screen" suitable for playback
//	-------------------------------------------------------------------

struct ScreenRecord
{
	char *physAddr;		//	Real physical top of screen
	short height;		//	Number of lines

	char *baseAddr;		//	Base addr of start of image (may be in the middle of the physical screen)
	short rowBytes;	//	Number of bytes between a line and the next
	short stride4;		//	Number of longs from the end of a line to the beginning of the next
						//	stride4*4+64 == rowBytes

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
//	If silent, will use the silent routines
//	Rowbytes are the rowbytes of the original flim (64 bytes)
//	-------------------------------------------------------------------

ScreenPtr ScreenInit( ScreenPtr scrn, short rowbytes );

//	-------------------------------------------------------------------
//	Clears screen to black
//	-------------------------------------------------------------------

void ScreenClear( ScreenPtr scrn );

//	-------------------------------------------------------------------
//	Flashes a subset of the lines (useful for debugging)
//	-------------------------------------------------------------------

void ScreenFlash( ScreenPtr scrn, short from, short lines );

//	-------------------------------------------------------------------
//	Uncompress a video frame of data
//	-------------------------------------------------------------------

void ScreenUncompressFrame( ScreenPtr scrn, char *source );

#endif
