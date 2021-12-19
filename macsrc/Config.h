#ifndef CONFIG_INCLUDED__
#define CONFIG_INCLUDED__

//	-------------------------------------------------------------------
//	Those #defines configures the player binary
//	-------------------------------------------------------------------

#define noVERBOSE			//	Enables verbose logs
#define noSYNCPLAY			//	Just execute the display code in synchronous mode for decoding debugging
#define CODEC_TYPE 1		//	0 => Forces use of reference codec implementation
#define REFERENCE_ASSERTS	//	Adds debug asserts in reference codecs
#define noDISPLAY_STATS		//	Displays playback stats at the end
#define noFORCE_MINIMAL		//	Forces the machine to be the minimal version for testing

#define HUD
#define noNODISPLAY			//	Doesn't actually execute the flim display code

#endif
