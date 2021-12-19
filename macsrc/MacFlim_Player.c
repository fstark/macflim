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


#include <stdio.h>
#include <Types.h>
#include <QuickDraw.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Memory.h>
#include <Retrace.h>
#include <Sound.h>
#include <string.h>

#include "Config.h"
#include "Keyboard.h"
#include "Playback.h"
#include "Screen.h"
#include "Log.h"
#include "Util.h"
#include "Preferences.h"
#include "Codec.h"
#include "Checksum.h"
#include "Machine.h"
#include "Buffer.h"












#define noSLOW






//	-------------------------------------------------------------------
//	Global variables
//	#### MOVE ME
//	-------------------------------------------------------------------








//	-------------------------------------------------------------------
//	Autoplays all the flims selected from the Finder in a loop
//	Returns FALSE if there were no such files
//	-------------------------------------------------------------------

Boolean AutoPlayFlims( void );
Boolean AutoPlayFlims( void )
{
	short doWhat = appOpen;
	short fileCnt = 0;
	AppFile theAppFile;
	int i;

	CountAppFiles(&doWhat,&fileCnt);

		//	No files, nothing to do
	if (fileCnt==0)
		return FALSE;

	for (;;)
	{
			//	Multiple files, we play each of them in turn
		for (i=0;i!=fileCnt;i++)
		{
			ePlayResult theResult;
			GetAppFiles( i+1, &theAppFile );
			//	If a file is cancelled, we abort everything
			//	(note: this means that a bad file will abort the looping too)
			
			theResult = PlayFlimFile( theAppFile.fName, theAppFile.vRefNum );
			if (theResult==kError || theResult==kAbort)
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
			ClrAppFiles( i+1 );
		}
	}

end:
	return TRUE;
}

//	-------------------------------------------------------------------
//	Filter function : returns TRUE if file is not a FLIM
//	-------------------------------------------------------------------

pascal Boolean FileFilter( FileParam *pbp );
pascal Boolean FileFilter( FileParam *pbp )
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
	PBClose( &pb, 0 );

	return theResult;
}

//	-------------------------------------------------------------------
//	Returns TRUE is flim file type is correct
//	-------------------------------------------------------------------

Boolean IsFlimTypeCorrect( Str255 fName, short vRefNum );
Boolean IsFlimTypeCorrect( Str255 fName, short vRefNum )
{
	FInfo fInfo;
	if (GetFInfo( fName, vRefNum, &fInfo )==noErr && fInfo.fdType=='FLIM')
		return TRUE;
	return FALSE;
}

//	-------------------------------------------------------------------
//	Sets the flim type
//	-------------------------------------------------------------------

void SetFlimTypeCreator( Str255 fName, short vRefNum );
void SetFlimTypeCreator( Str255 fName, short vRefNum )
{
	FInfo fInfo;
	if (GetFInfo( fName, vRefNum, &fInfo )==noErr)
	{
		fInfo.fdType='FLIM';
		fInfo.fdCreator='FLPL';
		SetFInfo( fName, vRefNum, &fInfo );
	}
}


//	-------------------------------------------------------------------
//	Sets flim creator if needed and accepted by user
//	Does an optional integrity check
//	Return FALSE if file should not be played
//	-------------------------------------------------------------------

Boolean SetFlimTypeCreatorIfNeeded( Str255 fName, short vRefNum );
Boolean SetFlimTypeCreatorIfNeeded( Str255 fName, short vRefNum )
{
	DialogPtr theSetTypeDialog = NULL;
	short itemHit;

	if (IsFlimTypeCorrect( fName, vRefNum ))
	{
		unsigned char theKeys[16];
		GetKeys( theKeys );
		if (TestKey( theKeys, 0x3a ))	//	Option
			return ChecksumFlimIfNeeded( fName, vRefNum );
		
		return TRUE;
	}

	theSetTypeDialog = GetNewDialog( 130, NULL, (WindowPtr)-1 );
	ShowWindow( theSetTypeDialog );
	ModalDialog( NULL, &itemHit );
	DisposDialog( theSetTypeDialog );	
	if (itemHit==1)
	{
		if (ChecksumFlimIfNeeded( fName, vRefNum ))
			SetFlimTypeCreator( fName, vRefNum );
		else
			return FALSE;
	}

	return TRUE;
}

//	-------------------------------------------------------------------
//	Main function
//	-------------------------------------------------------------------

int main()
{
	Ptr savePtr;
	OSErr err;
	SFReply theReply;
	Point where;

		//	Mac toolbox init

//	Debugger();

//	There is a whole mess around MaxApplZone()
//	It is part of the "Pascal Toolbox interface" (OSIntf)
//		(TODO: find old mac dev env. compile MaxApplZone. disass)
//	It doesn't look to be in 64K ROM
//	It is in the 128K ROM
//	Its trap# is A063
//	However, A063 is something else in old trap lists (_InitUtil in MDS1.1)

	InitGraf( &thePort );
	MachineInit();

	if (!MachineIsMinimal())
		MaxApplZone();

	BufferInit( 600000L, 30000L );

	InitFonts();
	FlushEvents( everyEvent, 0 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( NULL );
	InitUtilities();
	InitCursor();

	{
		//	As sprintf may be used in interruption for HUD
		//	libc has to be initialised once from the main code
		//	or it will crash 'cause it allocates memory at the first call
		char buffer[16];
		sprintf( buffer, "%d", 42 );
	}


	PreferenceInit();

	CodecInit();


		//	Log subsystem
	dinit_log();

	HideCursor();

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
			PlayFlimFile( theReply.fName, theReply.vRefNum );
		}
	}
	else
	{
		//	The fancy version	
		SaveScreen( &savePtr );
	
			//	If we have flims to autoplay, let's do exactly that
		if (!AutoPlayFlims())
		{
				//	Ask user for a file
			where.h = 20;
			where.v = 90;
			ShowCursor();
			SFGetFile( where, 0, FileFilter, -1, NULL, 0, &theReply );
			
			if (theReply.good)
			{
				if (SetFlimTypeCreatorIfNeeded( theReply.fName, theReply.vRefNum ))
				{
					HideCursor();
					PlayFlimFile( theReply.fName, theReply.vRefNum );
				}
			}
		}
	
	done:
		RestoreScreen( &savePtr );
	}

	ShowCursor();

	BufferDispos();

	FlushEvents( everyEvent, 0 );

	return 0;
}
