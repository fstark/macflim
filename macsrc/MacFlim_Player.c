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
#include "Render.h"


//	-------------------------------------------------------------------
//	GENERAL LOW LEVEL ROUTINES/MACROS
//	-------------------------------------------------------------------
//	Some re-implementation/variations of C library functions
//	and general macros
//	-------------------------------------------------------------------




//	-------------------------------------------------------------------
//	Concatenates a Pascal String and a C String
//	-------------------------------------------------------------------

void StrCatPC( Str255 p, const char *q );
void StrCatPC( Str255 p, const char *q )
{
	memcpy( p+p[0]+1, q, strlen( q ) );
	p[0] += strlen( q );
}









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
//	Returns TRUE is flim file contains a checksum
//	-------------------------------------------------------------------

unsigned short FlimChecksum( Str255 fName, short vRefNum );
unsigned short FlimChecksum( Str255 fName, short vRefNum )
{
	unsigned fletcher = 0xffff;
	OSErr err;
	short fRefNum;

		//	Open flim
	err = FSOpen( fName, vRefNum, &fRefNum );
	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return;
	}

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1022 );
	if (err==noErr)
	{
			//	Read Fletcher-16
		long read_size = 2;
		FSRead( fRefNum, &read_size, &fletcher );
	}


	FSClose( fRefNum );
	
	return fletcher;
}

//	-------------------------------------------------------------------
//	Reference implementation of fletcher16 checksum
//	Adds all words, modulo 65535
//	-------------------------------------------------------------------

unsigned short fletcher16_ref( register unsigned short fletcher, register unsigned short *buffer, long count );
unsigned short fletcher16_ref( register unsigned short fletcher, register unsigned short *buffer, long count )
{
	register long i;
	register long f = fletcher;

	for (i=0;i!=count;i++)
	{
		f += buffer[i];
		if (f>=65535)
			f -= 65535;
	}
	
	return f;
}

//	-------------------------------------------------------------------
//	Assembly implemetation of fletcher16
//	Works on buffer of 1 to 65536 words (pass 0 for count)
//	-------------------------------------------------------------------

unsigned short fletcher16_short( unsigned short fletcher, unsigned short *buffer, unsigned short count );
unsigned short fletcher16_short( unsigned short fletcher, unsigned short *buffer, unsigned short count )
{
	asm{
	
		move.w fletcher,d0
		movea.l buffer,a0
		move.w count,d1
		subq.w #1,d1

@loop:
		add.w (a0)+,d0
		bcc @skip
		addq.w #1,d0
@skip:
		dbra d1,@loop
		move.w d0,fletcher
	}
	

	return fletcher;
}

//	-------------------------------------------------------------------
//	Wrapper around assembly version of fletcher
//	-------------------------------------------------------------------

unsigned short fletcher16( unsigned short fletcher, unsigned short *buffer, long count );
unsigned short fletcher16( unsigned short fletcher, unsigned short *buffer, long count )
{
	while (count>65536	)
	{
		fletcher = fletcher16_short( fletcher, buffer, 65536 );	//	Note: 65536 is passed as 0
		count -= 65536;
		buffer += 65536;
	}
	return fletcher16_short( fletcher, buffer, count );
}

//	-------------------------------------------------------------------
//	Performs the integrity check, with skippable dialog
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrity( Str255 fName, short vRefNum, unsigned short fletcher_ref );
Boolean CheckFlimIntegrity( Str255 fName, short vRefNum, unsigned short fletcher_ref )
{
	OSErr err;
	short fRefNum;
	long read_size;
	unsigned short *theBuffer = NULL;
	unsigned short fletcher;
	long total_size = 0;
	long size = 1024;	//	The header we skip
	int percent;
	int last_percent = -1;
	DialogPtr theProgress = NULL;
	Handle thePercentText;
	short dummy0;
	Rect dummy1;
	Boolean skip = false;

	//	Check integrity by 20K blocks
#define BUFFER_SIZE 20000

#define kProgressDlog 129
#define kProgressItem 1

		//	Open flim
	err = FSOpen( fName, vRefNum, &fRefNum );
	if (err!=noErr)
	{
		printf( "OPEN ERROR=%d\n", err );
		return;
	}

		//	Get file size
	err = GetEOF( fRefNum, &total_size );

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1024 );

	fletcher = 0;
	
	while (Button())
		;

	theProgress = GetNewDialog( kProgressDlog, NULL, (WindowPtr)-1 );
	GetDItem( theProgress, kProgressItem, &dummy0, &thePercentText, &dummy1 );
	SetIText( thePercentText, "" );
	ShowWindow( theProgress );
	DrawDialog( theProgress );

	//	Allocate late, to reduce memory fragmentation
	theBuffer = (unsigned short *)NewPtr( BUFFER_SIZE );

	while (1)
	{
		register long i;
	
		read_size = BUFFER_SIZE;
		err = FSRead( fRefNum, &read_size, theBuffer );
		if (err!=noErr && err!=eofErr)
		{
			printf( "READ ERROR=%d\n", err );
			goto done;
		}

		size += read_size;

		if (read_size!=BUFFER_SIZE)	//	Truncate last byte is needed
			read_size -= read_size&1;

		percent = size*100.0/total_size;
		if (percent!=last_percent)
		{
			Str255 theBuffer;
			last_percent = percent;
			NumToString( percent, theBuffer );
			StrCatPC( theBuffer, "% complete" );
			SetIText( thePercentText, theBuffer );
		}

		fletcher = fletcher16( fletcher, theBuffer, read_size/2 );

		if (err==eofErr)
			break;
			
		if (Button())
		{
			skip= true;
			break;
		}
	};

done:
	FSClose( fRefNum );

	if (theBuffer) DisposPtr( theBuffer );

	if (theProgress) DisposDialog( theProgress );

	if (!skip)
	{
		if (fletcher_ref==0xffff)
		{
			ShowCursor();
			ParamText( "\pFlim have no checksum", "", "", "" );
			NoteAlert( 130, NULL );
			HideCursor();
		}
		else if (fletcher!=fletcher_ref)
		{
			ShowCursor();
			ParamText( "\pFlim is corrupted", "", "", "" );
			StopAlert( 131, NULL );
			HideCursor();
	
			return FALSE;
		}
	}

	return TRUE;
}

//	-------------------------------------------------------------------
//	Checks flim integrity if possible and accepted by user
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrityIfNeeded( Str255 fName, short vRefNum );
Boolean CheckFlimIntegrityIfNeeded( Str255 fName, short vRefNum )
{
	unsigned short checksum = FlimChecksum( fName, vRefNum );
	if (checksum!=0xffff)
	{
		DialogPtr theCheckDialog = NULL;
		short itemHit;

		theCheckDialog = GetNewDialog( 131, NULL, (WindowPtr)-1 );
		ShowWindow( theCheckDialog );
		ModalDialog( NULL, &itemHit );
		DisposDialog( theCheckDialog );
		if (itemHit==2)
		{
			if (!CheckFlimIntegrity( fName, vRefNum, checksum ))
				return FALSE;
		}
	}
	
	return TRUE;
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
			return CheckFlimIntegrityIfNeeded( fName, vRefNum );
		
		return TRUE;
	}

	theSetTypeDialog = GetNewDialog( 130, NULL, (WindowPtr)-1 );
	ShowWindow( theSetTypeDialog );
	ModalDialog( NULL, &itemHit );
	DisposDialog( theSetTypeDialog );	
	if (itemHit==1)
	{
		if (CheckFlimIntegrityIfNeeded( fName, vRefNum ))
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
	InitGraf( &thePort );
	InitFonts();
	FlushEvents( everyEvent, 0 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( NULL );
	InitCursor();

	InitUtilities();
	CheckMachine();
	InitPreference();
	CodecBuildRegistry();

#ifndef PREHFS
	MaxApplZone();
#endif

		//	Log subsystem
	dinit_log();

	HideCursor();

	if (MinimalVersion())
	{
		//	In the minimal version, we don't open files from Finder
		//	we don't set the type/creator either
		//	we don't save the screen content
		//	we just display the open file and play the flim

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

	FlushEvents( everyEvent, 0 );

	return 0;
}
