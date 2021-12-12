#include "Preferences.h"

#include "Log.h"
#include "Util.h"
#include "Config.h"

//	-------------------------------------------------------------------
//	The preference data structure
//	-------------------------------------------------------------------

//	Bumped at every incompatible change of data structure
#define kPrefVersion 0x01

typedef struct
{
	short version;
	
	Boolean playbackVBL;
}	PreferenceRecord;

typedef PreferenceRecord *PreferencePtr;

//	-------------------------------------------------------------------
//	Static variables
//	-------------------------------------------------------------------

static short sRefNum;		//	-1 if no preferences file
Handle sPrefHdl = NULL;		//	Handle to pref
PreferencePtr gPreferences;	//	The preferences object

//	-------------------------------------------------------------------
//	Fills gPreferences with the default preferences
//	-------------------------------------------------------------------

static void PreferenceDefault()
{
	assert( gPreferences!=NULL, "No preferences" );
	gPreferences->version = kPrefVersion;
	gPreferences->playbackVBL = MinimalVersion()?TRUE:FALSE;
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
	gPreferences = *((PreferencePtr*)sPrefHdl);

	for (i=0;i!=PREF_SIZE;i++)
		(*sPrefHdl)[i] = 0;

	PreferenceDefault();
}

//	-------------------------------------------------------------------
//	Creates preference file in System folder if not existent
//	Opens it
//	Adds it to the resource path
//	Loads the 'PREF' resource #0
//	Hlock it in memory
//	-------------------------------------------------------------------

void PreferenceInit( void )
{
	OSErr theError;

	if (MinimalVersion())
	{
		PreferenceDefaultInMemory();
		return ;
	}
	
	sRefNum = OpenResFile( "\pMacFlim Player Preferences" );

	if (sRefNum==-1)
	{
		Create( "\pMacFlim Preferences", 0, 'FLIM', 'PREF' );
		CreateResFile( "\pMacFlim Preferences" );
		sRefNum = OpenResFile( "\pMacFlim Preferences" );
		if (sRefNum!=-1)
		{
			PreferenceDefaultInMemory();

			AddResource( sPrefHdl, 'PREF', 0, "\pPreferences" );
			if (ResError()!=noErr)
			{
				sRefNum = -1;	//	With some leaks
			}
			PreferenceSave();
		}
	}

	if (sRefNum==-1)
	{
		//	Pref will be in memory, not saved (maybe ROM disk? Locked drive?)
		PreferenceDefaultInMemory();
	}
	else
	{
		sPrefHdl = Get1Resource( 'PREF', 0 );
		if (!sPrefHdl)
		{
			PreferenceDefaultInMemory();
			return ;
		}
		HLock( sPrefHdl );

		if (gPreferences->version!=kPrefVersion)
		{
			gPreferences = *((PreferencePtr*)sPrefHdl);
			HUnlock( sPrefHdl );
			SetHandleSize( sPrefHdl, sizeof( PreferenceRecord ) );
			HLock( sPrefHdl );
			gPreferences = *((PreferencePtr*)sPrefHdl);

				//	We have a new version of preferences, let's re-create them
			PreferenceDefault();
			PreferenceSave();
		}
	}
}

//	Dispose of all preferences resources
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

//	Call when you have changed the preferences
void PreferenceSave( void )
{
	if (sRefNum!=-1)
	{
		ChangedResource( sPrefHdl );
		if (ResError()==noErr)
			WriteResource( sPrefHdl );
	}
}

Boolean PreferenceGetIsPlaybackVBL( void )
{
	assert( gPreferences!=NULL, "IsPlaybackVBL" );

		//	On XL we have no other solution
	if (MinimalVersion())
		return TRUE;

	return gPreferences->playbackVBL;
}

void PreferenceSetIsPlaybackVBL( Boolean b )
{
	assert( gPreferences!=NULL, "PreferenceSetIsPlaybackVBL" );

	if (MinimalVersion())
		b = TRUE;

	gPreferences->playbackVBL = b;
}

//	-------------------------------------------------------------------

pascal void DoFrame( void );

#include "Playback.h"

void PreferenceDialog( void )
{
	DialogPtr preferences = GetNewDialog( 135, NULL, (WindowPtr)-1 );
	short itemHit;

	gState = stopRequestedState;
	while (gState!=stoppedState)
		;

	ShowWindow( preferences );
	DrawDialog( preferences );
	ShowCursor();
	ModalDialog( NULL, &itemHit );
	HideCursor();
	
	DisposDialog( preferences );

	gPlayback.restart();
}
