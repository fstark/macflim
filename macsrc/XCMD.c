#include "HyperXCmd.h"
#include "Screen.h"
#include "Playback.h"
#include "Preferences.h"
#include "Util.h"
#include "Flim.h"
#include <QuickDraw.h>
#include <SetUpA4.h>

unsigned char *phys;
void doit();

pascal void main( XCmdBlockPtr params )
{
	GrafPtr port;
	FlimPtr flim;
	Str255 flimName;
	Ptr savePtr = NULL;
	short vRefNum;

	RememberA0();
	SetUpA4();

	InitUtilities();
	MachineInit();
	CodecInit();
	PreferenceLoad();
	BufferInit( PreferenceGetMaxBufferSize(), 30000L + GetScreenSaveSize() );

	InitPlayback();
	ScreenInit( gScreen );
	HideCursor();

	SaveScreen( &savePtr );
	ScreenClear( gScreen );

	if (params->paramCount==1)
	{
		char *name = (char *)*(params->params[0]);
		my_strcpy( (char*)flimName+1, name );
		flimName[0] = my_strlen( name );
//		DebugStr( flimName );

		GetVol( 0, &vRefNum );
		flim = FlimOpenByName( flimName, vRefNum, 0/* CurDirStore */, kHFS );
	
		if (!flim)
			MessageStr( (unsigned char*)"\pCannot open flim file" );
		else
		{
			if (PlayFlim( flim, false )!=kDone)
			{
				RestoreScreen( &savePtr );
			}
			FlimDispos( flim );
		}
	}
	
	RestoreScreen( &savePtr );
	ShowCursor();
	BufferDispos();
	DeinitUtilities();

	RestoreA4();
}
