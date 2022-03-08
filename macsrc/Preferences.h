#ifndef PREFERENCES_INCLUDED__
#define PREFERENCES_INCLUDED__

//	-------------------------------------------------------------------
//	MacFlim user preferences management
//	-------------------------------------------------------------------
//  Manages the preference file for MacFlim
//
//  Preferences are stored in a global data structure
//  Load it by call PreferenceInit() after InitResources()
//  Access it by PreferenceGetXXX() functions
//  Change it by PreferenceSetXXX() functions
//		(note: incompatible preferences may be adjusted by the SetXXXCode)
//  After modification, use PreferenceSave() to flush it to disk
//  At the end of the program, call PreferenceDispos()
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Loads preference from the resource. Can be used from mini player
//	-------------------------------------------------------------------

void PreferenceLoad( void );

//	-------------------------------------------------------------------
//	VBL driven playback. Can be used from mini player
//	-------------------------------------------------------------------

Boolean PreferenceGetIsPlaybackVBL( void );
void PreferenceSetIsPlaybackVBL( Boolean b );

//	-------------------------------------------------------------------
//	Play in loop
//	-------------------------------------------------------------------

Boolean PreferenceGetLoop( void );
void PreferenceSetLoop( Boolean b );

//	-------------------------------------------------------------------
//	Show all flims in SFPGetFile
//	-------------------------------------------------------------------

Boolean PreferenceGetShowAll( void );
void PreferenceSetShowAll( Boolean b );

//	-------------------------------------------------------------------
//	Set type/creator on open
//	-------------------------------------------------------------------

Boolean PreferenceGetSetTypeCreator( void );
void PreferenceSetSetTypeCreator( Boolean b );

//	-------------------------------------------------------------------
//	Size of buffers (0 if no size set). Can be used from mini player
//	-------------------------------------------------------------------

Size PreferenceGetMaxBufferSize( void );
void PreferenceSetMaxBufferSize( Size maxBufferSize );

//	-------------------------------------------------------------------

short PreferencesGetNextTipIndex( void );
void PreferencesSetNextTipIndex( short nextTipIndex );
short PreferencesGetNextTipBtnIndex( void );
void PreferencesSetNextTipBtnIndex( short nextTipBtnIndex );
Boolean PreferencesGetShowTipsStartup( void );
void PreferencesSetShowTipsStartup( Boolean showTipsStartup );
Boolean PreferencesGetDebugMenu( void );
Boolean PreferencesSetDebugMenu( Boolean debugMenu );

//	-------------------------------------------------------------------
//	Call at the beginning of the program
//	-------------------------------------------------------------------

void PreferenceInit( void );
 
//	-------------------------------------------------------------------
//	Dispose of all preferences resources
//	-------------------------------------------------------------------

void PreferenceDispos( void );

//	-------------------------------------------------------------------
//	Call when you have changed a preference
//	-------------------------------------------------------------------

void PreferenceSave( void );

//	-------------------------------------------------------------------
//	Shows the preference dialog, let user interact and save
//	-------------------------------------------------------------------

void PreferenceDialog( Boolean option );

#endif
