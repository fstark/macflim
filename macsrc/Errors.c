#include "Errors.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Resources.h"
#include "Preferences.h"
#include "Util.h"
#include "stdio.h"
#include "string.h"

//	-------------------------------------------------------------------

int ErrorCannotOpenFlimFile(
	OSErr err,
	const char *fileName,
	short vRefNum,
	long dirID
	)
{
//	ParamText( fileName, errStr, vRefNumStr, dirIDStr );
	
	DialogPtr dialog = UtilPlaceDialog( kDLOGOpenFlimErrorID );
	short itemHit;

	ParamText( fileName, "", "", "" );

	do
	{
		ModalDialog( NULL, &itemHit );
		if (PreferencesGetDebugMenu() && itemHit==kOpenFlimErrorDebugTextID)
			ErrorDebugFile( err, (void*)fileName, vRefNum, dirID );
	} while (itemHit>=kOpenFlimErrorDebugTextID);

	DisposDialog( dialog );

	return itemHit-1;
}

//	-------------------------------------------------------------------

void ErrorScreenTooSmall(
	const char *name,
	unsigned short flimWidth,
	unsigned short flimHeight,
	unsigned short screenWidth,
	unsigned short screenHeight
	)
{
	char flimResolutionStr[256];
	char screenResolutionStr[256];

	sprintf( flimResolutionStr+1, "%ux%u", flimWidth, flimHeight );
	flimResolutionStr[0] = my_strlen( flimResolutionStr+1 );
	sprintf( screenResolutionStr+1, "%ux%u", screenWidth, screenHeight );
	screenResolutionStr[0] = my_strlen( screenResolutionStr+1 );

	ParamText( name, flimResolutionStr, screenResolutionStr, "" );
	
	UtilModalDialog( kDLOGScreenTooSmallErrorID );
}

//	-------------------------------------------------------------------

void ErrorDebugFile(
	OSErr err,
	Str255 fileName,
	long vRefNum,
	long dirID
	)
{
	Str255 errStr;
	Str255 vRefNumStr;
	Str255 dirIDStr;
	
	UtilErrToString( err, errStr );
	NumToString( vRefNum, vRefNumStr );
	NumToString( dirID, dirIDStr );
	
	ParamText( fileName, errStr, vRefNumStr, dirIDStr );
	
	UtilModalDialog( kDLOGDebugFileErrorID );
}

//	-------------------------------------------------------------------

void InfoIntegritySuccess( void )
{
	ParamText( "", "", "", "" );
	UtilModalDialog( kDLOGFileIntegrityOkID );
}

//	-------------------------------------------------------------------

void InfoAutoPlaySuccess( void )
{
	ParamText( "", "", "", "" );
	UtilModalDialog( kDLOGAutoPlayOkID );
}

