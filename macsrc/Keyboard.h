#ifndef KEYBOARD_INCLUDED__
#define KEYBOARD_INCLUDED__

//	-------------------------------------------------------------------
//	KEYBOARD HANDLING ROUTINES AND FLAGS
//	-------------------------------------------------------------------

extern Boolean gEscape;
extern Boolean gSkip;
extern Boolean gPrevious;
extern Boolean gRestart;
extern Boolean gPause;
extern Boolean gMuted;
extern Boolean gHelp;
extern Boolean gPreferences;
extern Boolean gDebug;

//	-------------------------------------------------------------------
//	Checks keyboard state and set the flags variable to indicate
//	user action
//	-------------------------------------------------------------------

void CheckKeys( void );

#endif
