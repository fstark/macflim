#include "Playback.h"

#include <Retrace.h>
#include "Screen.h"
#include "Log.h"

//	-------------------------------------------------------------------
//	The VBL task for silent playback, driven by vertical retrace
//	-------------------------------------------------------------------

//	We insert that in the VBL task list
//	We have saved the value of A5 in it, so we can access our globals
//	from the VBL code
typedef struct MyTaskElem
{
    long myA5;
    VBLTask gTask;
};

//	The VBL task
struct MyTaskElem taskElem;

static int gInter = 0;	//	Counting interruption to find re-entries

extern long kludge;

static long sWaitForRead = 0;
static long sAlive = 0;

#define noDEBUG_VBL

static pascal void DoFrameSilent()
{
		//	Recover the value of A5 for access to global
		//	from the 4 bytes before the VBL entry
    asm
    {
        move.l A5,-(SP)
        move.l -4(A0),A5
    }

	if (gState==stopRequestedState)
	{
//	ScreenLogHome( gScreen );
//	ScreenLog( gScreen, "STOP REQUESTED" );
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
	
//	assert( gPlaybackBlock->ticks!=0, "Playback ticks" );

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
       move.l (SP)+,A5
    }
}

//	-------------------------------------------------------------------
//	Installs VBL Handler
//	-------------------------------------------------------------------

static void Init( void )
{
    OSErr theError;
    taskElem.gTask.qType = vType;
    taskElem.gTask.vblAddr = DoFrameSilent;
    taskElem.gTask.vblCount = 1;
    taskElem.gTask.vblPhase = 0;
    taskElem.myA5 = (long)CurrentA5;

	gState = pausedState;

    theError = VInstall( (QElemPtr)&taskElem.gTask );
    assert( theError==noErr, "Failed to install VBL task" );
}

//	-------------------------------------------------------------------
//	Removes VBL Handler
//	-------------------------------------------------------------------

static void None( void )
{
	//	The task was removed by the gStoppedState (keeping vblCount to 0)
	//	The remove VBL should be used if the task is still pending
/*
    OSErr theError;
    theError = VRemove( (QElemPtr)&taskElem.gTask );
    assert( theError==noErr, "Failed to remove VBL task" );
*/
}

void PlaybackVBLInit( struct Playback *playback )
{
	playback->init = Init;
	playback->restart = Init;
	playback->dispos = None;
}
