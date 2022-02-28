#include "Playback.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Config.h"
#include "Machine.h"
#include "Log.h"
#include "Keyboard.h"
#include "Screen.h"
#include "Util.h"
#include "Preferences.h"
#include "Flim.h"
#include "Buffer.h"

#include <stdio.h>

//	-------------------------------------------------------------------

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

BlockPtr FlimInitBlock( FlimPtr flim, Ptr block )
{
	BlockPtr blk = (BlockPtr)block;
	blk->status = blockUnused;
	blk->index = -1;
	blk->ticks = 0;
	blk->frames_left = 0;
	blk->sound = blk->video = NULL;
	CheckBlock( NULL, blk );
	return blk;
}

void CheckBlock( FlimPtr flim, BlockPtr blk )
{
	assert( blk->status>=blockUnused && blk->status<=blockClosed, "Block Status" );
//	assert( blk->ticks>=0, "Block Ticks" );
	assert( blk->frames_left>=0, "Block Frames Left" );
//	assert( (char*)blk->sound>=blk->buffer && (char*)blk->sound<blk->buffer+flim->maxBlockSize, "Block Sound" );
//	assert( (char*)blk->video>=blk->buffer && (char*)blk->video<blk->buffer+flim->maxBlockSize, "Block Video" );
}

long kludge;

void FlimReadBlock( FlimPtr flim, int index, BlockPtr blk )
{
	OSErr err;
	long readSize;

#ifdef VERBOSE
	printf( "FlimReadBlock(%d)\n", index);
#endif

	assert( index>=0 && index<FlimGetBlockCount( flim ), "BlockRead" );
	
	blk->index = index;
	blk->frames_left = FlimGetBlockFrameCount( flim, index );

		//	Block Will be read from disk
	blk->status = blockReading;

	readSize = FlimGetBlockSize( flim, index );
	err = FSRead( FlimGetFileRefNum( flim ), &readSize, blk->buffer );
	assert( err==noErr, "FSRead" );
	assert( readSize==FlimGetBlockSize( flim, index ), "Short read" );

	blk->ticks = *(short *)(blk->buffer);
	blk->sound = (FrameDataPtr)(blk->buffer+2);
	blk->video = NextDataPtrV( blk->sound );

//	for (kludge=0;kludge!=40000;kludge++) ;

		//	Block is now ready to be played
	blk->status = blockReady;
	
	CheckBlock( flim, blk );
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

#ifdef VERBOSE
	printf( "BlockWaitPlayed(%d)\n", blk->index);
#endif

	do
	{
		status = blk->status;
		assert( status==blockPlaying || status==blockPlayed, "BlockWaitPlayed: wrong status" );

		exec_log();

#ifndef MINI_PLAYER
		CheckKeys();
		if (gEscape)
		{
			return kAbort;
		}
		if (gSkip)
			return kSkip;
		if (gPrevious)
			return kPrevious;
		if (gRestart)
			return kRestart;

		if (gPause)
		{
			enum State state = gState;

			gState = pausedState;

			do
			{
				CheckKeys();
				if (gEscape)
					return kAbort;
				if (gSkip)
					return kSkip;
				if (gPrevious)
					return kPrevious;
				if (gRestart)
					return kRestart;
			}
			while (!gPause);

            gState = playingState;
		}

		gSpinCount++;
#else
		if (Button())
			return kAbort;
#endif MINI_PLAYER

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


static Boolean TestKey( unsigned char *keys, char k )
{
	return !!((keys[k>>3]>>(k&7))&1);
}

//	-------------------------------------------------------------------
//	Plays a file
//	-------------------------------------------------------------------

ePlayResult PlayFlim( FlimPtr flim, Boolean silent )
{
	OSErr err;
	short fRefNum;
	long read_size;
	long next_read_size;
	long tick = 0;
	ePlayResult theResult;
	int index;
	struct FlimInfo *flimInfo;

#ifdef SYNCPLAY
	return FlimSyncPlay( flim );
#endif

	if (!MachineIsBlackAndWhite())
		return kScreenError;

	flimInfo = FlimGetInfo( flim );
	if (!ScreenVideoPrepare( gScreen, flimInfo->width, flimInfo->height ))
		return kCodecError;

	//	Hack to check the buffer size
	{
		unsigned char theKeys[16];

		GetKeys( theKeys );
		if (TestKey( theKeys, 0x3a ))	//	Options
		{
			MessageLong( BufferGetSize() );
		}
	}

	theResult = kDone;

	ScreenClear( gScreen );

		//	Start of flim
	FlimSeekStart( flim );

		//	Allocate block2
	gBlock1 = FlimInitBlock( flim, BufferGet(0) );
	gBlock2 = FlimInitBlock( flim, BufferGet(1) );

		//	We read the first one from disk
	dlog_str( "\nFlimReadBlock(0)\n" );
	exec_log();
	gReadBlock = GetFirstBlock();
	FlimReadBlock( flim, 0, gReadBlock );
	
		//	This will be the first playback block
	gPlaybackBlock = gReadBlock;

		//	We read the second block from disk
	gReadBlock = GetOtherBlock( gReadBlock );
	gReadBlock->status = blockPlayed;

		//	Start flim sound
	exec_log();


	if ( silent ||
#ifndef MINI_PLAYER
	PreferenceGetIsPlaybackVBL() ||
#endif
	FlimGetIsSilent( flim ))
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

	CheckBlock( flim, gBlock1 );
	CheckBlock( flim, gBlock2 );

	for (index=1;index!=FlimGetBlockCount( flim );index++)
	{
			//	Goal is to load the block 'index' from the disk into gReadBlock and start playing it
	
			//	We sync load the data
		dlog_str( "\nFlimReadBlock(index)\n" );
		exec_log();
		FlimReadBlock( flim, index, gReadBlock );
		CheckBlock( flim, gBlock1 );
		CheckBlock( flim, gBlock2 );

			//	We switch to the next one
		gReadBlock = GetOtherBlock( gReadBlock );
		exec_log();

		dlog_str( "\n MAIN LOOP WILL WAIT\n" );
		exec_log();

			//	We wait for that 'next one' to be available
		dlog_str( "\nBlockWaitPlayed(index-1)\n" );
		exec_log();
		theResult = BlockWaitPlayed( gReadBlock );
		if (theResult!=kDone)
		{
			goto end;	//	User abort
		}

		dlog_str( "\n MAIN WILL READ\n" );
		exec_log();
	}

	dlog_str( "\n Wait for last block\n" );
	exec_log();

		//	Wait for last block
	gReadBlock->status = blockClosed;	//	So the interrupt passes the block to played
	theResult = BlockWaitPlayed( GetOtherBlock( gReadBlock ) );
	
end:

	dlog_str( "\n Wait for stop\n" );
	exec_log();

	gState = stopRequestedState;

		//	Wait for the VBL or Sound Interrupt to acknowledge the end
	while (gState!=stoppedState)
		;

	dlog_str( "\n Stopped\n" );
	exec_log();


#ifdef VERBOSE
	printf( "Is VLB %s\n", PreferenceGetIsPlaybackVBL()?"YES":"NO" );
#endif

	gPlayback.dispos();

	tick = TickCount()-tick;

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

	return theResult;
}

ePlayResult PlayFlimFile( Str255 fName, short vRefNum, long dirID, eFileAPI api, Boolean silent )
{
		//	Open flim
	FlimPtr flim = FlimOpenByName( fName, vRefNum, dirID, api );
	ePlayResult theResult;

	if (!flim)
		return kFileError;

	do
	{
#ifdef SYNCPLAY
		theResult = FlimSyncPlay( flim );
		goto close;
#endif

		theResult = PlayFlim( flim, silent );
	}
	while (theResult==kRestart);

close:
	FlimDispos( flim );

	return theResult;
}

//	-------------------------------------------------------------------
//	Plays a file in a loop
//	-------------------------------------------------------------------

ePlayResult PlayFlimFileLoop( Str255 fName, short vRefNum, long dirID, eFileAPI api, Boolean silent )
{
	ePlayResult theResult;

	do
	{
		theResult = PlayFlimFile( fName, vRefNum, dirID, api, silent );
	} while (theResult==kDone);		//	Loop over if no button click

	return theResult;
}
