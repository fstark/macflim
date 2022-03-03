#include "User Interface.h"

#include "Resources.h"
#include "Util.h"
#include "Self Player.h"

Handle gMenuBar;
MenuHandle gAppleMenu;
MenuHandle gFileMenu;
MenuHandle gLibraryMenu;


Boolean sFinished = FALSE;		//	Set to true if we want to exit


#include <string.h>

#include "Playback.h"
#include "Checksum.h"
#include "Library.h"
#include "Config.h"
#include "Preferences.h"
#include "Machine.h"
#include "Tips.h"

LibraryPtr sLibrary;


//	-------------------------------------------------------------------
//	Returns TRUE is flim file type is correct
//	-------------------------------------------------------------------

static Boolean IsFlimTypeCorrect( Str255 fName, short vRefNum, long dirID )
{
	OSErr err;
	OSType type;
	OSType creator;

	err = UtilGetFileTypeCreator( fName, vRefNum, dirID, &type, NULL );

	CheckErr( err, "UtilGetFileTypeCreator" );

	if (err==noErr &&
			(type=='FLIM' || type=='MMFL'))
		return TRUE;

	return FALSE;
}


static Boolean IsFlimTypeCorrectDeep( Str255 fName, short vRefNum, long dirId )
{
	OSErr err;
	short fRefNum;
	long read_size = 5;
	char buffer[522];
	static char magic[] = { 'F', 'L', 'I', 'M', 0x0a };
	Boolean theResult = FALSE;

		//	Constructing an HFS param block
	HParamBlockRec pb;
	pb.ioParam.ioNamePtr = fName;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.ioParam.ioMisc = 0;
	pb.ioParam.ioVersNum = 0;
	pb.fileParam.ioDirID = dirId;
	
		//	Open file
	err = PBHOpen( &pb, FALSE );
	if (err!=noErr)
	{
		return FALSE;
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
			theResult = TRUE;
	}

		//	Close file
	PBClose( &pb, 0 );

	return theResult;
}

//	-------------------------------------------------------------------
//	Sets the flim type
//	-------------------------------------------------------------------

static void SetFlimTypeCreator( Str255 fName, short vRefNum, long dirID )
{
	HParamBlockRec pb;
	OSErr err;

	pb.fileParam.ioCompletion = NULL;
	pb.fileParam.ioVRefNum = vRefNum;
	pb.fileParam.ioFVersNum = 0;
	pb.fileParam.ioFDirIndex = 0;
	pb.fileParam.ioNamePtr = fName;
	pb.fileParam.ioDirID = dirID;

	err = PBHGetFInfo( &pb, FALSE );

	CheckErr( err, "PBHGetFInfo" );

	if (err==noErr)
	{
		pb.fileParam.ioCompletion = NULL;
		pb.fileParam.ioVRefNum = vRefNum;
		pb.fileParam.ioFVersNum = 0;
		pb.fileParam.ioFDirIndex = 0;
		pb.fileParam.ioNamePtr = fName;
		pb.fileParam.ioDirID = dirID;

		pb.fileParam.ioFlFndrInfo.fdType='FLIM';
		pb.fileParam.ioFlFndrInfo.fdCreator='FLPL';
		err = PBHSetFInfo( &pb, FALSE );

		CheckErr( err, "PBHSetFInfo" );
	}
}

//	-------------------------------------------------------------------
//	Sets flim creator if needed and accepted by user
//	Does an optional integrity check
//	Return FALSE if file should not be played (checksum error)
//	-------------------------------------------------------------------

static Boolean SetFlimTypeCreatorIfNeeded( Str255 fName, short vRefNum, long dirID )
{
	DialogPtr theSetTypeDialog = NULL;
	short itemHit;

	if (IsFlimTypeCorrect( fName, vRefNum, dirID ))
	{
		unsigned char theKeys[16];
		GetKeys( theKeys );
		if (TestKey( theKeys, 0x3a ))	//	Option
			return ChecksumFlimIfNeeded( fName, vRefNum, dirID, TRUE );
		
		return TRUE;
	}

	if (PreferenceGetSetTypeCreator())
	{
		SetFlimTypeCreator( fName, vRefNum, dirID );
		return TRUE;
	}

	ParamText( fName, "", "", "" );
	theSetTypeDialog = GetNewDialog( kDLOGSetTypeID, NULL, (WindowPtr)-1 );
	ShowWindow( theSetTypeDialog );
	ModalDialog( NULL, &itemHit );
	DisposDialog( theSetTypeDialog );	
	if (itemHit==kSetTypeButtonOk)
	{
		if (!ChecksumFlimIfNeeded( fName, vRefNum, dirID, TRUE ))
			return FALSE;

		SetFlimTypeCreator( fName, vRefNum, dirID );
	}

	return TRUE;
}

#include <Files.h>
#include <stdio.h>

//	#### DUPLICATE FROM MacFlim.c
//	-------------------------------------------------------------------
//	Filter function : returns TRUE if file is not a FLIM
//	-------------------------------------------------------------------

static pascal Boolean CustomFileFilterDeep( FileParam *pbp )
{
	OSErr err;
	short fRefNum;
	long read_size = 5;
	char buffer[522];
	static char magic[] = { 'F', 'L', 'I', 'M', 0x0a };
	Boolean theResult = FALSE;

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
			theResult = TRUE;
	}

		//	Close file
	PBClose( &pb, 0 );

	return theResult;
}

static gDeepInspection = FALSE;

static pascal Boolean CustomFileFilter( FileParam *pbp )
{
	if (pbp->ioFlFndrInfo.fdType=='FLIM')
		return FALSE;
	
	if (pbp->ioFlFndrInfo.fdType=='APPL'
	 	&& pbp->ioFlFndrInfo.fdCreator=='MMFL')
		return FALSE;
		
	if (gDeepInspection)
		return !CustomFileFilterDeep( pbp );

	return TRUE;
}

Boolean gFirstHook = TRUE;
Str255 gDirectoryName;
Boolean gFolderSelected = FALSE;

//	The core problem is that there is no specific hook when user changes directory
//	We get an "item==100" all the time, so we can use it to update for "first time" interactions
//	Also, item 6 (drive) is called before the drive is switch, so we set the "first time" flag there
//	Same goes for item 102 (directory drop down) and item 103 (double-click on directory list)
//	#### The "disk insert" event is probably missed too -- need to check
static pascal short CustomHook( short item, DialogPtr dlg )
{
	//	Any interfaction, we change the directory name
	CInfoPBRec r;
	short iType;
	Handle iHandle;
	Rect *iRect;

		//	Handle "defered" buttons
	if (item==6 || item==102 || item==103)
		gFirstHook = TRUE;

	//	item 100 is sent continuously (unsure what it is)
	if (item==100)
		if (gFirstHook)
			gFirstHook = FALSE;
		else
			return item;		//	Ignore, to avoid setting the button if not changed

	r.hFileInfo.ioCompletion = NULL;
	r.hFileInfo.ioNamePtr = gDirectoryName;
	r.hFileInfo.ioVRefNum = SFSaveDisk;
	r.hFileInfo.ioFDirIndex = -1;
	r.hFileInfo.ioDirID = CurDirStore;	//	Selected directory
	PBGetCatInfo( &r, FALSE );

	GetDItem( dlg, kSFAddFolderChooseDirectoryID, &iType, &iHandle, &iRect);

//	This only works on the emulator, not on real hardware. Maybe a patch in ROM?
//	SetCTitle( (ControlHandle)iHandle, gDirectoryName );
	SetCTitle( (ControlHandle)iHandle, "\pAdd All Flims In Current Folder" );

	if (item==kSFAddFolderChooseDirectoryID)
	{
		gFolderSelected = TRUE;
		return 1;		//	Treat this as 'ok'
	}

	gFolderSelected = FALSE;
	return item;
}

DialogPtr gProgress = NULL;
short gLastProgressUpdt = 0;

static Boolean DoIt( HFileInfo *hfi )
{
//	if (gLastProgressUpdt<Ticks-10)
//	{
		ParamText( hfi->ioNamePtr, "", "", "" );
		DrawDialog( gProgress );
//		gLastProgressUpdt = Ticks;
//	}

	if (IsFlimTypeCorrect(hfi->ioNamePtr, hfi->ioVRefNum, hfi->ioFlParID )
	|| gDeepInspection && IsFlimTypeCorrectDeep( hfi->ioNamePtr, hfi->ioVRefNum, hfi->ioFlParID ))
	{
		SetFlimTypeCreatorIfNeeded( hfi->ioNamePtr, hfi->ioVRefNum, hfi->ioFlParID );

		if (!LibraryAddFlim( &sLibrary, hfi->ioNamePtr, hfi->ioVRefNum, hfi->ioFlParID ))
		{
			//	Failed. Do we put an alert?
		}
	}

	return TRUE;
}

static void FindAll( short vRefNum, long dirId )
{
	CInfoPBRec cipbr;
	HFileInfo *fpb = (HFileInfo *)&cipbr;
	DirInfo *dpb = (DirInfo *)&cipbr;
	short rc,idx;
	char nameBuf[255];

	fpb->ioVRefNum = vRefNum;
	fpb->ioNamePtr = (void *)nameBuf;

	for (idx=1;;idx++)
	{
		fpb->ioDirID = dirId;
		fpb->ioFDirIndex = idx;

		rc = PBGetCatInfo( &cipbr, FALSE );
		if (rc)
			break;
			
		{
			if (fpb->ioFlAttrib & 0x10)
			{
				FindAll( vRefNum, dpb->ioDrDirID );
			}
			else
				DoIt( fpb );
		}
	}
}

static Boolean UserInterfaceSFGetFlim( Boolean option, Str255 fName, short *vRefNum, long *dirID, Boolean *folders )
{
	OSErr err;
	Point where;
	SFReply theReply;
	Ptr savePtr;

	if (PreferenceGetShowAll())
	{
		gDeepInspection = TRUE;
	}
	else
	{
		gDeepInspection = FALSE;
	}

		//	Ask user for a file
	where.h = 20;
	where.v = 90;
	gFirstHook = TRUE;			//	Make sure custom file filters have a good initial button

	SFPGetFile( where, 0, CustomFileFilter, -1, NULL, CustomHook, &theReply, kSFAddFolderDialogID, NULL );
	
	if (!theReply.good)
		return FALSE;

	{
		OSType dummy;
		err = GetWDInfo( theReply.vRefNum, vRefNum, dirID, &dummy );
		CheckErr( err, "GetWDInfo" );

//		printf( "%d %ld\n", *vRefNum, *dirID );

		*folders = gFolderSelected;

		if (!*folders)
		{
			StrCpyPP( fName, theReply.fName );			
		}
		else
		{
			StrCpyPP( fName, gDirectoryName );			
		}
	}
	
	return TRUE;
}

void UserInterfaceInit()
{
	gMenuBar = GetNewMBar( kMENUBarID );

	if (!gMenuBar)
	{
		Abort( "\pCannot load the MBAR resource. Maybe you are developing and have the resource file opened in ResEdit. Or your copy of MacFlim is corrupted." );
	}

	SetMenuBar( gMenuBar );

	gAppleMenu = GetMHandle( kMENUAppleID );
	gFileMenu = GetMHandle( kMENUFileID );
	gLibraryMenu = GetMHandle( kMENULibraryID );

	AddResMenu( gAppleMenu, 'DRVR' );

	DrawMenuBar();
}

static void DoAboutMenuItem( Boolean option )
{
	short itemHit;

	DialogPtr theDialog = GetNewDialog( kDLOGAboutID, NULL, (WindowPtr)-1 );
	CheckPtr( theDialog, "kDLOGAboutID" );

	ParamText( VERSION_STRING, "", "", "" );
	UtilPlaceWindow( theDialog, 0.2 );
	ShowWindow( theDialog );
	ModalDialog( NULL, &itemHit );
	DisposDialog( theDialog );

	if (itemHit==2)
	{
		Str255 bogoStr;

		NumToString( MachineGetBogoMips(), bogoStr );
		ParamText( bogoStr, "", "", "" );

		theDialog = GetNewDialog( kDLOGThxID, NULL, (WindowPtr)-1 );
		CheckPtr( theDialog, "kDLOGThx" );
		UtilPlaceWindow( theDialog, 0.2 );
		ShowWindow( theDialog );
		ModalDialog( NULL, &itemHit );
		DisposDialog( theDialog );
	}
}

static void UserInterfaceDoAppleMenu( short aMenuItem, Boolean option )
{
	if (aMenuItem==kMENUItemAboutID)
		DoAboutMenuItem( option );
	else
	{	
		Str255 theDAName;
		GrafPtr theSavePort;

		//	TODO: Disable Edit menu items
		//	TODO: Enable File close item
		
		GetItem( gAppleMenu, aMenuItem , theDAName );
		GetPort( &theSavePort );
		
		OpenDeskAcc( theDAName );
		
		SetPort( theSavePort );
		
		//	TODO: re-enable menu items items
	}
}

static void UserInterfaceDoFileMenu( short aMenuItem, Boolean thefOption )
{
	switch (aMenuItem)
	{
		case kMENUItemShowTips:
			ToggleTips();
			break;

//		case kMenuItemCloseID:
//			DoCloseWindow( FrontWindow() );
//			break;
	
//		case kSaveFrameItemID:
//		{
//			FlimPtr theFlim = FlimFromWindow( FrontWindow() );
//			if (theFlim)
//			{
//				if (LoadFrameStillLarge( theFlim, FlimGetFrame( theFlim ) ))
//					SFSaveFrame( &theFlim->currentLargeBitMap, theFlim->selected_frame );
//			}
//			break;
//		}
		case kMENUItemPreferences:
			PreferenceDialog();
			break;
		case kMENUItemQuitID:
			sFinished = TRUE;
			break;
	}
}

static void UserInterfaceDoEditMenu( short item, short option )
{
	if (SystemEdit( item-1 ))
		return ;		//	DA has handled the command
	switch (item)
	{
		case kMENUItemSelectAll:
			LibrarySelectAll( sLibrary );
			LibraryDrawContentIfNeeded( sLibrary );
			break;
	}
}

static short Lerp( short from, short to, float v )
{
	return from+v*(to-from);
}

static void XorRect( Rect *fromRect, Rect *toRect, float v )
{
	Rect r;
	v = v*v;
	SetRect( &r,
		Lerp( fromRect->left, toRect->left, v ),
		Lerp( fromRect->top, toRect->top, v ),
		Lerp( fromRect->right, toRect->right, v ),
		Lerp( fromRect->bottom, toRect->bottom, v )
			);
	FrameRect( &r );	
}

//	-------------------------------------------------------------------
//	Performs the Zoom animation
//	This is slow on old macs
//	We could accelerate using an array of rects that we would pre-fill
//	We could also not wait on tick change on slower macs
//	(could also re-implement FrameRect patXor in assembly to avoid xor'ing with 0)
//	As of now, we just shorten the anim based on the bogomips
//	-------------------------------------------------------------------

static void XorZoom( Rect *fromRectPtr, int ticks, Boolean open )
{
	int i;
	Rect fromRect;
	Rect toRect;
	Rect r;

	int begin, end, step;	//	Re-inventing the BASIC FOR TO STEP loop

	GrafPtr savePort;
	GrafPtr directPort;

	if (MachineGetBogoMips()<4000)	//	Make sure we accelerate on the older macs
		ticks/=3;

	fromRect = *fromRectPtr;

	GetPort( &savePort );

	directPort = (GrafPtr)MyNewPtr( sizeof( GrafPort ) );
	OpenPort( directPort );
//	visRgn =copy GrayRgn
//	portRect = GrayRgn->rgnBBox;

	SetPort( directPort );

	PenMode( patXor );

	toRect = directPort->portBits.bounds;

	r = fromRect;

	if (open)
	{
		begin = 0;
		end = ticks+4;
		step = 1;
	}
	else
	{
		begin = ticks+3;
		end = -1;
		step = -1;
	}

		//	This is to make sure larger rectangle will not stay after close
	if (!open)
		XorRect( &fromRect, &toRect, 1 );

	for (i=begin;i!=end;i+=step)
	{
		long t = Ticks;

		float v0 = (i-4)/(float)ticks;
		float v1 = i/(float)ticks;

		if (v0>=0 && v0<=1)
			XorRect( &fromRect, &toRect, v0 );
		if (v1>=0 && v1<=1)
			XorRect( &fromRect, &toRect, v1 );
		while (t==Ticks)
			;
	}

		//	This is to make sure larger rectangle will not stay after open
	if (open)
		XorRect( &fromRect, &toRect, 1 );

	ClosePort( directPort );

	MyDisposPtr( directPort );

	SetPort( savePort );
}

typedef enum
{
	kStop,
	kPrev,
	kAgain,
	kNext
}	eIterateChoice;



static Boolean sZoomed;		//	Did we execute the "zoom-in code" ?
static Rect sZoomedRect;	//	From which rect did we "zoom-in" (for the zoom-out) ?

static eIterateChoice ApplyPlay( LibraryPtr lib, int index, Str255 fName, short vRefNum, long dirID )
{
	FlimPtr flim = LibraryOpenFlim( lib, index );
	eIterateChoice result = kNext;
	Rect fromRect;

		//	We failed to open, we abort
	if (!flim)
		return kStop;

	if (!sZoomed)
	{
		LibraryGetFlimRectGlobal( lib, index, &sZoomedRect );
		XorZoom( &sZoomedRect, ZOOM_FRAMES, TRUE );
		sZoomed = TRUE;
	}

	switch (PlayFlim( flim, PreferenceGetIsPlaybackVBL() ))
	{
		case kScreenError:
			ShowCursor();
			Alert( kALRTErrorNoBWScreenID, NULL );
			HideCursor();
			//	fall through
		case kError:
		case kFileError:		//	#### We should display proper error
		{
//			Str32 name;
//			ErrorFlim( "\pCannot play flim", "\pFile error", FlimGetName( flim, name ) );
		}
		case kCodecError:
//			Error( "\pCannot play flim", "\pScreen too small" );
//			break;
		case kAbort:
			result = kStop;
			break;
		case kRestart:
			result = kAgain;
			break;
		case kSkip:
			result = kNext;
			break;
		case kPrevious:
			result = kPrev;
			break;
		default:
			break;
	}

	//	Consumes the keypress/mouse from playback
	FlushEvents( everyEvent, 0 );
	FlimDispos( flim );
	
	return result;
}

static eIterateChoice ApplyCheckIntegrity( LibraryPtr lib, int index, Str255 fName, short vRefNum, long dirID )
{
	return ChecksumFlimIfNeeded( fName, vRefNum, dirID, FALSE )?kNext:kAbort;
}

typedef eIterateChoice (*ApplyFun)( LibraryPtr lib, int index, Str255 fName, short vRefNum, long dirID );

static void UserInterfaceIterateSelection( LibraryPtr lib, ApplyFun f, Boolean loop )
{
	int i;
	Str255 fName;
	short vRefNum;
	long dirID;

	do
	{
		for (i=0;i!=LibraryGetCount(lib);i++)
			if (LibraryGetSelection(lib,i))
			{
				eIterateChoice c;
				LibraryGet( lib, i, fName, &vRefNum, &dirID );
				c = ((*f)( lib, i, fName, vRefNum, dirID ));
				switch (c)
				{
					case kStop:
						return;
					case kPrev:
						i -= 2;
						if (i==-2)
						{
							if (!loop)
								i = -1;	//	No Loop? we repeat first
							else
								i = LibraryGetCount(lib)-2;	//	ugly
						}
						break;
					case kAgain:
						i--;
						break;
					case kNext:
						break;
				}
			}
	} while (loop);
}

static void UserInterfaceIterateAll( LibraryPtr lib, ApplyFun f, Boolean loop )
{
	int i;
	Str255 fName;
	short vRefNum;
	long dirID;

	do
	{
		for (i=0;i!=LibraryGetCount(lib);i++)
		{
			LibraryGet( lib, i, fName, &vRefNum, &dirID );
			if ((*f)( lib, i, fName, vRefNum, dirID ))
				return;
		}
	} while (loop);
}

static void UserInterfacePlaySelected( LibraryPtr lib )
{
	Ptr savePtr;
	GrafPtr savePort;
	
	GetPort( &savePort );
	SetPort( LibraryGetWindow( lib ) );

	sZoomed = FALSE;
	HideCursor();
	SaveScreen( &savePtr );
	if (!LibraryIsSelectionEmpty( lib ))
		UserInterfaceIterateSelection( lib, ApplyPlay, PreferenceGetLoop() );
	else
		UserInterfaceIterateAll( lib, ApplyPlay, PreferenceGetLoop() );
	RestoreScreen( &savePtr );
	if (sZoomed)
		XorZoom( &sZoomedRect, ZOOM_FRAMES, FALSE );
	ShowCursor();

	SetPort( savePort );
}

static void UserInterfaceRemoveSelected( LibraryPtr lib )
{
	WindowPtr w;
	GrafPtr savePort;
	LibraryRemoveSelectedFlims( lib );
	w = LibraryGetWindow( lib );
	GetPort( &savePort );
	SetPort( w );
	InvalRect( &w->portRect );
	SetPort( savePort );
}

static void UserInterfaceTrySelect( LibraryPtr lib, int selection )
{
	if (selection<0)
		selection = 0;
	if (selection>=LibraryGetCount( lib ))
		selection = LibraryGetCount( lib )-1;

	if (selection<0)
		return;			//	Case of empty library 
		
	LibraryUnselectAll( lib );
	LibrarySetSelection( lib, selection, TRUE );
	LibraryDrawContentIfNeeded( lib );

	LibraryScrollToIndex( lib, selection );
}

static void UserInterfaceAutoPlaySelected( LibraryPtr lib )
{
	Str255 selectedCount;
	
	NumToString( LibraryGetSelectionCount( lib ), selectedCount );
	ParamText( selectedCount, "", "", "" );
	NoteAlert( kALRTConfirmAutoplayID, NULL );
	LibraryAutostartSelectedFlims( lib );
}

static void UserInterfaceEnableDisableMenus()
{
	if (!LibraryIsSelectionEmpty( sLibrary ))
	{
		EnableItem( gLibraryMenu, kMENUItemPlayID );
		EnableItem( gLibraryMenu, kMENUItemRemoveID );
		EnableItem( gLibraryMenu, kMENUItemCheckIntegrityID );
		EnableItem( gLibraryMenu, kMENUItemAutostartID );
	}
	else
	{
		DisableItem( gLibraryMenu, kMENUItemPlayID );
		DisableItem( gLibraryMenu, kMENUItemRemoveID );
		DisableItem( gLibraryMenu, kMENUItemCheckIntegrityID );
		DisableItem( gLibraryMenu, kMENUItemAutostartID );
	}

		//	Special case 'play all flims'
	if (LibraryIsSelectionEmpty( sLibrary ) && LibraryGetCount( sLibrary )>0)
		EnableItem( gLibraryMenu, kMENUItemPlayID );

	CheckItem( gLibraryMenu, kMENUItemLoopCheckID, PreferenceGetLoop() );
	CheckItem( gLibraryMenu, kMENUItemSilentCheckID, PreferenceGetIsPlaybackVBL() );
}


static void UserInterfaceDoLibraryMenu( short item, Boolean option )
{
	switch (item)
	{
		case kMENUItemAddFlimID:
		{
			Str255 fName;
			short vRefNum;
			long dirID;
			Boolean isFolder;

			if (UserInterfaceSFGetFlim( option, fName, &vRefNum, &dirID, &isFolder ))
			{
				WindowPtr w;
				GrafPtr savePort;
				
				
				if (!isFolder)
				{
					if (SetFlimTypeCreatorIfNeeded( fName, vRefNum, dirID ))
						LibraryAddFlim( &sLibrary, fName, vRefNum, dirID );
				}
				else
				{
					assert( gProgress==NULL, "Progress Dialog is NULL" );
					gProgress = GetNewDialog( kDLOGProgress, NULL, (WindowPtr)-1 );
					assert( gProgress!=NULL, "GetNewDialog( kDLOGProgress )" );
					ShowWindow( gProgress );
					FindAll(vRefNum, dirID );
					HideWindow( gProgress );
					DisposDialog( gProgress );
					gProgress = NULL;
				}

				LibraryXXX( sLibrary );
				w = LibraryGetWindow( sLibrary );
				GetPort( &savePort );
				SetPort( w );
				InvalRect( &w->portRect );
				SetPort( savePort );
			}
			break;
		}

		case kMENUItemPlayID:
			UserInterfacePlaySelected( sLibrary );
			break;
		case kMENUItemRemoveID:
			UserInterfaceRemoveSelected( sLibrary );
			break;
		case kMENUItemCheckIntegrityID:
			UserInterfaceIterateSelection( sLibrary, ApplyCheckIntegrity, FALSE );
			break;
		case kMENUItemAutostartID:
			UserInterfaceAutoPlaySelected( sLibrary );
			break;
		case kMENUItemLoopCheckID:
			PreferenceSetLoop( !PreferenceGetLoop() );
			PreferenceSave();
			break;
		case kMENUItemSilentCheckID:
			PreferenceSetIsPlaybackVBL( !PreferenceGetIsPlaybackVBL() );
			PreferenceSave();
			break;
	}
}

static void UserInterfaceDoCommand( short aMenu, short aMenuItem, Boolean option )
{
	switch (aMenu)
	{
		case kMENUAppleID:
			UserInterfaceDoAppleMenu( aMenuItem, option );
			break;
		case kMENUFileID:
			UserInterfaceDoFileMenu( aMenuItem, option );
			break;
		case kMENUEditID:
			UserInterfaceDoEditMenu( aMenuItem, option );
			break;
		case kMENULibraryID:
			UserInterfaceDoLibraryMenu( aMenuItem, option );
			break;
	}
}

static void UserInterfaceDoMenuResult( long theMenuResult, Boolean thefOption )
{
	if (HiWord(theMenuResult)!=0)
	{
		UserInterfaceDoCommand( HiWord(theMenuResult), LoWord(theMenuResult), thefOption );
		HiliteMenu( 0 );
	}
}

///	Returns top-most window that accepts keys
static WindowPtr UserInterfaceKeyWindow()
{
	WindowPtr result = FrontWindow();
	if (IsTips(result))
		result = (WindowPtr)(((WindowRecord *)result)->nextWindow);

	return result;
}

static Boolean UserInterfaceDoKey( EventRecord *anEvent )
{
	char theKey = anEvent->message & charCodeMask;
	if (anEvent->modifiers & cmdKey)
	{
		UserInterfaceEnableDisableMenus();
		UserInterfaceDoMenuResult( MenuKey( theKey ), !!(anEvent->modifiers&optionKey) );
		return TRUE;
	}
	else
	{
		if (UserInterfaceKeyWindow()==LibraryGetWindow(sLibrary))
		{
			switch (anEvent->message&charCodeMask)
			{
				case 0x1b:
					LibraryUnselectAll( sLibrary );
					LibraryDrawContentIfNeeded( sLibrary );
					break;
				case 0x03:	//	Enter
				case 0x0d:	//	Return
					UserInterfacePlaySelected( sLibrary );
					break;
				case 0x08:	//	Delete
					UserInterfaceRemoveSelected( sLibrary );
					LibraryXXX( sLibrary );
					break;
				case 0x1c:	//	LEFT
				{
					int selection = LibraryGetFirstSelected( sLibrary );
					selection--;
					UserInterfaceTrySelect( sLibrary, selection );
					break;
				}
				case 0x1e:	//	UP
				{
					int selection = LibraryGetFirstSelected( sLibrary );
					selection -= LibraryGetColumns( sLibrary );
					UserInterfaceTrySelect( sLibrary, selection );
					break;
				}
				case 0x1d:	//	RIGHT
				{
					int selection = LibraryGetLastSelected( sLibrary );
					if (selection==-1)
						selection = LibraryGetCount( sLibrary )-1;
					selection++;
					UserInterfaceTrySelect( sLibrary, selection );
					break;
				}
				case 0x1f:	//	Down
				{
					int selection = LibraryGetLastSelected( sLibrary );
					if (selection==-1)
						selection = LibraryGetCount( sLibrary )-1;
					selection += LibraryGetColumns( sLibrary );
					UserInterfaceTrySelect( sLibrary, selection );
					break;
				}
				default:
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

static void UserInterfaceDoCloseWindow( WindowPtr aWindow )
{
	if (aWindow == LibraryGetWindow(sLibrary))
		sFinished = TRUE;
	if (IsTips(aWindow))
		ToggleTips();
}

static void UserInterfaceDoGoAway( WindowPtr aWindow, Point aStartPoint )
{
	if (TrackGoAway( aWindow, aStartPoint ))
	{
		UserInterfaceDoCloseWindow( aWindow );
	}
}

static void UserInterfaceDoMenuBar( Point aPoint, Boolean thefOption )
{
		//	Flim menu wordings before pulling down menues

	if (IsVisibleTips())
		SetItem( gFileMenu, kMENUItemShowTips, "\pHide Tips" );	//	#### Localization
	else
		SetItem( gFileMenu, kMENUItemShowTips, "\pShow Tips..." );	//	#### Localization

	if (LibraryIsSelectionEmpty( sLibrary ) && LibraryGetCount( sLibrary)>0)
	{
		SetItem( gLibraryMenu, kMENUItemPlayID, "\pPlay All Flims" );	//	#### Localization
	}
	else
	{
		SetItem( gLibraryMenu, kMENUItemPlayID, "\pPlay Selected Flims" );	//	#### Localization
	}

	UserInterfaceEnableDisableMenus();
	UserInterfaceDoMenuResult( MenuSelect( aPoint ), thefOption );
}

static void UserInterfaceDoDrag( WindowPtr aWindow, Point aStartPoint )
{
	Rect theLimit = screenBits.bounds;
	InsetRect( &theLimit, 4, 4 );

	DragWindow( aWindow, aStartPoint, &theLimit );
}

#include <stdio.h>

#define ABS(x) ((x)<0?-(x):(x))

static pascal void ScrollAction( ControlHandle control, short part )
{
	switch (part)
	{
		case inDownButton:
			LibraryScroll( sLibrary, 20 );
			break;
		case inUpButton:
			LibraryScroll( sLibrary, -20 );
			break;
		case inPageDown:
			LibraryScroll( sLibrary, LibaryGetVisibleHeight(sLibrary)-20 );
			break;
		case inPageUp:
			LibraryScroll( sLibrary, -(LibaryGetVisibleHeight(sLibrary)-20) );
			break;
	}
}

static void UserInterfaceDoContentLibrary( WindowPtr window, Point mouse, Boolean shift )
{
	int i;
	static long lastTicks = 0;
	static Point lastMouse;

	long ticks = Ticks;
	Boolean doubleClick = FALSE;

	//	Looking for drag
	{
		ControlHandle control;
		short part = FindControl( mouse, window, &control );
		if (part==inThumb)
		{	//	This is braindead: if we don't pass NULL, it crashes
			if (TrackControl( control, mouse, NULL ))
				LibraryScrollTo( sLibrary, GetCtlValue( control ) );
		}
		else
			if (part)
				TrackControl( control, mouse, ScrollAction );

		if (part!=0)
			return;		//	We were in a control, nothing more to do
	}


	if (ticks-lastTicks<DoubleTime)
		if (ABS(lastMouse.h-mouse.h)<5
			&& ABS(lastMouse.v-mouse.v)<5)
				doubleClick = TRUE;
	lastTicks = Ticks;
	lastMouse = mouse;

	i = LibraryGetFlimUnder( sLibrary, mouse );

	if (i!=-1)
	{
		if (!doubleClick)
		{
			if (!shift)
			{
				if (!LibraryGetSelection( sLibrary, i ))
				{
					LibraryUnselectAll( sLibrary );
					LibrarySetSelection( sLibrary, i, TRUE );
				}
			}
			else
				LibrarySetSelection( sLibrary, i, !LibraryGetSelection( sLibrary, i ) );
		}
		else
		{
			UserInterfacePlaySelected( sLibrary );
		}
	}
	else
	{
		LibraryUnselectAll( sLibrary );
	}
	LibraryDrawContentIfNeeded( sLibrary );
}

static void UserInterfaceDoMouse( EventRecord *anEvent )
{
	WindowPtr theWindow;
	short where;

	where = FindWindow( anEvent->where, &theWindow );

	switch (where)
	{
		case inMenuBar:
			UserInterfaceDoMenuBar( anEvent->where, !!(anEvent->modifiers&optionKey) );
			break;
		
		case inSysWindow:
			SystemClick( anEvent, theWindow );
			break;

		case inDrag:
		{
			WindowPtr frontWindow = FrontWindow();
			UserInterfaceDoDrag( theWindow, anEvent->where );
//			if (FrontWindow()!=frontWindow)
//			{
//				InvalWindow( frontWindow );
//				InvalWindow( FrontWindow() );
//			}
			break;
		}
		case inGrow:
		{
			WindowPtr frontWindow = FrontWindow();
			Rect theRect;
			long res;
			short w,h;

			SetRect( &theRect,
				LibaryGetMinWidth( sLibrary ),
				LibaryGetMinHeight( sLibrary ),
				LibaryGetMaxWidth( sLibrary ),
				LibaryGetMaxHeight( sLibrary ) );
			res = GrowWindow( frontWindow, anEvent->where, &theRect );
			if (res)
			{
				h = HiWord( res );
				w = LibraryGetBestWidth( sLibrary, LoWord( res ) );
			
				SizeWindow( frontWindow, w, h, TRUE );
				LibraryWindowResized( sLibrary );
			}

			{
				GrafPtr savePort;
				GetPort( &savePort );
				SetPort( frontWindow );
				InvalRect( &frontWindow->portRect );
				SetPort( savePort );
			}
		}
		case inGoAway:
			UserInterfaceDoGoAway( theWindow, anEvent->where );
			break;
			
		case inContent:
			if (theWindow!=FrontWindow())
			{
//				InvalWindow( FrontWindow() );
				SelectWindow( theWindow );
//				InvalWindow( FrontWindow() );
			}
			else
			{
				SetPort( theWindow );
				GlobalToLocal( &anEvent->where );
				if (theWindow==LibraryGetWindow( sLibrary ))
					UserInterfaceDoContentLibrary( theWindow, anEvent->where, !!(anEvent->modifiers&shiftKey) );
			}
			break;
	}
}

static void UserInterfaceDoUpdate( WindowPtr aWindow )
{
	GrafPtr savedPort;

	GetPort( &savedPort );
	SetPort( aWindow );
	BeginUpdate( aWindow );

	if (aWindow==LibraryGetWindow(sLibrary))
	{
		LibraryDrawWindow( sLibrary );
	}

	EndUpdate( aWindow );	
	SetPort( savedPort );
}

///	Return TRUE if event was handled
static Boolean DoDialogEvent( EventRecord *anEvent )
{
	DialogPtr theDialog;
	short theItem;
	
	if (anEvent->what==keyDown && anEvent->modifiers&cmdKey)
	{
		return UserInterfaceDoKey( anEvent );
	}
	
	
	if (DialogSelect( anEvent, &theDialog, &theItem ))
	{
		if (IsTips( theDialog ))
			return DoTipsSelect( theItem, anEvent );
	}
	
	return FALSE;
}

void UserInterfaceLoop()
{
	//	Note: we do not manage multi-finder properly
	RgnHandle mouseRgn;
	EventRecord theEvent;
	
		//	Shows library
	sLibrary = LibraryOpenDefault();
	ShowWindow( LibraryGetWindow(sLibrary) );

		//	Show tool tips window (in front of Libary)
	if (PreferencesGetShowTipsStartup())
		ToggleTips();


	mouseRgn = NewRgn();

	while (!sFinished)
	{
			//	#### If I understood correctly, this should be WaitNextEvent for multi-finder
			//	see technote #158
		if (!GetNextEvent( everyEvent, &theEvent ))
			;
//			DoUpdateMovie();

		if (IsDialogEvent( &theEvent ))
		{
			if (DoDialogEvent( &theEvent ))
				continue;		//	Handled
		}

		switch (theEvent.what)
		{
			case mouseDown:
				UserInterfaceDoMouse( &theEvent );
				break;
			case keyDown:
			case autoKey:
				UserInterfaceDoKey( &theEvent );
				break;
			case activateEvt:
//?? not working				DoUpdate( (WindowPtr)(theEvent.message) );
				break;
			case updateEvt:
				UserInterfaceDoUpdate( (WindowPtr)(theEvent.message) );
				break;
		}
	}
}
