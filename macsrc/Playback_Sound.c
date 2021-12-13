//	-------------------------------------------------------------------
//	The callback for noisy playback driven by sound interrupts
//	-------------------------------------------------------------------

#include "Playback.h"

#include <Sound.h>

#include "Log.h"
#include "Keyboard.h"
#include "Screen.h"

//	Silence
FFSynthPtr silence;

//	The param block used to send work to the sound driver

ParamBlockRec pb;

//	-------------------------------------------------------------------
//	Generates a tick of silence
//	-------------------------------------------------------------------

static void DoSilence( ProcPtr callback )
{
	OSErr sound_err;
	assert( silence!=NULL, "Silence buffer unallocated" );
	pb.ioParam.ioRefNum = -4;
	pb.ioParam.ioBuffer = (Ptr)silence;
	pb.ioParam.ioReqCount = 370;
	pb.ioParam.ioCompletion = callback;
	sound_err = PBWrite(&pb, TRUE);
	assert( sound_err==noErr, "DoSilence: PBWrite failed" );
}



//	The nested interuption level, to count reentries
static int gInter = 0;


//	Per counters
long gPlayedFrames = 0;
long gReentryCount = 0;
long gStalledTicks = 0;
long gSpinCount = 0;



//	-------------------------------------------------------------------
//	Callback to play a frame of sound and display a frame of video
//	-------------------------------------------------------------------

pascal void DoFrame( void );
pascal void DoFrame()
{
	OSErr err;

		//	Find globals for A5
	asm
	{
		move.l a5, -(a7)
		move.l 0x904, a5
	}

#ifdef VERBOSE
	dlog_str( "CALLBACK #" );
	dlog_int( gPlaybackBlock->frames_left );
	dlog_str( " " );
#endif

	ScreenLogHome( gScreen );
	ScreenLog( gScreen, "SND" );

	if (gState==pauseRequestedState)
	{
		gState=pausedState;
	}

	if (gState==pausedState)
	{
	    DoSilence( (ProcPtr)DoFrame );
		goto end;
	}

	if (gInter==1)
	{
		gReentryCount++;
	    DoSilence( (ProcPtr)DoFrame );
		goto end;
	}

		//	If request to stop, do not play sound, no more callbacks, and notify new state
	if (gState==stopRequestedState)
	{
		gState = stoppedState;
		goto end;	//	So? Sue me.
	}

		//	We have displayed all the frames from this block
		//	We can now release the block
	if (gPlaybackBlock->frames_left==0)
	{
			//	We want to switch to the other block
		if (GetOtherBlock( gPlaybackBlock )->status==blockReady)
		{
#ifdef VERBOSE
			dlog_str( "\nSWITCHING " );
#endif
			gPlaybackBlock->status = blockPlayed;	//	Now, the main loop can start filling this block again
			gPlaybackBlock = GetOtherBlock( gPlaybackBlock );
			gPlaybackBlock->status = blockPlaying;
		}
		else
		{
#ifdef VERBOSE
			dlog_str( " STALLED " );
#endif
			//	We have no available blocks
			//	We don't have enough resources
			DoSilence( (ProcPtr)DoFrame );
			gStalledTicks++;
			goto end;
		}
	}

		//	Play one frame of sound
	gPlayedFrames++;

	gInter++;

	pb.ioParam.ioRefNum = -4;
	
	{
		Ptr audio = (Ptr)gPlaybackBlock->sound->data;
		pb.ioParam.ioReqCount = gPlaybackBlock->sound->data_size-8 /* size+header */;
		pb.ioParam.ioCompletion = (ProcPtr)DoFrame;

		if (sMuted)
		{
			audio = (Ptr)silence;
		}
		if (audio==(Ptr)gPlaybackBlock->video)
		{	//	No sound
			audio = (Ptr)silence;
			pb.ioParam.ioReqCount = 370*gPlaybackBlock->ticks;
		}
		pb.ioParam.ioBuffer = audio;
	}


/*	if (muted)		//	more than 5 ticks of silence are needed...
	{
		register int i;
		register int c = pb.ioParam.ioReqCount;
		register char *p = pb.ioParam.ioBuffer+6;
		for (i=0;i!=c;i++)
			*p++ = 128;
	}
*/
	err = PBWrite(&pb, TRUE);
	ScreenUncompressFrame( gScreen, (char *)gPlaybackBlock->video->data );

		//	Compute next frame
	gPlaybackBlock->frames_left--;
	gPlaybackBlock->sound = NextDataPtrS( gPlaybackBlock->video );
	gPlaybackBlock->ticks = ((short *)gPlaybackBlock->sound)[-1];
	gPlaybackBlock->video = NextDataPtrV( gPlaybackBlock->sound );

	gInter--;

end:

		//	Restore previous A5
	asm
	{
		move.l (a7)+, a5
	}
}







//	-------------------------------------------------------------------
//	Starts sound
//	-------------------------------------------------------------------

static void Init( void )
{
	long i;
	int n = 0;
	int k = 1;

	assert( silence==NULL, "Silence already allocated" );
	silence = (FFSynthPtr)NewPtr(2+4+370*12);
	assert( silence!=NULL, "Silence allocation failed" );
	silence->mode = ffMode;
	silence->count = FixRatio(1,1);
	for (i=0;i!=370*12;i++)
		silence->waveBytes[i] = 128;

//	gPlaybackBlock->status = blockPlaying;	//	unsure

	gState = pausedState;

	DoFrame();
}

//	-------------------------------------------------------------------
//	Stops the flim
//	-------------------------------------------------------------------

static void Restart( void )
{
	gState = pausedState;
	DoFrame();
}

//	-------------------------------------------------------------------
//	Stops the flim
//	-------------------------------------------------------------------

static void Dispos( void )
{
//	Does not work. Unsure why
//	KillIO(-4);  /* stops all pending StartSounds, see Sound Manager in IM2 */

	assert( silence!=NULL, "Silence unallocated1" );
	DisposPtr( (Ptr)silence );
	silence = NULL;
}


/*void PlaybackStop()
{
	gState = stopRequestedState;
	while (gState!=stoppedState)
		;
}*/

void PlaybackSoundInit( struct Playback *playback )
{
	playback->init = Init;
	playback->restart = Restart;
	playback->dispos = Dispos;
}
