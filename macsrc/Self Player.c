#include "Self Player.h"

#include "Util.h"

#include <stdio.h>
#include <string.h>

typedef struct ResourceSpec
{
	OSType type;
	short id;
};

typedef struct CopyRecord
{
	short version;
	short count;
	struct ResourceSpec resources[1];
};

Boolean SelfGetMiniPlayer()
{
	short dest = CurResFile();
	short source = OpenResFile( "\pMini MacFlim" );
	int i,j;
	int selfIndex = 0;
	Handle h;
	struct CopyRecord **sCopyHdl;

	if (source==-1)
	{
		return FALSE;
	}

	//	Delete the 'COPY' resource
	UseResFile( dest );
	h = Get1Resource( 'COPY', 128 );
	RmveResource( h );
	DisposHandle( h );

	//	and all the 'SELF' resources
	while (Count1Resources('SELF')!=0)
	{
		Handle h = Get1IndResource( 'SELF', 1 );
		if (ResError()) return FALSE;
		RmveResource( h );
		if (ResError()) return FALSE;
		DisposHandle( h );
		if (ResError()) return FALSE;
	}
	UpdateResFile( dest );

	UseResFile( source );

	//	Create the right 'COPY' resource
	sCopyHdl = (struct CopyRecord **)NewHandle( 1024L );	//	Hopefully large enough
	HLock( sCopyHdl );

	for (i=0;i!=Count1Types();i++)
	{
		ResType rType;

		Get1IndType( &rType, i+1 );

		for (j=0;j!=Count1Resources(rType);j++)
		{
			Handle h = Get1IndResource( rType, j+1 );
			short id;
			ResType rTypeDummy;
			Str255 rName;

			GetResInfo( h, &id, &rTypeDummy, rName );
//			printf( "%d: %.4s #%d\n", selfIndex, &rType, id );
			UseResFile( dest );
			DetachResource( h );

			sprintf( (char *)rName+1, "%.4s #%d", &rType, id );
			rName[0] = strlen( (char *)rName+1 );
			
			AddResource( h, 'SELF', selfIndex, rName );
			WriteResource( h );
			ReleaseResource( h );
			UseResFile( source );

			(*sCopyHdl)->resources[selfIndex].type = rType;
			(*sCopyHdl)->resources[selfIndex].id = id;
			selfIndex++;
		}
	}

		//	Handle header + size + save resource
	(*sCopyHdl)->version = 0x01;
	(*sCopyHdl)->count = selfIndex;
	HUnlock( sCopyHdl );
	SetHandleSize( sCopyHdl, 4+(*sCopyHdl)->count*sizeof( (*sCopyHdl)->resources[0] ) );

	UseResFile( dest );
	AddResource( sCopyHdl, 'COPY', 128, "\p" );
	WriteResource( sCopyHdl );
	UseResFile( source );

	CloseResFile( source );

	return TRUE;
}

Boolean SelfInstallPlayer( Str255 fName, short vRefNum, short dirID )
{
	struct CopyRecord **sCopyHdl;
	struct ResourceSpec *resSpec;
	short source = CurResFile();
	short dest;
	int i;
	short saveRefNum;
	long saveDirID;
	Boolean result = TRUE;

	HGetVol( NULL, &saveRefNum, &saveDirID );
	HSetVol( NULL, vRefNum, dirID );
	
	CreateResFile( fName );
	dest = OpenResFile( fName );
	
	if (dest==-1)
	{
		result = FALSE;
		goto done;
	}

	sCopyHdl = (struct CopyRecord **)GetResource( 'COPY', 128 );

	if (!sCopyHdl)
	{
		result = FALSE;
		goto done1;
	}

	HLock( sCopyHdl );

	resSpec = (*sCopyHdl)->resources;

	for (i=0;i!=(*sCopyHdl)->count;i++)
	{
		Handle h = GetResource( 'SELF', i );
		DetachResource( h );
		AddResource( h, resSpec->type, resSpec->id, "\p" );
		resSpec++;
	}

	HUnlock( sCopyHdl );
	
	UtilSetFileTypeCreator( fName, vRefNum, dirID, 'APPL', 'MMFL' );

done1:
	CloseResFile( dest );

done:
	HSetVol( NULL, saveRefNum, dirID );

	return result;
}

void SelfInstallPlayerUI()
{
	Point where;
	SFReply theReply;

	SelfGetMiniPlayer();	//	Try to copy the miniplayer resources

		//	Ask user for a file
	where.h = 20;
	where.v = 90;
	ShowCursor();
	SFGetFile( where, 0, NULL, -1, NULL, 0, &theReply );
	
	if (theReply.good)
	{
		OSType dummy;
		short vRefNum;
		long dirID;
		OSErr err;

		err = GetWDInfo( theReply.vRefNum, &vRefNum, &dirID, &dummy );
		CheckErr( err, "GetWDInfo" );

		SelfInstallPlayer( theReply.fName, vRefNum, dirID );
	}
}


