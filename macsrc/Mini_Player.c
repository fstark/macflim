#include "Screen.h"
#include "Playback.h"
#include "Preferences.h"
#include "Util.h"

int main()
{
	Ptr savePtr = NULL;
	OSErr err;
	SFReply theReply;
	Point where;
	Boolean saveScreen;
	Size reservedSpace;

		//	Mac toolbox init
	InitGraf( &thePort );
	MachineInit();
	CodecInit();

	if (MachineIsMinimal())
	{
		//	Basically a MacXL or an original Mac128K
		saveScreen = FALSE;			//	Not needed due lack of multi-finder
		reservedSpace = 10000L;		//	10K (8K is the current minimal)
									//	This currently let us have 2x19Kb buffers on a Mac128
	}
	else
	{
		/* Normal memory usage parameters */
		/* User may be asked to increase the MultiFinder partition */
		MaxApplZone();
		saveScreen = TRUE;
		reservedSpace = 30000L + GetScreenSaveSize() + 30000L /* in case of codec mapping table */;
	}

	PreferenceLoad();

	InitFonts();
	FlushEvents( everyEvent, 0 );
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( NULL );
	InitUtilities();
	BufferInit( PreferenceGetMaxBufferSize(), reservedSpace );
	InitCursor();

	HideCursor();
	

	if (!MachineIsBlackAndWhite())
	{
		//	Not a BW screen
		return 0;
	}

	InitPlayback();

	//	The fancy version
	//	Set up screen
	gScreen = ScreenInit( gScreen );

	if (saveScreen)
		SaveScreen( &savePtr );

	{
		Str255 apName;
		short resRefNum;
		Handle hParms;
		ePlayResult theResult;

		GetAppParms(apName, &resRefNum, &hParms);
		
		if (PlayFlimFileLoop( apName, 0, -1, kMFS, FALSE )==kFileError)
		{
				//	Ask user for a file
			where.h = 20;
			where.v = 90;
			ShowCursor();
			SFGetFile( where, 0, NULL, -1, NULL, 0, &theReply );
			
			if (theReply.good)
			{
				HideCursor();
				PlayFlimFileLoop( theReply.fName, theReply.vRefNum, -1, kMFS, FALSE );
			}
		}
	}

	if (saveScreen)
		RestoreScreen( &savePtr );

	ShowCursor();

	BufferDispos();

	FlushEvents( everyEvent, 0 );

	return 0;
}