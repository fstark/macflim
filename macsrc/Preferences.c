#include "Preferences.h"

#include "Config.h"
#include "Machine.h"
#include "Log.h"
#include "Util.h"
#include "Screen.h"

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
	gPreferences->playbackVBL = MachineIsMinimal()?TRUE:FALSE;
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

	if (MachineIsMinimal())
	{
		PreferenceDefaultInMemory();
		return ;
	}

#define PREF_FILE "\pMacFlim Player Preferences"
	
	sRefNum = OpenResFile( PREF_FILE );

	if (sRefNum==-1)
	{
			//	Creates initial preference file
		Create( PREF_FILE, 0, 'FLIM', 'PREF' );
		CreateResFile( PREF_FILE );
		sRefNum = OpenResFile( PREF_FILE );
		if (sRefNum!=-1)
		{
			PreferenceDefaultInMemory();
			AddResource( sPrefHdl, 'PREF', 0, "\pPreferences" );
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
		sPrefHdl = Get1Resource( 'PREF', 0 );
		//	File does not contain the right resource, we'll do everything from memory
		if (!sPrefHdl)
		{
			PreferenceDefaultInMemory();
			return ;
		}
		
		HLock( sPrefHdl );
		gPreferences = *((PreferencePtr*)sPrefHdl);

		//	Mmmm. Need to update the preferences.
		if (gPreferences->version!=kPrefVersion)
		{
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
	if (MachineIsMinimal())
		return TRUE;

	return gPreferences->playbackVBL;
}

void PreferenceSetIsPlaybackVBL( Boolean b )
{
	assert( gPreferences!=NULL, "PreferenceSetIsPlaybackVBL" );

	if (MachineIsMinimal())
		b = TRUE;

	gPreferences->playbackVBL = b;
}

//	-------------------------------------------------------------------

pascal void DoFrame( void );

#include "Playback.h"
#include "Keyboard.h"

#include "Resources.h"

void PreferenceDialog( void )
{
	enum State state = gState;
	DialogPtr preferences;
	short itemHit;
	Handle iCheckVBL;
	short iType;
	short iRect;
	Ptr savePtr = NULL;

	gState = stopRequestedState;
	while (gState!=stoppedState)
		;

	SaveScreen( &savePtr );

	preferences = GetNewDialog( kPreferenceDialogID, NULL, (WindowPtr)-1 );
	GetDItem( preferences, kPreferenceCheckVBL, &iType, &iCheckVBL, &iRect );
	SetCtlValue( iCheckVBL, PreferenceGetIsPlaybackVBL() );

	if (MachineIsMinimal())
		HiliteControl( iCheckVBL, 255 );

	UtilPlaceWindow( preferences );
	ShowWindow( preferences );
	DrawDialog( preferences );
	ShowCursor();
	do
	{
		ModalDialog( NULL, &itemHit );
		if (itemHit==kPreferenceCheckVBL)
		{
			PreferenceSetIsPlaybackVBL( !PreferenceGetIsPlaybackVBL() );
			SetCtlValue( iCheckVBL, PreferenceGetIsPlaybackVBL() );
		}
	}	while (itemHit!=kPreferenceButtonOk);

	CheckKeys();	//	This reads the keyboard and don't act on it
					//	so if the user closed the dialog with ENTER
					//	it won't be catch by the main loop, as there will be
					//	no transitions.
	HideCursor();

	PreferenceSave();
	DisposDialog( preferences );

	RestoreScreen( &savePtr );

	gPlayback.restart();

	gState = state;
}
