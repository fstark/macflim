#include "Keyboard.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Config.h"
#include "Machine.h"
#include "Util.h"

//	-------------------------------------------------------------------
//	Checks for escape (and others)
//	-------------------------------------------------------------------

static Boolean CheckEscape( unsigned char *keys )
{
	//	We start at true, so if the mouse or the return key
	//	is pressed at the start, we don't exit (we require
	//	at least one 'escaped = FALSE' before escaping)
	
	static Boolean escaped = TRUE;
	Boolean old_state = escaped;

	escaped = TestKey( keys, 0x35 )	//	ESC
		|| TestKey( keys, 0x32 )	//	`
		|| TestKey( keys, 0x33 )	//	Backspace
		|| TestKey( keys, 0x77 )	//	End
		|| TestKey( keys, 0x0c )	//	'q'
		|| Button();

	if (!old_state && escaped)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for TAB (and others)
//	-------------------------------------------------------------------

static Boolean CheckSkip( unsigned char *keys )
{
	static Boolean nexted = TRUE; //	See comment for ESCAPE
	Boolean old_state = nexted;

	nexted = TestKey( keys, 0x30 )		//	Tab
		|| TestKey( keys, 0x24 )		//	Return
		|| TestKey( keys, 0x05 )		//	Right Arrow (plus)
		|| TestKey( keys, 0x0D );		//	Left Arrow (plus)

	if (!old_state && nexted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for restart
//	-------------------------------------------------------------------

static Boolean CheckRestart( unsigned char *keys )
{
	static Boolean restarted = TRUE; //	See comment for ESCAPE
	Boolean old_state = restarted;

	restarted = TestKey( keys, 0x0f );		//	'r'

	if (!old_state && restarted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for help
//	-------------------------------------------------------------------

static Boolean CheckHelp( unsigned char *keys )
{
	static Boolean helped = TRUE; //	See comment for ESCAPE
	Boolean old_state = helped;

	helped = TestKey( keys, 0x04 );		//	'h'

	if (!old_state && helped)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for preferences
//	-------------------------------------------------------------------

static Boolean CheckPreferences( unsigned char *keys )
{
	static Boolean preferenced = TRUE; //	See comment for ESCAPE
	Boolean old_state = preferenced;

	preferenced = TestKey( keys, 0x23 );		//	'p'

	if (!old_state && preferenced)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for debug
//	-------------------------------------------------------------------

static Boolean CheckDebug( unsigned char *keys )
{
	static Boolean debugged = TRUE; //	See comment for ESCAPE
	Boolean old_state = debugged;

	debugged = TestKey( keys, 0x2 );		//	'd'

	if (!old_state && debugged)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for SPACE
//	-------------------------------------------------------------------

static Boolean CheckPause( unsigned char *keys )
{
	static Boolean paused = TRUE;	//	See comment for ESCAPE      
	Boolean old_state = paused;
	
	paused = TestKey( keys, 0x31 );	//	Space bar

	if (!old_state && paused)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Checks for mute
//	-------------------------------------------------------------------

static Boolean CheckMute( unsigned char *keys )
{
#define kMute 	0x1	//	's' key

	static Boolean muted = TRUE;	//	See comment for ESCAPE      
	Boolean old_state = muted;
	
	muted = TestKey( keys, kMute );

	if (!old_state && muted)
		return TRUE;	//	TRANSITION
	
	return FALSE;
}

//	-------------------------------------------------------------------
//	Check keys status
//	Sets the sEscape, sSkip, sPause, sMuted and other variables
//	-------------------------------------------------------------------

Boolean gEscape;
Boolean gSkip;
Boolean gPrevious;
Boolean gRestart;
Boolean gPause;
Boolean gMuted = FALSE;
Boolean gHelp;
Boolean gPreferences;
Boolean gDebug;

void CheckKeys( void )
{
	unsigned char theKeys[16];

	GetKeys( theKeys );
	gEscape = CheckEscape( theKeys );
	if (CheckSkip( theKeys ))
	{
		if (TestKey( theKeys, 56 ))	//	Shift
		{
			gSkip = FALSE;
			gPrevious = TRUE;
		}
		else
		{
			gSkip = TRUE;
			gPrevious = FALSE;
		}
	}
	else
		gSkip = gPrevious = FALSE;
	gRestart = CheckRestart( theKeys );
	gPause = CheckPause( theKeys );

		//	We don't do any UI in the minimal version
		//	and we don't support sound
	if (!MachineIsMinimal())
	{
		gHelp = CheckHelp( theKeys );
		if (CheckMute( theKeys ))
			gMuted = !gMuted;
	}

	gPreferences = CheckPreferences( theKeys );

	if (CheckDebug( theKeys ))
		 gDebug = !gDebug;
}

