#include <Memory.h>

#include "Flim.h"
#include "Util.h"

//	-------------------------------------------------------------------
//	Each frame is composed of a tick count followed by FrameDataRecords
//	First one is sound
//	Second is video
//	...then iterates back
//	-------------------------------------------------------------------

struct FrameDataRecord
{
	unsigned short data_size;		//	Size of the data, including header
	unsigned char  data[1];			//	Data
};

//	-------------------------------------------------------------------
//	Having a pointer to a frame data, moves to the next one
//	-------------------------------------------------------------------

typedef struct FrameDataRecord *FrameDataPtr;
FrameDataPtr NextDataPtrS( FrameDataPtr ptr );
FrameDataPtr NextDataPtrV( FrameDataPtr ptr );

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
	blockClosed = 5			//	We don't use this block anymore
};

//	-----------------------------¬--------------------------------------
//	In-memory block record
//	-------------------------------------------------------------------

struct BlockRecord
{
	short status;			//	The status of this block
	short index;			//	Index in the file currently loaded in

	short ticks;			//	The current tick count
	FrameDataPtr sound;		//	The current sound
	FrameDataPtr video;		//	The current video

	short frames_left;		//	The number of frames left to play

	char buffer[1];			//	The data buffer we use
};

typedef struct BlockRecord *BlockPtr;


typedef enum
{
	kError,
	kDone,
	kAbort,
	kSkip,
	kPrevious,
	kRestart,
	kFileError,		//	File was not a FLIM
	kScreenError,	//	Cannot find a B&W screen to play
	kCodecError		//	Don't know how to decode the flim
}	ePlayResult;

extern BlockPtr gPlaybackBlock;

ePlayResult BlockWaitPlayed( BlockPtr blk );
void BlockKill( BlockPtr blk );
BlockPtr GetOtherBlock( BlockPtr blk );
BlockPtr GetFirstBlock( void );

void CheckBlock( FlimPtr flim, BlockPtr blk );




//	Global state of player
enum State
{
	playingState = 0,
	stopRequestedState,
	stoppedState,
	pauseRequestedState,
	pausedState
};

extern short gState;

extern long gSpinCount;

//	Initialize globals for playback
void InitPlayback( void );

//	#### playbackvbl (should not be visible)
//void InstallPlaybackHandler( void );
//void RemovePlaybackHandler( void );

ePlayResult PlayFlim( FlimPtr flim, Boolean silent );
ePlayResult PlayFlimFile( Str255 fName, short vRefNum, long dirID, eFileAPI api, Boolean silent );
ePlayResult PlayFlimFileLoop( Str255 fName, short vRefNum, long dirID, eFileAPI api, Boolean silent );

//void FlimSoundStop( void );
//void FlimSoundStart( void );

ePlayResult FlimSyncPlay( FlimPtr flim );

BlockPtr FlimInitBlock( FlimPtr flim, Ptr block );
void FlimDispos( FlimPtr flim );
void FlimReadBlock( FlimPtr flim, int index, BlockPtr blk );

typedef void (*PlaybackControlFunc)( void );

struct Playback
{
	//	Install the interrupt functions and start playback in paused state
	PlaybackControlFunc init;

	//	Resumes playback in paused state
	PlaybackControlFunc resume;

	//	Clean up a stopped playback
	PlaybackControlFunc dispos;
};

void PlaybackVBLInit( struct Playback *playback );
void PlaybackSoundInit( struct Playback *playback );

extern struct Playback gPlayback;


