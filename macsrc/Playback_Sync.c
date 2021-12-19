//	-------------------------------------------------------------------
//	The simple synchronous playback, for development
//	-------------------------------------------------------------------

#include "Playback.h"

#include "Config.h"
#include "Machine.h"
#include "Movie.h"
#include "Screen.h"
#include "Buffer.h"
#include "Keyboard.h"

//	-------------------------------------------------------------------
//	Plays movie synchronously, with no sound
//	Alternates disk reads and displays
//	Usefull to debug display code or measure raw performance
//	-------------------------------------------------------------------

ePlayResult FlimSyncPlay( short fRefNum )
{
	unsigned char *data;
	long tick;
	int index = 0;
	int slow_count = 0;
	OSErr err;
	BlockPtr blk;
	MoviePtr movie;
	ePlayResult theResult = kDone;
	movie = MovieOpen( fRefNum, BufferGetSize()-sizeof( struct BlockRecord ) );
	blk = MovieInitBlock( movie, BufferGet( 0 ) );

	gScreen = ScreenInit( gScreen, 64 );
//	ScreenClear( gScreen );

	tick = TickCount();

	for (index=0;index!=MovieGetBlockCount(movie);index++)
	{
		int frame,j;
	
		MovieReadBlock( movie, index, blk );

		while (theResult==kDone && blk->frames_left>0)
		{
			CheckKeys();

			if (sEscape)
			{
				theResult = kAbort;
			}
			if (sSkip)
				theResult = kSkip;
			if (sPrevious)
				theResult=  kPrevious;
			if (sRestart)
				theResult = kRestart;

			if (sDebug)
			{
				ScreenLogHome( gScreen );
				ScreenLog( gScreen, "%c DBG %ld/%ld BUF=%ld", (MachineIsMinimal()?'M':' '), FreeMem(), MachineGetMemory(), BufferGetSize() );
			}

			ScreenUncompressFrame( gScreen, (char *)blk->video->data );
			blk->sound = NextDataPtrS( blk->video );
			blk->video = NextDataPtrV( blk->sound );

			blk->frames_left--;
		}
	}
	
end:

	MovieDispos( movie );

	tick = TickCount()-tick;

//	while (!Button())
//		;
//	while (Button())
//		;
//	
//	printf( "Ticks = %ld\n", tick );
//
//
//	while (!Button())
//		;

	return theResult;
}


