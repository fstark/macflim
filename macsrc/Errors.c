#include "Errors.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Resources.h"
#include "Util.h"
#include "stdio.h"
#include "string.h"

//	-------------------------------------------------------------------

void ErrorCannotOpenFlimFile(
	OSErr err,
	const char *fileName,
	long vRefNum,
	long dirID
	)
{
	Str255 errStr;
	Str255 vRefNumStr;
	Str255 dirIDStr;
	
	NumToString( err, errStr );
	NumToString( vRefNum, vRefNumStr );
	NumToString( dirID, dirIDStr );
	
	ParamText( fileName, errStr, vRefNumStr, dirIDStr );
	
	UtilDialog( kDLOGOpenFlimErrorID );
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
	flimResolutionStr[0] = strlen( flimResolutionStr+1 );
	sprintf( screenResolutionStr+1, "%ux%u", screenWidth, screenHeight );
	screenResolutionStr[0] = strlen( screenResolutionStr+1 );
	
	ParamText( name, flimResolutionStr, screenResolutionStr, "" );
	
	UtilDialog( kDLOGScreenTooSmallErrorID );
}

//	-------------------------------------------------------------------

void InfoIntegritySuccess( void )
{
	ParamText( "", "", "", "" );
	UtilDialog( kDLOGFileIntegrityOkID );
}

//	-------------------------------------------------------------------

void InfoAutoPlaySuccess( void )
{
	ParamText( "", "", "", "" );
	UtilDialog( kDLOGAutoPlayOkID );
}

