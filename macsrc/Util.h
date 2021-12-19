
//	-------------------------------------------------------------------
//	Initing the utilities set aside some memory to be able to properly
//	abort in low-memory conditions
//	-------------------------------------------------------------------

void InitUtilities( void );

//	-------------------------------------------------------------------
//	Displays the string and abort the program
//	-------------------------------------------------------------------

void Abort( Str255 s );

//	-------------------------------------------------------------------
//	Allocation without fail
//	-------------------------------------------------------------------
Ptr NewPtrNoFail( Size aSize );

//	-------------------------------------------------------------------
//	Allocation without fail
//	-------------------------------------------------------------------

Handle NewHandleNoFail( Size aSize );

//	-------------------------------------------------------------------
//	If err is not noErr, displays a dialog and abort the programm
//	-------------------------------------------------------------------

void CheckErr( short err, const char *msg );

//	-------------------------------------------------------------------
//	GENERAL LOW LEVEL ROUTINES/MACROS
//	-------------------------------------------------------------------
//	Some re-implementation/variations of C library functions
//	and general macros
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Concatenates a Pascal String and a C String
//	-------------------------------------------------------------------
void StrCatPC( Str255 p, const char *q );

//	####
void UtilPlaceWindow( WindowPtr window );
