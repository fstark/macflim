//	-------------------------------------------------------------------
//	The help panel is currently the basic UI of Mac Flim
//	-------------------------------------------------------------------

#include "Help.h"

//	-------------------------------------------------------------------

#include "Playback.h"
#include "Screen.h"
#include "Keyboard.h"

//	-------------------------------------------------------------------
//	
//	-------------------------------------------------------------------

void DisplayHelp( void )
{
	enum State state = gState;
	Ptr savePtr = NULL;
	long ticks;
	DialogPtr theDialog;
	DialogPtr theDialog1;
	DialogPtr theDialog2;

	gState = pauseRequestedState;

		//	Load the dialogs during the last bits of on-screen display
	theDialog1 = GetNewDialog( 128, NULL, (WindowPtr)-1 );
	theDialog2 = GetNewDialog( 134, NULL, (WindowPtr)-1 );

		//	We wait until display stabilize
	while (gState!=pauseRequestedState)
		;

	if (!theDialog1 || !theDialog2)
		goto done ;

	SaveScreen( &savePtr );

	if (savePtr==NULL)
		goto done;

	theDialog = theDialog1;

	ShowWindow( theDialog );
	DrawDialog( theDialog );
	ticks = Ticks + 60*5;

	while (!Button())
	{
		CheckKeys();
		if (sHelp || sEscape)
			break;
		if (ticks<Ticks)
		{
			HideWindow( theDialog );
			if (theDialog==theDialog1)
				theDialog = theDialog2;
			else
				theDialog = theDialog1;
			ShowWindow( theDialog );
			DrawDialog( theDialog );
			ticks = Ticks + 60*5;
		}
	}
	
done:
	if (theDialog1)
		DisposDialog( theDialog1 );
	if (theDialog2)
		DisposDialog( theDialog2 );

	RestoreScreen( &savePtr );

		//	Make sure button is not clicked
	while (Button())
		;

	gState = state;
}

