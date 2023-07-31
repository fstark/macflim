#include "HyperXCmd.h"
#include "Screen.h"
#include "Playback.h"
#include "Preferences.h"
#include "Util.h"
#include "Flim.h"
#include <QuickDraw.h>
#include <SetUpA4.h>
#include "Mouse.h"

//	-------------------------------------------------------------------
//	Parses the XCMD arguments and put them in the global variables
//	-------------------------------------------------------------------
//	playflim FLIMFILE,
//		  [window]
//		| [window x y]
//		| [fullscreen]
//		  [loop]
//		| [no-loop]
//		  [sound]
//		| [no-sound]
//		  [mouse]
//		| [no-mouse]
//	-------------------------------------------------------------------

Str255 fileName;	//	Name of the flim to play
Boolean fullscreen;	//	Play full screen ?
long x;
long y;
Boolean loop;		//	Should we loop?
Boolean sound;		//	Should we play sound?
Boolean mouse;		//	Stop on mouse?

const char *error = NULL;

Boolean ArgGetString( XCmdBlockPtr params, int index, Str255 result, Str255 def )
{
	char *str;

	StrCpyPP( result, def );

	if (index>=params->paramCount)
		return false;

	str = (char *)*(params->params[index]);
	my_strcpy( (char*)result+1, str );
	result[0] = my_strlen( str );

	return true;
}

Boolean ArgIsString( XCmdBlockPtr params, int index, Str255 what )
{
	Str255 arg;

	if (!ArgGetString( params, index, arg, "\p" ))
		return false;

	return StrEquPP( arg, what );
}

Boolean ArgIsOption( XCmdBlockPtr params, int index, Str255 optName, Boolean *option )
{
	Str255 arg;
	Str255 test;

	StrCpyPP( test, optName );
	if (ArgIsString( params, index, test ))
	{
		*option = true;
		return true;
	}	

	StrCpyPP( test, "\pno" );
	StrCatPP( test, optName );
	if (ArgIsString( params, index, test ))
	{
		*option = false;
		return true;
	}	

	StrCpyPP( test, "\pno-" );
	StrCatPP( test, optName );
	if (ArgIsString( params, index, test ))
	{
		*option = false;
		return true;
	}	

	if (ArgIsString( params, index, "\ptrue" ))
	{
		*option = true;
		return true;
	}	

	if (ArgIsString( params, index, "\pfalse" ))
	{
		*option = false;
		return true;
	}	

	return false;
}

Boolean ArgIsNumber( XCmdBlockPtr params, int index, long *num )
{
	Str255 arg;

	if (!ArgGetString( params, index, arg, "\p" ))
		return false;
	
	if (arg[0]==0)
		return false;

	if (arg[1]<'0')
		return false;
	if (arg[1]>'9')
		return false;

	StringToNum( arg, num );
	
	return true;
}

void ReturnXCMDValue( XCmdBlockPtr params, const char *val )
{
	if (!params->returnValue)
		params->returnValue = NewHandle( 10 );
	
	SetHandleSize( params->returnValue, my_strlen( val )+1 );

	my_strcpy( *params->returnValue, val );
}

void ParseXCMDArguments( XCmdBlockPtr params )
{
	int index = 0;
	
	//	Filename
	if (!ArgGetString( params, index, fileName, "\pSample" ))
	{
		error = "Error: filename parameter is mandatory";
		return;
	}
	index++;

	//	Fullscreen vs window	
	fullscreen = false;
	if (ArgIsString( params, index, "\pwindow"))
	{
		fullscreen = false;
		index++;
	}
	else if (ArgIsString( params, index, "\pfullscreen"))
	{
		index++;
		fullscreen = true;
	}
	
	//	Parse x and y
	if (ArgIsNumber( params, index, &x ))
	{
		index++;
		if (!ArgIsNumber( params, index, &y ))
		{
			error = "Error: two numbers are necessary for positioning playback, found only one";
			return ;
		}
		index++;
	}
	
	//	Parse loop
	if (ArgIsOption( params, index, "\ploop", &loop))
		index++;

	sound = true;

	//	Parse sound
	if (ArgIsOption( params, index, "\psound", &loop))
		index++;

	//	Parse mouse
	if (!fullscreen && ArgIsOption( params, index, "\pmouse", &mouse))
		index++;

	if (index!=params->paramCount)
	{
		error = "Error: I don't understand all the parameters";	//	#### More precise
		return;
	}
}

pascal void main( XCmdBlockPtr params )
{
	GrafPtr port;
	FlimPtr flim;
	Str255 flimName;
	Ptr savePtr = NULL;
	short vRefNum;

	RememberA0();
	SetUpA4();

//	DebugMem();

	InitUtilities();
	MachineInit();
	CodecInit();
	PreferenceLoad();
	BufferInit( PreferenceGetMaxBufferSize(), 30000L + GetScreenSaveSize() );

	InitPlayback();
	HideCursor();
	ScreenInit( gScreen );
//	SaveScreen( &savePtr );

	ComputeMouse();
	DrawMouse();

	ParseXCMDArguments( params );

	if (fullscreen)
		ScreenClear( gScreen );

	if (error)
			ReturnXCMDValue( params, error );
	else
	{
		GetVol( 0, &vRefNum );//?
		flim = FlimOpenByName( fileName, vRefNum, 0/* CurDirStore */, kHFS );
	
		if (!flim)
		{
//			MessageStr( (unsigned char*)"\pCannot open flim file" );
			ReturnXCMDValue( params, "Cannot open flim file" );
		}
		else
		{
			ePlayResult theResult;
			short playback_x;
			short playback_y;

			//	Let's compute the top-left in screen space

			if (fullscreen)
			{
				struct FlimInfo *fi = FlimGetInfo( flim );
				playback_x = (gScreen->width-fi->width)/2;
				playback_y = (gScreen->height-fi->height)/2;;
			}
			else
			{
				WindowPtr fw = FrontWindow();
				playback_x = -fw->portBits.bounds.left+x;
				playback_y = -fw->portBits.bounds.top+y;
			}

			do
			{
				theResult = PlayFlim( flim, playback_x, playback_y, !sound );
			} while (loop && theResult==kDone);		//	Loop over if no button click

			FlimDispos( flim );
		}
	}
	
//	DebugLong( x );
//	DebugLong( y );
	
	RestoreMouse();
//	RestoreScreen( &savePtr );
	ScreenDispos( gScreen );
	ShowCursor();
	DisposBuffer();
	DisposUtilities();

//	DebugMem();

	RestoreA4();
}
