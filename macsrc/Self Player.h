#ifndef SELF_PLAYER_INCLUDED__
#define SELF_PLAYER_INCLUDED__

//	-------------------------------------------------------------------
//	SELF PLAYER INSTALLATION
//	-------------------------------------------------------------------


//	-------------------------------------------------------------------
//	Make sure internal copy of MiniPlayer is up to date
//	-------------------------------------------------------------------

Boolean SelfGetMiniPlayer( void );

//	-------------------------------------------------------------------
//	Let user select a file and makes it into self-playable file
//	-------------------------------------------------------------------

void SelfInstallPlayerUI( void );

//	-------------------------------------------------------------------
//	Makes specific FLIM into a self playable flim
//	-------------------------------------------------------------------

Boolean SelfInstallPlayer( Str255 fName, short wdRefNum, short dirId );

#endif
