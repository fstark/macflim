#include "Playback.h"

#include "Config.h"
#include "Log.h"
#include "Keyboard.h"
#include "Screen.h"
#include "Help.h"
#include "Util.h"
#include "Preferences.h"
#include "Movie.h"

struct Playback gPlayback;

//	-------------------------------------------------------------------
//	BLOCK READING FUNCTIONS
//	-------------------------------------------------------------------
//	Functions that manages blocks
//	-------------------------------------------------------------------

//	Dual blocks for read and play

BlockPtr gBlock1;
BlockPtr gBlock2;

//	-------------------------------------------------------------------
//	Utility function for file parsing
//	-------------------------------------------------------------------

unsigned short as2( unsigned char *p );
unsigned short as2( unsigned char *p )
{
	return p[0]*256+p[1];
}

//	-------------------------------------------------------------------

unsigned long as4( unsigned char *p );
unsigned long as4( unsigned char *p )
{
	return as2(p)*65536L+as2(p+2);
}

//	-------------------------------------------------------------------
//	Skips to the next frame data ptr
//	-------------------------------------------------------------------

FrameDataPtr NextDataPtrS( FrameDataPtr ptr )
{
	return (FrameDataPtr)(((char *)ptr)+ptr->data_size+2);
}

FrameDataPtr NextDataPtrV( FrameDataPtr ptr )
{
	return (FrameDataPtr)(((char *)ptr)+ptr->data_size);
}

//	-------------------------------------------------------------------
//	Sync read a block from disk
//	-------------------------------------------------------------------


//	-------------------------------------------------------------------
//	Creates an in-memory block
//	-------------------------------------------------------------------

BlockPtr MovieAllocateBlock( MoviePtr movie )
{
	BlockPtr blk = (BlockPtr)NewPtrNoFail( sizeof(struct BlockRecord)-1+MovieGetMaxBlockSize(movie) );
	blk->status = blockUnused;
	blk->index = -1;
	blk->ticks = 0;
	blk->frames_left = 0;
	blk->sound = blk->video = NULL;
	CheckBlock( NULL, blk );
	return blk;
}

//	-------------------------------------------------------------------
//	Block deallocate
//	-------------------------------------------------------------------

void MovieDisposBlock( BlockPtr blk )
{
	DisposPtr( blk );
}

void CheckBlock( MoviePtr movie, BlockPtr blk )
{
	assert( blk->status>=blockUnused && blk->status<=blockClosed, "Block Status" );
//	assert( blk->ticks>=0, "Block Ticks" );
	assert( blk->frames_left>=0, "Block Frames Left" );
//	assert( (char*)blk->sound>=blk->buffer && (char*)blk->sound<blk->buffer+movie->maxBlockSize, "Block Sound" );
//	assert( (char*)blk->video>=blk->buffer && (char*)blk->video<blk->buffer+movie->maxBlockSize, "Block Video" );
}

long kludge;

void MovieReadBlock( MoviePtr movie, int index, BlockPtr blk )
{
	OSErr err;
	long readSize;

	assert( index>=0 && index<MovieGetBlockCount(movie), "BlockRead" );
	
	blk->index = index;
	blk->frames_left = MovieGetBlockFrameCount( movie, index );

		//	Block Will be read from disk
	blk->status = blockReading;

	readSize = MovieGetBlockSize( movie, index );
	assert( readSize<=MovieGetMaxBlockSize( movie ), "Block Size" );
	err = FSRead( MovieGetFileRefNum( movie ), &readSize, blk->buffer );
	assert( err==noErr, "FSRead" );
	assert( readSize==MovieGetBlockSize( movie, index ), "Short read" );

	blk->ticks = *(short *)(blk->buffer);
	blk->sound = (FrameDataPtr)(blk->buffer+2);
	blk->video = NextDataPtrV( blk->sound );

//	for (kludge=0;kludge!=40000;kludge++) ;

		//	Block is now ready to be played
	blk->status = blockReady;
	
	CheckBlock( movie, blk );
}

//	-------------------------------------------------------------------
//	Set the block state to playing
//	-------------------------------------------------------------------

void BlockWillPlay( BlockPtr blk );
void BlockWillPlay( BlockPtr blk )
{
	assert( blk->status==blockReady, "BlockWillPlay: block is not ready" );
	blk->status = blockPlaying;
}

//	-------------------------------------------------------------------
//	Set the block state to played
//	-------------------------------------------------------------------

void BlockDidPlay( BlockPtr blk );
void BlockDidPlay( BlockPtr blk )
{
	assert( blk->status==blockPlaying, "BlockDidPlay: block still playing" );
	blk->status = blockPlayed;
}


//	The current block getting played
BlockPtr gPlaybackBlock;

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

		exec_log();

		CheckKeys();
		if (sEscape)
		{
			return kAbort;
		}
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
				if (sHelp || sPreferences)
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
//	Returns the first buffer
//	-------------------------------------------------------------------

BlockPtr GetFirstBlock( void )
{
	return gBlock1;
}

//	-------------------------------------------------------------------
//	Returns the other buffer
//	-------------------------------------------------------------------

BlockPtr GetOtherBlock( BlockPtr blk )
{
	if (blk==gBlock1)
		return gBlock2;
	return gBlock1;
}

//	The screen
struct ScreenRecord gScreenRec;
ScreenPtr gScreen = &gScreenRec;



short gState;





//	The current block getting read
BlockPtr gReadBlock;

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
	MoviePtr movie;
	int index;

begin:
	theResult = kDone;

		//	Open flim (MFS fashion)
//	err = FSOpen( fName, vRefNum, &fRefNum );
//	FReD #### LISA, you are tearing me apart...
	{
		ParamBlockRec pb;
		pb.ioParam.ioCompletion = NULL;
		pb.ioParam.ioNamePtr = fName;
		pb.ioParam.ioVRefNum = vRefNum;
		pb.ioParam.ioVersNum = 0;
		pb.ioParam.ioPermssn = fsRdPerm;
		pb.ioParam.ioMisc = NULL;
		err = PBOpen( &pb, FALSE );
		fRefNum = pb.ioParam.ioRefNum;
	}

#ifdef VERBOSE
	printf( "%#s in %d\n", fName, vRefNum );
#endif
	CheckErr( err, "Open" );

	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return kError;
	}

//DebugStr( "\pDID OPEN FILE" );

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1024 );

#ifdef SYNCPLAY
	FlimSyncPlay( fRefNum );
	goto close;
#endif

		//	Set up screen
	gScreen = ScreenInit( gScreen, 64 );
	ScreenClear( gScreen );

		//	Open flim
	movie = MovieOpen( fRefNum, MOVIE_BUFFER_SIZE );

		//	Allocate block2
	gBlock1 = MovieAllocateBlock( movie );
	gBlock2 = MovieAllocateBlock( movie );

		//	We read the first one from disk
	gReadBlock = GetFirstBlock();
	MovieReadBlock( movie, 0, gReadBlock );
	
		//	This will be the first playback block
	gPlaybackBlock = gReadBlock;

		//	We read the second block from disk
	gReadBlock = GetOtherBlock( gReadBlock );
	MovieReadBlock( movie, 1, gReadBlock );

		//	This is the block we would love to read from
	gReadBlock = GetOtherBlock( gReadBlock );

		//	Start flim sound
	exec_log();


	if (PreferenceGetIsPlaybackVBL())
	{
		PlaybackVBLInit( &gPlayback );
	}
	else
	{
		PlaybackSoundInit( &gPlayback );
	}

	gPlayback.init();
	gPlaybackBlock->status = blockPlaying;
	gState = playingState;

	tick = TickCount();

	CheckBlock( movie, gBlock1 );
	CheckBlock( movie, gBlock2 );

	for (index=2;index!=MovieGetBlockCount( movie );index++)
	{
		dlog_str( "\n MAIN LOOP WILL WAIT\n" );
		exec_log();
		theResult = BlockWaitPlayed( gReadBlock );
		if (theResult!=kDone)
		{
			goto end;	//	User abort
		}

		dlog_str( "\n MAIN WILL READ\n" );
		exec_log();

			//	We sync load data into the next buffer into the one we just played
		MovieReadBlock( movie, index, gReadBlock );
	CheckBlock( movie, gBlock1 );
	CheckBlock( movie, gBlock2 );

			//	This is the next we would love to read from
		gReadBlock = GetOtherBlock( gReadBlock );
		exec_log();
	}

		//	Wait for first block to be played
	while (gReadBlock->status!=blockPlayed)
		;

		//	Close that block
	gReadBlock->status = blockClosed;

		//	Start last block
	theResult = BlockWaitPlayed( GetOtherBlock( gReadBlock ) );
	
		//	Wait until finished
	while (GetOtherBlock( gReadBlock )->status==blockPlaying)
		;

end:

	gState = stopRequestedState;

		//	Wait for the VBL or Sound Interrupt to acknowledge the end
	while (gState!=stoppedState)
		;

#ifdef VERBOSE
	printf( "Is VLB %s\n", PreferenceGetIsPlaybackVBL()?"YES":"NO" );
#endif

	gPlayback.dispos();

	tick = TickCount()-tick;

	MovieDisposBlock( gBlock1 );
	MovieDisposBlock( gBlock2 );

#ifdef DISPLAY_STATS

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
	err = FSClose( fRefNum );
	CheckErr( err, "FSClose" );

	if (theResult==kRestart)
		goto begin;

	return theResult;
}

