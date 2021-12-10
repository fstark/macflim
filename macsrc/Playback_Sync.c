//	-------------------------------------------------------------------
//	The simple synchronous playback, for development
//	-------------------------------------------------------------------

#include "Playback.h"

#include "Config.h"
#include "Movie.h"
#include "Screen.h"

//	-------------------------------------------------------------------
//	Plays movie synchronously, with no sound
//	Alternates disk reads and displays
//	Usefull to debug display code or measure raw performance
//	-------------------------------------------------------------------

void FlimSyncPlay( short fRefNum )
{
	unsigned char *data;
	long tick;
	int index = 0;
	int slow_count = 0;
	OSErr err;
	BlockPtr blk;
	MoviePtr movie;

	movie = MovieOpen( fRefNum, MOVIE_BUFFER_SIZE );
	blk = MovieAllocateBlock( movie );

	gScreen = ScreenInit( gScreen, 64 );
//	ScreenClear( gScreen );

	tick = TickCount();

	for (index=0;index!=MovieGetBlockCount(movie);index++)
	{
		int frame,j;
	
		MovieReadBlock( movie, index, blk );

		if (Button())
			goto end;
		
		while (blk->frames_left>0)
		{
			ScreenUncompressFrame( gScreen, (char *)blk->video->data );
			blk->sound = NextDataPtrS( blk->video );
			blk->video = NextDataPtrV( blk->sound );

			blk->frames_left--;
		}
	}
	
end:

	MovieDisposBlock( blk );
	MovieDispos( movie );

	tick = TickCount()-tick;

	while (!Button())
		;
	while (Button())
		;
	
	printf( "Ticks = %ld\n", tick );


	while (!Button())
		;
}


