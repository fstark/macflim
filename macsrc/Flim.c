#include "Flim.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Util.h"
#include "Log.h"
#include "Config.h"
#include "Machine.h"
#include "Buffer.h"
#include <stdio.h>
#include "Resources.h"
#include "Preferences.h"

//	-------------------------------------------------------------------
//	FLIM FILE PARSING FUNCTIONS
//	-------------------------------------------------------------------
//	All routines for parsing the flim files
//	and provide "Block" information for playback
//	-------------------------------------------------------------------
//	For good playback, flims are read in blocks. Each of such block
//	contains a certain number of frames. The size of each block depends
//	on the flim and on playback parameters.
//	Block sizes are pre-computed in the flim structure in AccessItems
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Structure that describes each of the blocks for accessing the flim
//	-------------------------------------------------------------------

struct AccessItem
{
	short frameCount;	//	Number of frames in this block
	Size blockSize;		//	Exact size of the block in bytes
};

//	-------------------------------------------------------------------
//	An opened flim
//	-------------------------------------------------------------------

#define MAX_STREAMS 10

typedef enum
{
	kFlimStreamInfo = 0,
	kFlimStreamFlim = 1,
	kFlimStreamToc = 2,
	kFlimStreamPoster = 3,
	
	kFlimStreamDummy = 256	//	To make sure the compiler uses a short to store the enum
}	eFlimStreamType;

//	Stream #0 : Info
//	contains a FlimInfo

struct FlimStream
{
	eFlimStreamType type;
	Size offset;
	Size size;
};

struct FlimRec
{
	short fRefNum;
	Size blockSize;

	unsigned short fletcher16;	//	offset : 1022
	short version;		//	offset : 1024
	short streamCount;	//	offset : 1026

	struct FlimStream streams[MAX_STREAMS];
	
	Size streamOffset;	//	The offset to the end of streams table
	
	struct FlimInfo info;

	short blockCount;
	struct AccessItem *accessTable;
	
	PicHandle poster;
	
	Str32 name;
};

//	-------------------------------------------------------------------

static Size FlimStreamGetOffset( FlimPtr flim, int index )
{
	struct FlimStream *s = flim->streams+index;
	return s->offset+flim->streamOffset;
}

//	-------------------------------------------------------------------

void FlimSeekStart( FlimPtr flim )
{
	OSErr err = SetFPos( flim->fRefNum, fsFromStart, FlimStreamGetOffset( flim, kFlimStreamFlim ) );
	assert( err==noErr, "FlimSeek" );
}

//	-------------------------------------------------------------------

static int FlimReadStream( FlimPtr flim, void *dest, int index )
{
	struct FlimStream *s = flim->streams+index;
	Size readSize;

	SetFPos( flim->fRefNum, fsFromStart, FlimStreamGetOffset( flim, index ) );
	readSize = s->size;
	FSRead( flim->fRefNum, &readSize, dest );
	if (readSize!=s->size)
		Abort( "\pBad flim stream" );

	return readSize;
}

//	-------------------------------------------------------------------

static Ptr FlimReadStreamNewPtr( FlimPtr flim, int index )
{
	Size s = flim->streams[index].size;
	Ptr p = MyNewPtr( s );
	assert( p!=NULL, "Not Enough Memory or bad file" );
	FlimReadStream( flim, p, index );
	return p;
}

//	-------------------------------------------------------------------

static FlimPtr FlimOpen( short fRefNum, Size maxBlockSize )
{
	Size read_size;
	FlimPtr flim = (FlimPtr)NewPtrNoFail( sizeof( struct FlimRec ) );
	Size maxAccessEntries;
	int i;

	flim->fRefNum = fRefNum;
	flim->blockSize = maxBlockSize;
	flim->name[0] = 0;

	flim->accessTable = NULL;
	flim->poster = NULL;


/*	flim->fletcher16 = -1;
	flim->version = -1;
	flim->streamCount = -1;
	flim->streamOffset = -1;
	flim->blockCount = -1;
	flim->accessTable = NULL;
	flim->poster = NULL;
*/

	if (maxBlockSize<=0)
	{
		ParamText( "\pError", "\pBuffer size are too small to play flims. Please correct in preferences.", "", "" );
		UtilModalDialog( kDLOGErrorNonFatal );
		FlimDispos( flim );
		return NULL;
	}

		//	Note: this could be made completely dynamic with a bit of work
	if (MachineIsMinimal())
		maxAccessEntries = 512L;
	else
		maxAccessEntries = 4096L;

		//	Read fletcher, version and stream count
	SetFPos( flim->fRefNum, fsFromStart, 1022 );
	read_size = 6;
	FSRead( fRefNum, &read_size, &flim->fletcher16 );
	if (read_size==0)
	{	//	Empty file, exit
		FlimDispos( flim );
		return NULL;
	}

		//	Check version
	if (flim->version!=1)
	{
		ParamText( "\pError", "\pFile corrupted or wrong version (expected version 1)", "", "" );
		UtilModalDialog( kDLOGErrorNonFatal );
		FlimDispos( flim );
		return NULL;
	}

		//	Check stream count
	if (flim->streamCount>MAX_STREAMS)
	{
		ParamText( "\pError", "\pFile has too many streams (max 10)", "", "" );
		UtilModalDialog( kDLOGErrorNonFatal );
		FlimDispos( flim );
		return NULL;
	}

		//	Read streams
	read_size = flim->streamCount*sizeof(struct FlimStream);
	FSRead( fRefNum, &read_size, flim->streams );

		//	Checks streams are correct according to current spec
	for (i=0;i!=flim->streamCount;i++)
		if (flim->streams[i].type!=i)
		{
			ParamText( "\pError", "\pFile streams are not ordered correctly", "", "" );
			UtilModalDialog( kDLOGErrorNonFatal );
			FlimDispos( flim );
			return NULL;
		}

		//	This is how the stream offsets points to
	flim->streamOffset = 1024 + 2 + 2 + read_size;

		//	Read info
	{
		int res = FlimReadStream( flim, &flim->info, kFlimStreamInfo );

			//	Old flims don't have the codec list, so we force it to "all reasonable"
		if (res==16)
		{	//	1c = 11100 => Copy + Invert + Z32, no Z16, no NULL
			flim->info.codecs = 0x0000001CL;
		}
	}

	{
		short *toc;
		int index;
		Size currentSize;
		long frameCount;
		short blockIndex = 0;

		toc = (short *)FlimReadStreamNewPtr( flim, kFlimStreamToc );

#ifndef MINI_PLAYER // Miniplayer don't support network play for now
		if (PreferencesGetSingleFrameReadAhead())
		{	//	We build an access table with a single frame
			int l = flim->info.frameCount;
			struct AccessItem *p = (struct AccessItem *)NewPtrNoFail( sizeof(struct AccessItem)*l );
			short *q = toc;

			if (!p)
			{
				ParamText( "\pError", "\pNot enough memory to load this flim in network mode.", "", "" );
				UtilModalDialog( kDLOGErrorNonFatal );
				MyDisposPtr( (Ptr)toc );
				FlimDispos( flim );
				return NULL;
			}
			
			flim->accessTable = p;
			flim->blockCount = l;

				//	Blocks size are a single frame
			while (l--)
			{
				p->frameCount = 1;
				p->blockSize = *q++;
				p++;
			}
		}
		else
#endif
		{
			flim->accessTable = (struct AccessItem *)NewPtrNoFail( sizeof(struct AccessItem)*maxAccessEntries );
	
			frameCount = 0;
			currentSize = 0;
			flim->blockCount = 0;
			for (index=0;index!=flim->info.frameCount;index++)
			{
				//	If we need to go to the next block, so be it
				if (currentSize+toc[index]>=flim->blockSize)
				{
					frameCount = 0;
					currentSize = 0;
					blockIndex++;
					if (blockIndex==maxAccessEntries)
					{
						MySetPtrSize( flim->accessTable, sizeof(struct AccessItem)*(maxAccessEntries+1024) );
						if (MemError())
							Abort( "\pNot enough access entries to load TOC" );
						maxAccessEntries += 1024;
					}
				}
	
				frameCount++;
				flim->accessTable[blockIndex].frameCount = frameCount;
				currentSize += toc[index];
				if (currentSize>flim->blockSize)
				{
					ParamText( "\pError", "\pBuffer size is too small to load this flim. Please change buffer size in Preferences.", "", "" );
					UtilModalDialog( kDLOGErrorNonFatal );
					MyDisposPtr( (Ptr)toc );
					FlimDispos( flim );
					return NULL;
				}
				flim->accessTable[blockIndex].blockSize = currentSize;
	
				flim->blockCount = blockIndex+1;
			}
		}

		MyDisposPtr( (Ptr)toc );
	
		//	Give back the extra memory
		MySetPtrSize( flim->accessTable, flim->blockCount*sizeof(struct AccessItem) );
	}

	flim->poster = NULL;

	FlimSeekStart( flim );

	return flim;
}

//	-------------------------------------------------------------------

static sTotalOpen = 0;
static sFlimError = noErr;

OSErr FlimError( void )
{
	return sFlimError;
}

FlimPtr FlimOpenByName( Str255 fName, short vRefNum, long dirID, eFileAPI api )
{
	Size maxBlockSize = BufferGetSize()-64;	//	16 is enough (sizeof BlockRecord)

		//	Open flim (MFS/HFS fashion)
	OSErr err;
	short fRefNum;

	FlimPtr flim = NULL;

	if (api==kMFS)
	{		//	MFS
		ParamBlockRec pb;

		//printf( "FlimOpenByName %d %s\n", vRefNum, fName );

		pb.ioParam.ioCompletion = NULL;
		pb.ioParam.ioNamePtr = fName;
		pb.ioParam.ioVRefNum = vRefNum;
		pb.ioParam.ioVersNum = 0;
		pb.ioParam.ioPermssn = fsRdPerm;
		pb.ioParam.ioMisc = NULL;
		err = PBOpen( &pb, FALSE );
		fRefNum = pb.ioParam.ioRefNum;
	}
	else
	{		//	HFS
		HParamBlockRec pb;

		//printf( "FlimOpenByName %d %s\n", vRefNum, fName );

		pb.ioParam.ioCompletion = NULL;
		pb.ioParam.ioNamePtr = fName;
		pb.ioParam.ioVRefNum = vRefNum;
//		pb.ioParam.ioVersNum = 0;
		pb.ioParam.ioPermssn = fsRdPerm;
		pb.ioParam.ioMisc = NULL;
		pb.fileParam.ioDirID = dirID;
		err = PBHOpen( &pb, FALSE );

		fRefNum = pb.ioParam.ioRefNum;
	}

#ifdef VERBOSE
	printf( "%#s in %d\n", fName, vRefNum );
#endif

	if (err!=noErr)
	{
		sFlimError = err;
		return NULL;
	}
	
	sTotalOpen++;

//	printf( "OPEN:  TOTAL OPEN FLIM:%d\n", sTotalOpen );

	flim = FlimOpen( fRefNum, maxBlockSize );
	
	if (!flim)
	{
		return NULL;
	}

#ifndef MINI_PLAYER
	StrCpyPP( flim->name, fName );
#else
	flim->name[0] = 0;
#endif

	return flim;
}

//	-------------------------------------------------------------------

void FlimDispos( FlimPtr flim )
{
	Size growBytes;
	
	sTotalOpen--;

//	printf( "CLOSE: TOTAL OPEN FLIM:%d\n", sTotalOpen );
	
	if (flim->accessTable) MyDisposPtr( (Ptr)(flim->accessTable) );
	if (flim->poster) DisposHandle( flim->poster );
	FSClose( flim->fRefNum );

	MyDisposPtr( (Ptr)flim );
	MaxMem( &growBytes );
}

//	-------------------------------------------------------------------

Size FlimGetBlockCount( FlimPtr flim )
{
	return flim->blockCount;
}

//	-------------------------------------------------------------------

int FlimGetBlockFrameCount( FlimPtr flim, int index )
{
	return flim->accessTable[index].frameCount;
}

//	-------------------------------------------------------------------

Size FlimGetBlockSize( FlimPtr flim, int index )
{
	return flim->accessTable[index].blockSize;
}

//	-------------------------------------------------------------------

int FlimGetFileRefNum( FlimPtr flim )
{
	return flim->fRefNum;
}

//	-------------------------------------------------------------------

Boolean FlimGetIsSilent( FlimPtr flim )
{
	return flim->info.silent;
}

//	-------------------------------------------------------------------

struct FlimInfo *FlimGetInfo( FlimPtr flim )
{
	return &flim->info;
}

#ifndef MINI_PLAYER

//	-------------------------------------------------------------------

PicHandle FlimCreatePoster( FlimPtr flim )
{
	Ptr poster = FlimReadStreamNewPtr( flim, kFlimStreamPoster );
	BitMap posterBits;
	RgnHandle saveClip;
	PicHandle posterHandle;
	
	posterBits.baseAddr = poster;
	posterBits.rowBytes = 128/8;
	SetRect( &posterBits.bounds, 0, 0, 128, 86 );

//	posterBits.rowBytes = 86/8;
//	SetRect( &posterBits.bounds, 0, 0, 86, 128 );

	saveClip = NewRgn();
	GetClip( saveClip ); 
	ClipRect( &posterBits.bounds );
	posterHandle = OpenPicture( &posterBits.bounds );
	CopyBits( &posterBits, &thePort->portBits, &posterBits.bounds, &posterBits.bounds, srcCopy, 0L );	
	ClosePicture();
	SetClip( saveClip );
	DisposeRgn( saveClip );

	MyDisposPtr( poster );
	
	return posterHandle;
}

//	-------------------------------------------------------------------

PicHandle FlimGetPoster( FlimPtr flim )
{
	if (!flim->poster)
		flim->poster = FlimCreatePoster( flim );	
	return flim->poster;
}

//	-------------------------------------------------------------------

const char *FlimGetName( FlimPtr flim )
{
	return (const char *)flim->name;
}

//	-------------------------------------------------------------------

FlimPtr FlimOpenByNameAnyVolumes( Str255 fName, short vRefNum, long dirID, eFileAPI api )
{
	FlimPtr flim = NULL;
	
	flim = FlimOpenByName( fName, vRefNum, dirID, api );

	if (!flim)
	{
		QElemPtr drives;
		
		for (drives = GetDrvQHdr()->qHead;drives;drives=drives->qLink)
		{
			OSErr err;
			Str255 volName;
			short newVRefNum;
			long freeBytes;

			err = GetVInfo( ((DrvQEl*)drives)->dQDrive, volName, &newVRefNum, &freeBytes );

			if (!err && vRefNum!=newVRefNum)	//	Ignoring errors seems the most logical play
			{
				flim = FlimOpenByName( fName, newVRefNum, dirID, api );
				if (flim)
					break;
			}
		}
	}
	return flim;
}

#else

const char *FlimGetName( FlimPtr flim ) { return ""; }

#endif

