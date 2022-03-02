#ifndef UTIL_INCLUDED__
#define UTIL_INCLUDED__

typedef enum
{
	kMFS = 0,
	kHFS = 1
}	eFileAPI;

#define kNoDirID 0

//	-------------------------------------------------------------------
//	RANDOM LOW-LEVEL UTILITIES
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Assertion (with C-string)
//	-------------------------------------------------------------------

void assert( int v, const char *msg );

//	-------------------------------------------------------------------
//	Initing the utilities set aside some memory to be able to properly
//	abort in low-memory conditions
//	-------------------------------------------------------------------

void InitUtilities( void );

//	-------------------------------------------------------------------
//	Empty routine helpful to place a breakpoint to
//	-------------------------------------------------------------------

void BreakHere( void );

//	-------------------------------------------------------------------
//	Displays a string modal
//	-------------------------------------------------------------------

void MessageStr( Str255 s );

//	-------------------------------------------------------------------
//	Displays a long modal
//	-------------------------------------------------------------------

void MessageLong( long l );

//	-------------------------------------------------------------------
//	Displays the string and abort the program
//	-------------------------------------------------------------------

void Abort( Str255 s );

void Error( Str255 s, short err );

//	-------------------------------------------------------------------
//	Allocation/Deallocation wrappers
//	-------------------------------------------------------------------

Ptr MyNewPtr( Size aSize );
void MyDisposPtr( void *aPtr );

//	-------------------------------------------------------------------
//	Allocation without fail
//	-------------------------------------------------------------------

Ptr NewPtrNoFail( Size aSize );

//	-------------------------------------------------------------------
//	Allocation without fail
//	-------------------------------------------------------------------

Handle NewHandleNoFail( Size aSize );

//	-------------------------------------------------------------------
//	If err is not noErr, displays a dialog and abort the program
//	-------------------------------------------------------------------

void CheckErr( short err, const char *msg );

//	-------------------------------------------------------------------
//	If ptr is NULL, displays a dialog and abort the program
//	-------------------------------------------------------------------

void CheckPtr( void *p, const char *msg );

//	-------------------------------------------------------------------
//	GENERAL LOW LEVEL ROUTINES/MACROS
//	-------------------------------------------------------------------
//	Some re-implementation/variations of C library functions
//	and general macros
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Copies a Pascal string
//	-------------------------------------------------------------------

void StrCpyPP( Str255 p, const Str255 q );

//	-------------------------------------------------------------------
//	Concatenates a Pascal string and a C string
//	-------------------------------------------------------------------

void StrCatPC( Str255 p, const char *q );

//	-------------------------------------------------------------------
//	Concatenates a Pascal string and a Pascal string
//	-------------------------------------------------------------------

void StrCatPP( Str255 p, Str255 q );

//	####
void UtilPlaceWindow( WindowPtr window, float percentTop );

//	-------------------------------------------------------------------
//	Helper function to test if a key is set
//	-------------------------------------------------------------------

Boolean TestKey( unsigned char *keys, char k );

//	-------------------------------------------------------------------
//	Size of a save screen buffer in bytes
//	-------------------------------------------------------------------

Size GetScreenSaveSize( void );

//	-------------------------------------------------------------------
//	Allocates memory and saves screen
//	-------------------------------------------------------------------

void SaveScreen( Ptr *ptr );

//	-------------------------------------------------------------------
//	Restore saved screen and deallocates memory
//	-------------------------------------------------------------------

void RestoreScreen( Ptr *ptr );

//	-------------------------------------------------------------------
//	Get type/creator of file
//	-------------------------------------------------------------------

OSErr UtilGetFileTypeCreator( Str255 fName, short vRefNum, long dirID, OSType *type, OSType *creator );

//	-------------------------------------------------------------------
//	Set type/creator of file
//	-------------------------------------------------------------------

OSErr UtilSetFileTypeCreator( Str255 fName, short vRefNum, long dirID, OSType type, OSType creator );

//	-------------------------------------------------------------------
//	Perform a modal dialog properly placed
//	-------------------------------------------------------------------

void UtilDialog( short dlogID );

//	-------------------------------------------------------------------
//	Outputs a string and a number in the debugger
//	-------------------------------------------------------------------

void DebugStrLong( Str255 s, long l );


#endif
