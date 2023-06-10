#ifndef MOUSE_INCLUDED__
#define MOUSE_INCLUDED__

//	-------------------------------------------------------------------
//	Mouse support
//	-------------------------------------------------------------------
//	Rationale: as we directly blit on the screen, enabling the mouse
//	during playback result in glitches and artifacts.
//	Here are routines that handle the display of the mouse in a way
//	that is compatible with MacFlim playback
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Initial precomputation of mouse shape and shifts
//	-------------------------------------------------------------------
void ComputeMouse( void );

//	-------------------------------------------------------------------
//	Restores the screen behind the mouse
//	(MUST NOT BE CALLED BEFORE DrawMouse)
//	-------------------------------------------------------------------
void RestoreCursor( void );

//	-------------------------------------------------------------------
//	Draws the mouse destroying the content below
//	-------------------------------------------------------------------
void DrawMouse( void );


#endif
