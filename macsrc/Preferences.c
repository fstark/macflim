#include "Preferences.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Config.h"
#include "Machine.h"
#include "Log.h"
#include "Util.h"
#include "Screen.h"
#include "Resources.h"
#include "Debug.h"

//	-------------------------------------------------------------------
//	The preference data structure
//	-------------------------------------------------------------------

#define kPrefVersion 0x04 //	Bumped at every incompatible change of data structure

typedef struct
{
	short version;
	
	Boolean playbackVBL;
	Boolean showAll;
	Boolean setTypeCreator;
	Boolean showTipsStartup;
	Boolean loop;
	Boolean debugMenu;

	char filler1;

	Size maxBufferSize;
	short nextTipIndex;
	short nextTipBtnIndex;
}	PreferenceRecord;

typedef PreferenceRecord *PreferencePtr;

//	-------------------------------------------------------------------
//	Static variables
//	-------------------------------------------------------------------

static short sRefNum;		//	-1 if no preferences file
static Handle sPrefHdl = NULL;		//	Handle to pref
static PreferencePtr sPreferences;	//	The preferences object

//	-------------------------------------------------------------------
//	Fills gPreferences with the default preferences
//	-------------------------------------------------------------------

static void PreferenceDefault()
{
	assert( sPreferences!=NULL, "No preferences" );
	sPreferences->version = kPrefVersion;
	sPreferences->playbackVBL = MachineIsMinimal()?TRUE:FALSE;
	sPreferences->maxBufferSize = 0;
}

//	-------------------------------------------------------------------
//	Creates an in-memory locked handle with default preferences
//	-------------------------------------------------------------------

#define PREF_SIZE 64

static void PreferenceDefaultInMemory()
{
	int i;

	sPrefHdl = NewHandleNoFail( PREF_SIZE );
	HLock( sPrefHdl );
	sPreferences = *((PreferencePtr*)sPrefHdl);

	for (i=0;i!=PREF_SIZE;i++)
		(*sPrefHdl)[i] = 0;

	PreferenceDefault();
}

//	-------------------------------------------------------------------
//	Just loads from the current resource file
//	Note: we can get a few bytes on the minimal version
//	by not keeping the handle around
//	-------------------------------------------------------------------

void PreferenceLoad( void )
{
	if (MachineIsMinimal())
	{
		PreferenceDefaultInMemory();
		return ;
	}

	sPrefHdl = Get1Resource( 'PREF', kPREFResourceID );

	//	File does not contain the right resource, we'll do everything from memory
	if (!sPrefHdl)
	{
		PreferenceDefaultInMemory();
		return ;
	}
	
	HLock( sPrefHdl );
	sPreferences = *((PreferencePtr*)sPrefHdl);
}

//	-------------------------------------------------------------------

Boolean PreferenceGetIsPlaybackVBL( void )
{
	assert( sPreferences!=NULL, "IsPlaybackVBL" );

		//	On XL we have no other solution
	if (MachineIsMinimal())
		return TRUE;

	return sPreferences->playbackVBL;
}

//	-------------------------------------------------------------------

Boolean PreferenceGetShowAll( void )
{
	return sPreferences->showAll;
}

//	-------------------------------------------------------------------

void PreferenceSetShowAll( Boolean b )
{
	sPreferences->showAll = b;
}

//	-------------------------------------------------------------------

Boolean PreferenceGetSetTypeCreator( void )
{
	return sPreferences->setTypeCreator;
}

//	-------------------------------------------------------------------

void PreferenceSetSetTypeCreator( Boolean b )
{
	sPreferences->setTypeCreator = b;
}

//	-------------------------------------------------------------------

Size PreferenceGetMaxBufferSize( void )
{
	return sPreferences->maxBufferSize;
}


#ifndef MINI_PLAYER

Boolean PreferenceGetLoop( void )
{
	return sPreferences->loop;
}

void PreferenceSetLoop( Boolean b )
{
	sPreferences->loop = b;
}

//	-------------------------------------------------------------------
//	Creates preference file in System folder if not existent
//	Opens it
//	Adds it to the resource path
//	Loads the 'PREF' resource #128
//	Hlock it in memory
//	-------------------------------------------------------------------

void PreferenceInit( void )
{
	OSErr theError;

	if (MachineIsMinimal())
	{
		PreferenceDefaultInMemory();
		return ;
	}

#define PREF_FILE "\pMacFlim Preferences"
	
	sRefNum = OpenResFile( PREF_FILE );

	if (sRefNum==-1)
	{
			//	Creates initial preference file
		Create( PREF_FILE, 0, 'FLPL', 'PREF' );
		CreateResFile( PREF_FILE );
		sRefNum = OpenResFile( PREF_FILE );
		if (sRefNum!=-1)
		{
			PreferenceDefaultInMemory();
			AddResource( sPrefHdl, 'PREF', kPREFResourceID, "\pPreferences" );
			PreferenceSave();
		}
	}

	//	We don't have a working pre
	//	Cannot be open/created (maybe ROM disk? Locked drive?). Will be kept in memory, not saved
	if (sRefNum==-1)
	{
		PreferenceDefaultInMemory();
	}

	//	We have a file, we read it
	if (sRefNum!=-1)
	{
		PreferenceLoad();

		//	Mmmm. Need to update the preferences.
		if (sPreferences->version!=kPrefVersion)
		{
			HUnlock( sPrefHdl );
			SetHandleSize( sPrefHdl, sizeof( PreferenceRecord ) );
			HLock( sPrefHdl );
			sPreferences = *((PreferencePtr*)sPrefHdl);

				//	We have a new version of preferences, let's re-create them
			PreferenceDefault();
			PreferenceSave();
		}
	}
}

//	-------------------------------------------------------------------
//	Release handle and close resource file
//	-------------------------------------------------------------------

void PreferenceDispos( void )
{
	if (sPrefHdl)
	{
		HUnlock( sPrefHdl );
	}
	if (sRefNum!=-1)
	{
		CloseResFile( sRefNum );
		sRefNum = -1;
	}
}

//	-------------------------------------------------------------------
//	Write the resource back to the file
//	-------------------------------------------------------------------

void PreferenceSave( void )
{
	if (sRefNum!=-1)
	{
		ChangedResource( sPrefHdl );
		if (ResError()==noErr)
			WriteResource( sPrefHdl );
		else
			Error( "\pChangedResource failed", ResError() );
	}
}

//	-------------------------------------------------------------------

void PreferenceSetIsPlaybackVBL( Boolean b )
{
	assert( sPreferences!=NULL, "PreferenceSetIsPlaybackVBL" );

	if (MachineIsMinimal())
		b = TRUE;

	sPreferences->playbackVBL = b;
}

//	-------------------------------------------------------------------

void PreferenceSetMaxBufferSize( Size maxBufferSize )
{
	sPreferences->maxBufferSize = maxBufferSize;
}

//	-------------------------------------------------------------------

short PreferencesGetNextTipIndex( void )
{
	return sPreferences->nextTipIndex;
}

void PreferencesSetNextTipIndex( short nextTipIndex )
{
	sPreferences->nextTipIndex = nextTipIndex;
}

short PreferencesGetNextTipBtnIndex( void )
{
	return sPreferences->nextTipBtnIndex;
}

void PreferencesSetNextTipBtnIndex( short nextTipBtnIndex )
{
	sPreferences->nextTipBtnIndex = nextTipBtnIndex;
}

Boolean PreferencesGetShowTipsStartup( void )
{
	return sPreferences->showTipsStartup;
}

void PreferencesSetShowTipsStartup( Boolean showTipsStartup )
{
	sPreferences->showTipsStartup = showTipsStartup;
}

Boolean PreferencesGetDebugMenu( void )
{
	return sPreferences->debugMenu;
}

Boolean PreferencesSetDebugMenu( Boolean debugMenu )
{
	sPreferences->debugMenu = debugMenu;
}

#include "Resources.h"

void PreferenceDialog( Boolean option )
{
	DialogPtr preferences;
	short itemHit;
	Handle iCheckShowAll;
	Handle iCheckSetTypeCreator;
	Handle iMaxBufferSize;
	Handle iDebugMenu;
	short iType;
	short iRect;
	Str255 iText;
	Ptr savePtr = NULL;

		//	Get the dialog and fill it
	preferences = GetNewDialog( kDLOGPreferenceID, NULL, (WindowPtr)-1 );

	if (!option && !PreferencesGetDebugMenu())
	{
		HideDItem( preferences, kPreferenceDebugMenu );
	}

	GetDItem( preferences, kPreferenceShowAll, &iType, &iCheckShowAll, &iRect );
	SetCtlValue( iCheckShowAll, PreferenceGetShowAll() );
	GetDItem( preferences, kPreferenceSetTypeCreator, &iType, &iCheckSetTypeCreator, &iRect );
	SetCtlValue( iCheckSetTypeCreator, PreferenceGetSetTypeCreator() );

	GetDItem( preferences, kPreferenceMaxBufferSize, &iType, &iMaxBufferSize, &iRect );
	if (PreferenceGetMaxBufferSize()!=0)
		NumToString( PreferenceGetMaxBufferSize(), iText );
	else
		iText[0] = 0;
	SetIText( iMaxBufferSize, iText );

	HiliteControl( iCheckSetTypeCreator, PreferenceGetShowAll()?0:255 );


	GetDItem( preferences, kPreferenceDebugMenu, &iType, &iDebugMenu, &iRect );
	SetCtlValue( iDebugMenu, PreferencesGetDebugMenu() );

	UtilPlaceWindow( preferences, 0.2 );
	ShowWindow( preferences );
	DrawDialog( preferences );
	do
	{
		ModalDialog( NULL, &itemHit );
		if (itemHit==kPreferenceShowAll)
		{
			PreferenceSetShowAll( !PreferenceGetShowAll() );
			SetCtlValue( iCheckShowAll, PreferenceGetShowAll() );
			HiliteControl( iCheckSetTypeCreator, PreferenceGetShowAll()?0:255 );
		}
		if (itemHit==kPreferenceSetTypeCreator)
		{
			PreferenceSetSetTypeCreator( !PreferenceGetSetTypeCreator() );
			SetCtlValue( iCheckSetTypeCreator, PreferenceGetSetTypeCreator() );
		}
		if (itemHit==kPreferenceDebugMenu)
		{
			PreferencesSetDebugMenu( !PreferencesGetDebugMenu() );
			SetCtlValue( iDebugMenu, PreferencesGetDebugMenu() );
		}
	}	while (itemHit!=kPreferenceButtonOk);

		//	Max buffer size
	{
		Size maxBufferSize;
		GetIText( iMaxBufferSize, iText );
		StringToNum( iText, &maxBufferSize );
		if (maxBufferSize!=PreferenceGetMaxBufferSize())
		{
			HideCursor();	//	#### Hideous
			UtilDialog( kDLOGPreferencesRestart );
			ShowCursor();
			PreferenceSetMaxBufferSize( maxBufferSize );
		}
	}

	DebugSetMenuEnabled( PreferencesGetDebugMenu() );
	DrawMenuBar();

	PreferenceSave();
	DisposDialog( preferences );
}

#endif
