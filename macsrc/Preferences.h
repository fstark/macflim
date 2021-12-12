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

//	Call at the beginning of the program
void PreferenceInit( void );
 
//	Dispose of all preferences resources
void PreferenceDispos( void );

//	Call when you have changed a preference
void PreferenceSave( void );

//	VBL driven playback
Boolean PreferenceGetIsPlaybackVBL( void );
void PreferenceSetIsPlaybackVBL( Boolean b );

//	-------------------------------------------------------------------
//	Shows the preference dialog, let user interact and save
//	-------------------------------------------------------------------

void PreferenceDialog( void );
