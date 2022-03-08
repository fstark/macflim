#ifndef DEBUG_INCLUDED__
#define DEBUG_INCLUDED__

//	-------------------------------------------------------------------
//	Debug functions for MacFlim
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Updates the display of Mem in top right corner of the screen
//	-------------------------------------------------------------------

void DebugUpdateMem( void );

//	-------------------------------------------------------------------
//	Clears display of mem in top righ
//	-------------------------------------------------------------------

void DebugMemUpdateClear( void );

void DebugUpdate( void );

void DebugOnOff( void );

Boolean DebugIsOn( void );

void DebugFillMem( void );

void DebugSetMenuEnabled( Boolean menuEnabled );

#endif
