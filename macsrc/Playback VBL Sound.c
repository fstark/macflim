//	-------------------------------------------------------------------
//	VBL-driven playback with direct sound DMA writes
//	-------------------------------------------------------------------
//	Phase 3: Extract real audio from frame data
//	Writes directly to SoundBase DMA buffer (high-order byte of each word)
//	Bypasses sound driver to avoid cracking on Mac Plus/68030 systems
//	Requires single-tick (370 byte) audio frames
//	-------------------------------------------------------------------

#include "Playback.h"

#define vBase 0xefe1fe
#define vBufB 0

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <Retrace.h>

//	-------------------------------------------------------------------

#include "Screen.h"
#include "Util.h"
#include "Config.h"
#include "Machine.h"
#include "Keyboard.h"
#include "Buffer.h"

//	-------------------------------------------------------------------
//	Sound buffer constants
//	-------------------------------------------------------------------

#define SOUND_BUFFER_SIZE 370		//	370 bytes per frame at 60Hz

//	-------------------------------------------------------------------
//	Static silence buffer (initialized once at startup)
//	-------------------------------------------------------------------

static unsigned char silenceBuffer[SOUND_BUFFER_SIZE];

//	-------------------------------------------------------------------
//	The VBL task
//	We insert that in the VBL task list
//	We have saved the value of A5 in it, so we can access our globals
//	from the VBL code
//	-------------------------------------------------------------------

typedef struct MyTaskElem
{
    long myA5;
    VBLTask gTask;
};

static struct MyTaskElem taskElem;

//	-------------------------------------------------------------------
//	The nested interuption level, to count reentries
//	-------------------------------------------------------------------

static int gInter = 0;	//	Counting interruption to find re-entries

//	-------------------------------------------------------------------
//	Perf counters
//	-------------------------------------------------------------------

static long sWaitForRead = 0;
static long sAlive = 0;

//	-------------------------------------------------------------------

#define noDEBUG_VBL

//	-------------------------------------------------------------------
//	Copy audio to sound DMA buffer
//	Write to high-order byte of each word (every other byte)
//	-------------------------------------------------------------------

static void CopyToSoundDMA( unsigned char *source )
{
	Ptr soundBase = *(Ptr *)0x266;		//	SoundBase low-memory global
	int i;
	
	for (i = 0; i < SOUND_BUFFER_SIZE; i++)
	{
		soundBase[i * 2] = source[i];	//	Write to high-order byte only
	}
}

//	-------------------------------------------------------------------
//	VBL callback with direct sound DMA writes
//	-------------------------------------------------------------------

static pascal void DoFrameWithSound()
{
		//	Recover the value of A5 for access to global
		//	from the 4 bytes before the VBL entry
    asm
    {
        move.l a5,-(a7)
        move.l -4(a0),a5
    }

#ifndef MINI_PLAYER
if (gDebug)
{
	ScreenLogHome( gScreen );
	ScreenLog( gScreen, "%c VBL-SND %ld/%ld BUF=%ld", (MachineIsMinimal()?'M':' '), FreeMem(), MachineGetMemory(), BufferGetSize() );
}
#endif

	if (gState==stopRequestedState)
	{
		gState = stoppedState;
		goto end;
	}
    
		//	We want to be called next frame
		//	But don't know when yet
	taskElem.gTask.vblCount = 1;

	if (gState==pauseRequestedState)
	{
		gState = pausedState;
	}

	if (gState==pausedState)
		goto end;

#ifdef DEBUG_VBL
	ScreenLogHome( gScreen );
	ScreenLog( gScreen, "\n\n[%ld]\n", sAlive++ );
	ScreenLog( gScreen, "*%p [%d,%d=%dt,%d] \n",
							gPlaybackBlock,
							gPlaybackBlock->status,
							gPlaybackBlock->index,
							gPlaybackBlock->ticks,
							gPlaybackBlock->frames_left
							 );
	ScreenLog( gScreen, " %p [%d,%d=%dt,%d] \n",
							GetOtherBlock( gPlaybackBlock ),
							GetOtherBlock( gPlaybackBlock )->status,
							GetOtherBlock( gPlaybackBlock )->index,
							GetOtherBlock( gPlaybackBlock )->ticks,
							GetOtherBlock( gPlaybackBlock )->frames_left
							);
	ScreenLog( gScreen, "%ld \n", kludge );
#endif

	CheckBlock( NULL, gPlaybackBlock );
	CheckBlock( NULL, GetOtherBlock( gPlaybackBlock ) );

	gInter++;

	if (gInter>1)
	{
		//	We are called too soon, we are still working, we skip one tick
		taskElem.gTask.vblCount = 1;

		ScreenFlash( gScreen, 0, 10 );

		goto end;
	}
	
	//	-------------------------------------------------------------------
	//	Extract audio from block or use silence
	//	-------------------------------------------------------------------
	
	{
		unsigned char *audioSource;
		
		//	Check if sound exists and is exactly 1 tick
		if (gPlaybackBlock->sound != (FrameDataPtr)gPlaybackBlock->video && 
		    gPlaybackBlock->ticks == 1)
		{
			//	Use real sound data from block
			audioSource = gPlaybackBlock->sound->data + 6;
		}
		else
		{
			//	Use silence (no sound or multi-tick frame)
			audioSource = silenceBuffer;
		}
		
		//	-------------------------------------------------------------------
		//	Write audio directly to sound DMA buffer (minimal latency)
		//	-------------------------------------------------------------------
		
		CopyToSoundDMA( audioSource );
	}
	
	//	-------------------------------------------------------------------
	//	Now handle video frame decode and block management
	//	-------------------------------------------------------------------

	if (gPlaybackBlock->frames_left==0)
	{
			//	Last block done
		if (GetOtherBlock( gPlaybackBlock )->status==blockClosed)
		{
			gPlaybackBlock->status = blockPlayed;
			goto end;
		}
	
			//	We want to switch to the other block
		if (GetOtherBlock( gPlaybackBlock )->status==blockReady)
		{
			gPlaybackBlock->status = blockPlayed;	//	Now, the main loop can start filling this block again
			gPlaybackBlock = GetOtherBlock( gPlaybackBlock );
			gPlaybackBlock->status = blockPlaying;
		}
		else
		{
			//	Next block is not available -- we wait
			taskElem.gTask.vblCount = 1;
#ifdef DEBUG_VBL
			ScreenLog( gScreen, "VBL %ld \n", sWaitForRead++ );
#endif
			goto end;
		}
	}

	taskElem.gTask.vblCount = gPlaybackBlock->ticks;
	ScreenUncompressFrame( gScreen, (char *)gPlaybackBlock->video->data );
	gPlaybackBlock->frames_left--;
	if (gPlaybackBlock->frames_left!=0)
	{
		gPlaybackBlock->sound = NextDataPtrS( gPlaybackBlock->video );
		gPlaybackBlock->ticks = ((short *)gPlaybackBlock->sound)[-1];
		gPlaybackBlock->video = NextDataPtrV( gPlaybackBlock->sound );
	}
	else
	{
		gPlaybackBlock->sound = NULL;
		gPlaybackBlock->ticks = 1;
		gPlaybackBlock->video = NULL;
	}
end:

	gInter--;

		//	Restore A5 to the value it had before entry
    asm
    {
       move.l (a7)+,a5
    }
}

//	-------------------------------------------------------------------
//	Initialize silence buffer and enable sound
//	-------------------------------------------------------------------

static void Init( void )
{
    OSErr theError;
    Ptr soundBase;
    int i;

	//	-------------------------------------------------------------------
	//	Initialize static silence buffer
	//	-------------------------------------------------------------------

	for (i = 0; i < SOUND_BUFFER_SIZE; i++)
	{
		silenceBuffer[i] = 128;		//	DC offset (silence)
	}
	
	//	-------------------------------------------------------------------
	//	Fill sound DMA buffer with silence before enabling sound
	//	-------------------------------------------------------------------
	
	soundBase = *(Ptr *)0x266;		//	SoundBase low-memory global
	
	for (i = 0; i < SOUND_BUFFER_SIZE; i++)
	{
		soundBase[i * 2] = 128;		//	Write silence (DC offset) to high-order byte
	}
	
	//	-------------------------------------------------------------------
	//	Enable sound via VIA register
	//	BCLR #7, vBase+vBufB  (0 = sound enabled)
	//	-------------------------------------------------------------------
	
	asm
	{
		bclr.b	#7, vBase+vBufB
	}
	
	//	-------------------------------------------------------------------
	//	Install VBL task
	//	-------------------------------------------------------------------
	
    taskElem.gTask.qType = vType;
    taskElem.gTask.vblAddr = DoFrameWithSound;
    taskElem.gTask.vblCount = 1;
    taskElem.gTask.vblPhase = 0;
    taskElem.myA5 = (long)CurrentA5;

	gState = pausedState;

    theError = VInstall( (QElemPtr)&taskElem.gTask );
    assert( theError==noErr, "Failed to install VBL task" );
}

//	-------------------------------------------------------------------
//	Resume playback
//	-------------------------------------------------------------------

static void Resume( void )
{
	gState = playingState;
}

//	-------------------------------------------------------------------
//	Cleanup: disable sound and remove VBL task
//	-------------------------------------------------------------------

static void Dispos( void )
{
	OSErr theError;
	
	//	-------------------------------------------------------------------
	//	Disable sound via VIA register
	//	BSET #7, vBase+vBufB  (1 = sound disabled)
	//	-------------------------------------------------------------------
	
	asm
	{
		bset.b	#7, vBase+vBufB
	}
	
	//	-------------------------------------------------------------------
	//	Remove VBL task
	//	-------------------------------------------------------------------
	
/*
	The VBL task has already been removed because we did not reset
	the vblCount to 1 in the interruption.

	theError = VRemove( (QElemPtr)&taskElem.gTask );
	assert( theError==noErr, "Failed to remove VBL task" );
*/
}

//	-------------------------------------------------------------------
//	Registration function
//	-------------------------------------------------------------------

void PlaybackVBLSoundInit( struct Playback *playback )
{
	playback->init = Init;
	playback->resume = Resume;
	playback->dispos = Dispos;
}
