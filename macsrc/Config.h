//	-------------------------------------------------------------------
//	Those #defines help debugging the player
//	-------------------------------------------------------------------

#define noVERBOSE			//	Enables verbose logs
#define noSYNCPLAY			//	Just execute the display code in synchronous mode for decoding debugging
#define noREFERENCE			//	Forces use of reference codec implementation (with support for non 512x342)
#define noREFERENCE_ASSERTS	//	Adds debug asserts in reference codecs
#define noDISPLAY_STATS		//	Displays playback stats at the end
#define noFORCE_MINIMAL		//	Forces the machine to be the minimal version for testing

#define PREHFS				//	If true, uses pre-HFS system calls

#define MOVIE_BUFFER_SIZE			30000

//	-------------------------------------------------------------------
//	Call once to perform checks about the kind of enviroment
//	we are running in
//	-------------------------------------------------------------------
void CheckMachine( void );

//	-------------------------------------------------------------------
//	If TRUE, we are running on a very old machine, and should avoid 
//	*everything* fancy
//	-------------------------------------------------------------------
Boolean MinimalVersion( void );
