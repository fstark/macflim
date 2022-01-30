//	-------------------------------------------------------------------
//	The simple synchronous playback, for development
//	-------------------------------------------------------------------

#include "Playback.h"

//	-------------------------------------------------------------------

#include "Config.h"
#include "Machine.h"
#include "Flim.h"
#include "Screen.h"
#include "Buffer.h"
#include "Keyboard.h"

//	-------------------------------------------------------------------
//	Plays flim synchronously, with no sound
//	Alternates disk reads and displays
//	Usefull to debug display code or measure raw performance
//	-------------------------------------------------------------------

ePlayResult FlimSyncPlay( FlimPtr flim )
{
	unsigned char *data;
	long tick;
	int index = 0;
	long frame = 0;
	int slow_count = 0;
	OSErr err;
	BlockPtr blk;
	ePlayResult theResult = kDone;
//	flim = FlimOpen( fRefNum, BufferGetSize()-sizeof( struct BlockRecord ) );
	blk = FlimInitBlock( flim, BufferGet( 0 ) );

	ScreenClear( gScreen );

	tick = TickCount();

	for (index=0;index!=FlimGetBlockCount(flim);index++)
	{
		int frame,j;
	
		FlimReadBlock( flim, index, blk );

		while (theResult==kDone && blk->frames_left>0)
		{
			CheckKeys();

			if (gEscape)
			{
				theResult = kAbort;
			}
			if (gSkip)
				theResult = kSkip;
			if (gPrevious)
				theResult=  kPrevious;
			if (gRestart)
				theResult = kRestart;

			ScreenLogHome( gScreen );
			ScreenLog( gScreen, "%d %ld %ld ", index, (long)(frame++), TickCount()-tick );
 
			if (gDebug)
			{
				ScreenLog( gScreen, "%c DBG %ld/%ld BUF=%ld", (MachineIsMinimal()?'M':' '), FreeMem(), MachineGetMemory(), BufferGetSize() );
			}

			ScreenUncompressFrame( gScreen, (char *)blk->video->data );
			blk->sound = NextDataPtrS( blk->video );
			blk->video = NextDataPtrV( blk->sound );

			blk->frames_left--;
		}
	}
	
end:

	FlimDispos( flim );

	tick = TickCount()-tick;

	return theResult;
}


