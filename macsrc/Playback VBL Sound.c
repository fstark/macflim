//	-------------------------------------------------------------------
//	VBL-driven playback with direct sound DMA writes
//	-------------------------------------------------------------------
//	Phase 1 & 2: Test with rotating 440Hz/880Hz/1320Hz tones
//	Writes directly to SoundBase DMA buffer (high-order byte of each word)
//	Bypasses sound driver to avoid cracking on Mac Plus/68030 systems
//	-------------------------------------------------------------------

#include "Playback.h"
#include <math.h>

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
#define PI 3.14159265358979323846

//	-------------------------------------------------------------------
//	Test tone buffers (440Hz, 880Hz, 1320Hz)
//	-------------------------------------------------------------------

static unsigned char *tone440 = NULL;
static unsigned char *tone880 = NULL;
static unsigned char *tone1320 = NULL;

//	-------------------------------------------------------------------
//	Frame counter for tone rotation (20 frames = 1/3 second per tone)
//	-------------------------------------------------------------------

static long gFrameCounter = 0;

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
//	Generate a sine wave tone buffer
//	-------------------------------------------------------------------

static void GenerateTone( unsigned char *buffer, int frequency )
{
	int i;
	double sampleRate = 22200.0;		//	Close enough to 22254.54
	
	for (i = 0; i < SOUND_BUFFER_SIZE; i++)
	{
		double phase = 2.0 * PI * frequency * i / sampleRate;
		double sample = 128.0 + 127.0 * sin(phase);
		buffer[i] = (unsigned char)sample;
	}
}

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
	unsigned char *currentTone;
	int toneIndex;
	
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
	//	PHASE 2: Rotate through test tones every 20 frames (1/3 second)
	//	-------------------------------------------------------------------
	
	gFrameCounter++;
	toneIndex = (gFrameCounter / 20) % 3;
	
	if (toneIndex == 0)
		currentTone = tone440;
	else if (toneIndex == 1)
		currentTone = tone880;
	else
		currentTone = tone1320;
	
	//	-------------------------------------------------------------------
	//	Write audio directly to sound DMA buffer (minimal latency)
	//	-------------------------------------------------------------------
	
	CopyToSoundDMA( currentTone );
	
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
//	Initialize tone buffers and enable sound
//	-------------------------------------------------------------------

static void Init( void )
{
    OSErr theError;
    Ptr soundBase;
    int i;

	//	-------------------------------------------------------------------
	//	Allocate and generate test tone buffers
	//	-------------------------------------------------------------------

	assert( tone440==NULL, "Tone buffers already allocated" );
	
	tone440 = (unsigned char *)MyNewPtr(SOUND_BUFFER_SIZE);
	tone880 = (unsigned char *)MyNewPtr(SOUND_BUFFER_SIZE);
	tone1320 = (unsigned char *)MyNewPtr(SOUND_BUFFER_SIZE);
	
	assert( tone440!=NULL && tone880!=NULL && tone1320!=NULL, "Tone buffer allocation failed" );
	
	GenerateTone( tone440, 440 );
	GenerateTone( tone880, 880 );
	GenerateTone( tone1320, 1320 );
	
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
	gFrameCounter = 0;

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
	
	//	-------------------------------------------------------------------
	//	Free tone buffers
	//	-------------------------------------------------------------------
	
	if (tone440 != NULL)
	{
		MyDisposPtr( tone440 );
		tone440 = NULL;
	}
	
	if (tone880 != NULL)
	{
		MyDisposPtr( tone880 );
		tone880 = NULL;
	}
	
	if (tone1320 != NULL)
	{
		MyDisposPtr( tone1320 );
		tone1320 = NULL;
	}
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
