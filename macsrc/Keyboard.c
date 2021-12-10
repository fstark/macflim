//	-------------------------------------------------------------------
//	KEYBOARD HANDLING ROUTINES
//	-------------------------------------------------------------------

#include "Keyboard.h"

//	-------------------------------------------------------------------
//	Helper to test a key
//	-------------------------------------------------------------------

Boolean TestKey( unsigned char *keys, char k )
{
	return !!((keys[k>>3]>>(k&7))&1);
}
	
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
//	Checks for SPACE
//	-------------------------------------------------------------------

Boolean CheckPause( unsigned char *keys )
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

Boolean CheckMute( unsigned char *keys )
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
//	Sets the sEscape, sSkip, sPause and sMuted variable
//	-------------------------------------------------------------------

Boolean sEscape;
Boolean sSkip;
Boolean sPrevious;
Boolean sRestart;
Boolean sPause;
Boolean sMuted = FALSE;
Boolean sHelp;
Boolean sPreferences;

void CheckKeys( void )
{
	unsigned char theKeys[16];

	GetKeys( theKeys );
	sEscape = CheckEscape( theKeys );
	if (CheckSkip( theKeys ))
	{
		if (TestKey( theKeys, 56 ))	//	Shift
		{
			sSkip = FALSE;
			sPrevious = TRUE;
		}
		else
		{
			sSkip = TRUE;
			sPrevious = FALSE;
		}
	}
	else
		sSkip = sPrevious = FALSE;
	sRestart = CheckRestart( theKeys );
	sPause = CheckPause( theKeys );
	sHelp = CheckHelp( theKeys );
	sPreferences = CheckPreferences( theKeys );
	if (CheckMute( theKeys ))
		sMuted = !sMuted;
}

