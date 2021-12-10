#include "Preferences.h"

#include "Log.h"
#include "Util.h"
#include "Config.h"

//	-------------------------------------------------------------------
//	Static variable
//	-------------------------------------------------------------------

static short sRefNum;	//	-1 if no preferences file
Handle sPrefHdl = NULL;	//	Handle to pref

//	-------------------------------------------------------------------
//	Fills gPreferences with the default preferences
//	-------------------------------------------------------------------

static void DefaultPreferences()
{
	assert( gPreferences!=NULL, "No preferences" );
	gPreferences->version = kPrefVersion;
	gPreferences->playbackVBL = FALSE;
}

//	-------------------------------------------------------------------
//	Creates an in-memory locked handle with default preferences
//	-------------------------------------------------------------------

#define PREF_SIZE 64

static void DefaultInMemoryPreference()
{
	sPrefHdl = NewHandleNoFail( PREF_SIZE );
	HLock( sPrefHdl );
	gPreferences = *((PreferencePtr*)sPrefHdl);

	DefaultPreferences();
}

//	-------------------------------------------------------------------
//	Creates preference file in System folder if not existent
//	Opens it
//	Adds it to the resource path
//	Loads the 'PREF' resource #0
//	Hlock it in memory
//	-------------------------------------------------------------------

void InitPreference( void )
{
	OSErr theError;

	if (MinimalVersion())
	{
		DefaultInMemoryPreference();
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
			DefaultInMemoryPreference();

			AddResource( sPrefHdl, 'PREF', 0, "\pPreferences" );
			if (ResError()!=noErr)
			{
				sRefNum = -1;	//	With some leaks
			}
			SavePreferences();
		}
	}

	if (sRefNum==-1)
	{
		//	Pref will be in memory, not saved (maybe ROM disk? Locked drive?)
		DefaultInMemoryPreference();
	}
	else
	{
		sPrefHdl = Get1Resource( 'PREF', 0 );
		if (!sPrefHdl)
		{
			DefaultInMemoryPreference();
			return ;
		}
		HLock( sPrefHdl );
		gPreferences = *((PreferencePtr*)sPrefHdl);

		if (gPreferences->version!=kPrefVersion)
		{
			HUnlock( sPrefHdl );
			SetHandleSize( sPrefHdl, sizeof( PreferenceRecord ) );
			HLock( sPrefHdl );
			gPreferences = *((PreferencePtr*)sPrefHdl);

				//	We have a new version of preferences, let's re-create them
			DefaultPreferences();
			SavePreferences();
		}
	}
}

//	Dispose of all preferences resources
void DisposPreference( void )
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

PreferencePtr gPreferences;

//	Call when you have changed the preferences
void SavePreferences( void )
{
	if (sRefNum!=-1)
	{
		ChangedResource( sPrefHdl );
		WriteResource( sPrefHdl );
	}
}

Boolean IsPlaybackVBL( void )
{
	assert( gPreferences!=NULL, "IsPlaybackVBL" );

		//	On XL we have no other solution
	if (MinimalVersion())
		return TRUE;

	return gPreferences->playbackVBL;
}
