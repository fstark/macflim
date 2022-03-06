#include "Debug.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "Util.h"
#include "Resources.h"

//	-------------------------------------------------------------------
//	STATICS
//	-------------------------------------------------------------------

static GrafPtr sDebugPort;
static Rect sDebugRect;
static Ptr sFillMem;
static Boolean sDebugOn = FALSE;

//	-------------------------------------------------------------------

void DebugUpdateMem()
{
	char buffer[255];
	GrafPtr savePort;
	static long sLastMem;
	
	if (sLastMem==FreeMem())
		return;

	DebugFillMem();

	sLastMem = FreeMem();

	GetPort( &savePort );

	sprintf( buffer+1, "%ld", sLastMem );
	buffer[0] = strlen( buffer+1 );
	
	if (!sDebugPort)
	{
		sDebugPort = (GrafPtr)MyNewPtr( sizeof( GrafPort ) );
		OpenPort( sDebugPort );
		SetRect( &sDebugRect,	sDebugPort->portRect.right-100, 4, 
								sDebugPort->portRect.right-40, 15 );
	}

	SetPort( sDebugPort );

	FillRect( &sDebugRect, white );

	PenPat( black );
	MoveTo( sDebugRect.left, sDebugRect.bottom-1 );

	DrawString( buffer );

	SetPort( savePort );
}

//	-------------------------------------------------------------------

void DebugMemUpdateClear( void )
{
	if (sDebugPort)
	{
		GrafPtr savePort;

		GetPort( &savePort );
		SetPort( sDebugPort );
		FillRect( &sDebugRect, white );
		SetPort( savePort );

		DisposPtr( sDebugPort );
		sDebugPort = NULL;
	}
}

//	-------------------------------------------------------------------

void DebugUpdate( void )
{
	if (sDebugOn)
	{
		DebugUpdateMem();
	}
	else
		if (sDebugPort)
			DebugMemUpdateClear();
}

//	-------------------------------------------------------------------

void DebugOnOff( void )
{
	sDebugOn = !sDebugOn;
}

//	-------------------------------------------------------------------

Boolean DebugIsOn( void )
{
	return sDebugOn;
}

//	-------------------------------------------------------------------

void DebugFillMem( void )
{
	Size s = 32768;
	while (s>=4)
	{
		Ptr p = MyNewPtr( s );
		if (p)
		{
			*(Ptr *)p = sFillMem;
			sFillMem = p;
		}
		else
			s /= 2;
	}
	
	while (sFillMem)
	{
		Ptr p = *(Ptr *)sFillMem;
		MyDisposPtr( sFillMem );
		sFillMem = p;
	}
}

//	-------------------------------------------------------------------

MenuHandle sDebugMenu;

void DebugAddMenu( void )
{
	return ;

	sDebugMenu = NewMenu( kMENUDebugID, "\pDebug" );
	AppendMenu( sDebugMenu, "\pShow Free Memory" );
	
	InsertMenu( sDebugMenu, 0 );
}


