/*
 * Manages the preference file for MacFlim
 *
 * Preferences are stored in a PreferenceRecord global data structure
 * Load it by call InitPreferences() after InitResources()
 * Access it by gPreferences->whatever
 * After modification, use SavePreferences() to flush it to disk
 * At the end of the program, call DisposPreferences()
 */

typedef struct
{
	short version;
	
	Boolean playbackVBL;
}	PreferenceRecord;

typedef PreferenceRecord *PreferencePtr;

//	Use this global variable to access and change preferences
extern PreferencePtr gPreferences;

//	Call at the beginning of the program
void InitPreference( void );
 
//	Dispose of all preferences resources
void DisposPreference( void );

//	Call when you have changed the preferences
void SavePreferences( void );

//	Bumped at every incompatible change of data structure
#define kPrefVersion 0x01

//	VBL driven playback
Boolean IsPlaybackVBL( void );
