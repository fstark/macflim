//	-------------------------------------------------------------------
//	MacFlim 2 standalone player
//	-------------------------------------------------------------------
//	Features:
//		Selects a flim file
//		Plays the flim file in full screen with sound
//		Handle type/creators
//		Looping
//		next, previous, restart, abort, pause
//		Set type creator at open
//		Supports silent flims
//	Todo:
//		(none left)
//	Nice to have:
//		Single step
//		Error handling
//		Autoplay (plays data fork)
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

//	-------------------------------------------------------------------

#include "Playback.h"
#include "Screen.h"
#include "Log.h"
#include "Util.h"
#include "Preferences.h"
#include "Checksum.h"
#include "Machine.h"
#include "Buffer.h"
#include "Resources.h"
#include "Self Player.h"
#include "User Interface.h"
#include "Library.h"

//	-------------------------------------------------------------------
//	Autoplays all the flims selected from the Finder in a loop
//	Returns FALSE if there were no such files
//	-------------------------------------------------------------------

static Boolean AutoPlayFlims( void )
{
	short doWhat = appOpen;
	short fileCnt = 0;
	AppFile theAppFile;
	int i;
	Ptr savePtr;
	Boolean played = FALSE;	//	Will be TRUE if we played one flim

	CountAppFiles(&doWhat,&fileCnt);

		//	No files, nothing to do
	if (fileCnt==0)
		return FALSE;

	HideCursor();
	SaveScreen( &savePtr );

	for (;;)
	{
			//	Multiple files, we play each of them in turn
		for (i=0;i!=fileCnt;i++)
		{
			ePlayResult theResult;
			GetAppFiles( i+1, &theAppFile );
			//	If a file is cancelled, we abort everything
			//	(note: this means that a bad file will abort the looping too)
			
			theResult = PlayFlimFile( theAppFile.fName, theAppFile.vRefNum, kNoDirID, kHFS, FALSE );
			if (theResult==kError || theResult==kFileError || theResult==kScreenError || theResult==kCodecError)
				continue;
			played = TRUE;
			if (theResult==kAbort)
				goto end;
			if (theResult==kRestart)
				i = i-1;
			//	kSkip or kDone, nothing to do
			if (theResult==kPrevious)
			{
				i = i-2;
				if (i==-2)
					i = fileCnt-2;
			}
		}

			//	If we failed at every flim, we abort
		if (!played)
			break;
	}

end:
	RestoreScreen( &savePtr );
	ShowCursor();

	return TRUE;
}

//	-------------------------------------------------------------------
//	Filter function : returns TRUE if file is not a FLIM
//	-------------------------------------------------------------------

static pascal Boolean FileFilter( FileParam *pbp )
{
	OSErr err;
	short fRefNum;
	long read_size = 5;
	char buffer[522];
	static char magic[] = { 'F', 'L', 'I', 'M', 0x0a };
	Boolean theResult = TRUE;

		//	Constructing an HFS param block
	HParamBlockRec pb;
	pb.ioParam.ioNamePtr = pbp->ioNamePtr;
	pb.ioParam.ioVRefNum = pbp->ioVRefNum;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.ioParam.ioMisc = 0;
	pb.ioParam.ioVersNum = 0;
	pb.fileParam.ioDirID = CurDirStore;
	
		//	Open file
	err = PBHOpen( &pb, FALSE );
	if (err!=noErr)
	{
		return TRUE;
	}

		//	Read 5 chars
	pb.ioParam.ioReqCount = 5;
	pb.ioParam.ioPosMode = fsFromStart;
	pb.ioParam.ioPosOffset = 0;
	pb.ioParam.ioBuffer = buffer;
	err = PBRead( &pb, FALSE );
	if (err==noErr)
	{
		if (memcmp(buffer, magic, 5)==0)
			theResult = FALSE;
	}

		//	Close file
	err = PBClose( &pb, 0 );
	CheckErr( err, "PBClose" );

	return theResult;
}

//	-------------------------------------------------------------------
//	Main function
//	-------------------------------------------------------------------

int main()
{
	OSErr err;
	Point where;
	SFReply theReply;
	Boolean saveScreen;
	Size reservedSpace;

		//	Mac toolbox init

//	Debugger();

//	There is a whole mess around MaxApplZone()
//	It is part of the "Pascal Toolbox interface" (OSIntf)
//		(TODO: find old mac dev env. compile MaxApplZone. disass)
//	It doesn't look to be in 64K ROM
//	It is in the å128K ROM
//	Its trap# is A063
//	However, A063 is something else in old trap lists (_InitUtil in MDS1.1)

	InitGraf( &thePort );

	MachineInit();

	if (MachineIsMinimal())
	{
		//	Basically a MacXL or an original Mac128K
		saveScreen = FALSE;			//	Not needed due lack of multi-finder
		reservedSpace = 25000L;		//	25 (is the current minimal)
									//	This currently let us have 2x5Kb buffers on a Mac128
	}
	else
	{
		/* Normal memory usage parameters */
		/* User may be asked to increase the MultiFinder partition */
		MaxApplZone();
		saveScreen = TRUE;
		reservedSpace = 30000L + GetScreenSaveSize();
	}

	//	Must be done before preferences are openend
	PreferenceInit();

	InitFonts();
	FlushEvents( everyEvent, 0 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( NULL );
	InitUtilities();
	BufferInit( PreferenceGetMaxBufferSize(), reservedSpace );
	InitCursor();

	{
		//	As sprintf may be used in interruption for HUD
		//	libc has to be initialised once from the main code
		//	or it will crash 'cause it allocates memory at the first call
		char buffer[16];
		sprintf( buffer, "%d", 42 );
	}

	CodecInit();

		//	Log subsystem
	dinit_log();

		//SelfInstallPlayerUI();

//	HideCursor();

	//	Set up screen
	gScreen = ScreenInit( gScreen );
//	ScreenVideoPrepare( gScreen, 512, 342 );

	if (!MachineIsBlackAndWhite())
	{
		Alert( kALRTErrorNoBWScreen, NULL );
		return 0;
	}

	if (MachineIsMinimal())
	{
		//	In the minimal version, we don't open files from Finder
		//	we don't set the type/creator either
		//	we don't save the screen content
		//	we just display the open file panel and play the flim

			//	Ask user for a file
		where.h = 20;
		where.v = 90;
		ShowCursor();
		
		SFGetFile( where, 0, NULL, -1, NULL, 0, &theReply );
		
		if (theReply.good)
		{
			HideCursor();
			PlayFlimFileLoop( theReply.fName, theReply.vRefNum, kNoDirID, kMFS, FALSE );
		}
		//	There is no multi-finder, so we don't care about the screen state
	}
	else
	{
		UserInterfaceInit();
		if (!AutoPlayFlims())
			UserInterfaceLoop();
	}

	ShowCursor();

	BufferDispos();

	FlushEvents( everyEvent, 0 );

	return 0;
}
