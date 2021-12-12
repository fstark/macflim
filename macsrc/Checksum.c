#include "Checksum.h"

#include "Util.h"

//	-------------------------------------------------------------------
//	Returns TRUE is flim file contains a checksum
//	-------------------------------------------------------------------

unsigned short FlimChecksum( Str255 fName, short vRefNum );
unsigned short FlimChecksum( Str255 fName, short vRefNum )
{
	unsigned fletcher = 0xffff;
	OSErr err;
	short fRefNum;

		//	Open flim
//	err = FSOpen( fName, vRefNum, &fRefNum );
	{
		ParamBlockRec pb;
		pb.ioParam.ioCompletion = NULL;
		pb.ioParam.ioNamePtr = fName;
		pb.ioParam.ioVRefNum = vRefNum;
		pb.ioParam.ioVersNum = 0;
		pb.ioParam.ioPermssn = fsRdPerm;
		pb.ioParam.ioMisc = NULL;
		err = PBOpen( &pb, FALSE );
		fRefNum = pb.ioParam.ioRefNum;
	}
	CheckErr( err, "FSOpen" );

		//	Skip first Kb of comments
	SetFPos( fRefNum, fsFromStart, 1022 );
	if (err==noErr)
	{
			//	Read Fletcher-16
		long read_size = 2;
		err = FSRead( fRefNum, &read_size, &fletcher );
		CheckErr( err, "FSRead" );
	}

	err = FSClose( fRefNum );
	CheckErr( err, "FSClose" );
	
	return fletcher;
}

//	-------------------------------------------------------------------
//	Reference implementation of fletcher16 checksum
//	Adds all words, modulo 65535
//	-------------------------------------------------------------------

unsigned short fletcher16_ref( register unsigned short fletcher, register unsigned short *buffer, long count );
unsigned short fletcher16_ref( register unsigned short fletcher, register unsigned short *buffer, long count )
{
	register long i;
	register long f = fletcher;

	for (i=0;i!=count;i++)
	{
		f += buffer[i];
		if (f>=65535)
			f -= 65535;
	}
	
	return f;
}

//	-------------------------------------------------------------------
//	Assembly implemetation of fletcher16
//	Works on buffer of 1 to 65536 words (pass 0 for count)
//	-------------------------------------------------------------------

unsigned short fletcher16_short( unsigned short fletcher, unsigned short *buffer, unsigned short count );
unsigned short fletcher16_short( unsigned short fletcher, unsigned short *buffer, unsigned short count )
{
	asm{
	
		move.w fletcher,d0
		movea.l buffer,a0
		move.w count,d1
		subq.w #1,d1

@loop:
		add.w (a0)+,d0
		bcc @skip
		addq.w #1,d0
@skip:
		dbra d1,@loop
		move.w d0,fletcher
	}
	

	return fletcher;
}

//	-------------------------------------------------------------------
//	Wrapper around assembly version of fletcher
//	-------------------------------------------------------------------

unsigned short fletcher16( unsigned short fletcher, unsigned short *buffer, long count );
unsigned short fletcher16( unsigned short fletcher, unsigned short *buffer, long count )
{
	while (count>65536	)
	{
		fletcher = fletcher16_short( fletcher, buffer, 65536 );	//	Note: 65536 is passed as 0
		count -= 65536;
		buffer += 65536;
	}
	return fletcher16_short( fletcher, buffer, count );
}

//	-------------------------------------------------------------------
//	Performs the integrity check, with skippable dialog
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrity( Str255 fName, short vRefNum, unsigned short fletcher_ref );
Boolean CheckFlimIntegrity( Str255 fName, short vRefNum, unsigned short fletcher_ref )
{
	OSErr err;
	short fRefNum;
	long read_size;
	unsigned short *theBuffer = NULL;
	unsigned short fletcher;
	long total_size = 0;
	long size = 1024;	//	The header we skip
	int percent;
	int last_percent = -1;
	DialogPtr theProgress = NULL;
	Handle thePercentText;
	short dummy0;
	Rect dummy1;
	Boolean skip = false;

	//	Check integrity by 20K blocks
#define BUFFER_SIZE 20000

#define kProgressDlog 129
#define kProgressItem 1

		//	Open flim
//	err = FSOpen( fName, vRefNum, &fRefNum );
	{
		ParamBlockRec pb;
		pb.ioParam.ioCompletion = NULL;
		pb.ioParam.ioNamePtr = fName;
		pb.ioParam.ioVRefNum = vRefNum;
		pb.ioParam.ioVersNum = 0;
		pb.ioParam.ioPermssn = fsRdPerm;
		pb.ioParam.ioMisc = NULL;
		err = PBOpen( &pb, FALSE );
		fRefNum = pb.ioParam.ioRefNum;
	}
#ifdef VERBOSE
	printf( "%#s in %d\n", fName, vRefNum );
#endif
	CheckErr( err, "Open" );

		//	Get file size
	err = GetEOF( fRefNum, &total_size );
	CheckErr( err, "GetEOF" );

		//	Skip first Kb of comments
	err = SetFPos( fRefNum, fsFromStart, 1024 );
	CheckErr( err, "SetFPos" );

	fletcher = 0;
	
	while (Button())
		;

	theProgress = GetNewDialog( kProgressDlog, NULL, (WindowPtr)-1 );
	GetDItem( theProgress, kProgressItem, &dummy0, &thePercentText, &dummy1 );
	SetIText( thePercentText, "" );
	ShowWindow( theProgress );
	DrawDialog( theProgress );

	//	Allocate late, to reduce memory fragmentation
	theBuffer = (unsigned short *)NewPtr( BUFFER_SIZE );

	while (1)
	{
		register long i;
	
		read_size = BUFFER_SIZE;
		err = FSRead( fRefNum, &read_size, theBuffer );
		if (err!=noErr && err!=eofErr)
			CheckErr( err, "FSRead" );

		size += read_size;

		if (read_size!=BUFFER_SIZE)	//	Truncate last byte is needed
			read_size -= read_size&1;

		percent = size*100.0/total_size;
		if (percent!=last_percent)
		{
			Str255 theBuffer;
			last_percent = percent;
			NumToString( percent, theBuffer );
			StrCatPC( theBuffer, "% complete" );
			SetIText( thePercentText, theBuffer );
		}

		fletcher = fletcher16( fletcher, theBuffer, read_size/2 );

		if (err==eofErr)
			break;
			
		if (Button())
		{
			skip= true;
			break;
		}
	};

done:
	FSClose( fRefNum );

	if (theBuffer) DisposPtr( theBuffer );

	if (theProgress) DisposDialog( theProgress );

	if (!skip)
	{
		if (fletcher_ref==0xffff)
		{
			ShowCursor();
			ParamText( "\pFlim have no checksum", "", "", "" );
			NoteAlert( 130, NULL );
			HideCursor();
		}
		else if (fletcher!=fletcher_ref)
		{
			ShowCursor();
			ParamText( "\pFlim is corrupted", "", "", "" );
			StopAlert( 131, NULL );
			HideCursor();
	
			return FALSE;
		}
	}

	return TRUE;
}

//	-------------------------------------------------------------------
//	Checks flim integrity if possible and accepted by user
//	-------------------------------------------------------------------

Boolean CheckFlimIntegrityIfNeeded( Str255 fName, short vRefNum )
{
	unsigned short checksum = FlimChecksum( fName, vRefNum );
	if (checksum!=0xffff)
	{
		DialogPtr theCheckDialog = NULL;
		short itemHit;

		theCheckDialog = GetNewDialog( 131, NULL, (WindowPtr)-1 );
		ShowWindow( theCheckDialog );
		ModalDialog( NULL, &itemHit );
		DisposDialog( theCheckDialog );
		if (itemHit==2)
		{
			if (!CheckFlimIntegrity( fName, vRefNum, checksum ))
				return FALSE;
		}
	}
	
	return TRUE;
}
