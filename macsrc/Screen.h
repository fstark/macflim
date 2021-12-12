#include <stdio.h>

void SaveScreen( Ptr *ptr );
void RestoreScreen( Ptr *ptr );


#include "Codec.h"



//	-------------------------------------------------------------------
//	A "Screen" suitable for playback
//	-------------------------------------------------------------------

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

extern ScreenPtr gScreen;

void ScreenLogHome( ScreenPtr screen );
void ScreenLogMoveTo( ScreenPtr screen, int x, int y );
void ScreenLogString( ScreenPtr screen, const char *s );
void ScreenLog( ScreenPtr screen, const char *format, ... );

//	-------------------------------------------------------------------
//	Initialize a screen structure for playing on the current screen
//	If silent, will use the silent routines
//	Rowbytes are the rowbytes of the original flim (64 bytes)
//	-------------------------------------------------------------------

ScreenPtr ScreenInit( ScreenPtr scrn, size_t rowbytes );

void ScreenClear( ScreenPtr scrn );
void ScreenFlash( ScreenPtr scrn, size_t from, size_t lines );
void ScreenUncompressFrame( ScreenPtr scrn, char *source );

//	Displays an internal kill screen with the killcode
//	Clicking the Button will exit the application
void KillScreen( ScreenPtr scrn, short killcode );
