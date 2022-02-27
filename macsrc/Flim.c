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

static void FlimReadStream( FlimPtr flim, void *dest, int index )
{
	struct FlimStream *s = flim->streams+index;
	Size readSize;

	SetFPos( flim->fRefNum, fsFromStart, FlimStreamGetOffset( flim, index ) );
	readSize = s->size;
	FSRead( flim->fRefNum, &readSize, dest );
	if (readSize!=s->size)
		Abort( "\pBad flim stream" );
}

//	-------------------------------------------------------------------

static Ptr FlimReadStreamNewPtr( FlimPtr flim, int index )
{
	Size s = flim->streams[index].size;
	Ptr p = NewPtr( s );
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

	if (maxBlockSize<=0)
	{
		ParamText( "\pError", "\pBuffer size are too small to play flims. Please correct in preferences.", "", "" );
		UtilDialog( kALRTErrorNonFatal );
		return NULL;
	}

	flim->fRefNum = fRefNum;
	flim->blockSize = maxBlockSize;

/*	flim->fletcher16 = -1;
	flim->version = -1;
	flim->streamCount = -1;
	flim->streamOffset = -1;
	flim->blockCount = -1;
	flim->accessTable = NULL;
	flim->poster = NULL;
*/

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
		DisposPtr( flim );
		return NULL;
	}

		//	Check version
	if (flim->version!=1)
	{
		ParamText( "\pError", "\pFile corrupted or wrong version (expected version 1)", "", "" );
		UtilDialog( kALRTErrorNonFatal );
		DisposPtr( flim );
		return NULL;
	}

		//	Check stream count
	if (flim->streamCount>MAX_STREAMS)
	{
		ParamText( "\pError", "\pFile has too many streams (max 10)", "", "" );
		UtilDialog( kALRTErrorNonFatal );
		DisposPtr( flim );
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
			UtilDialog( kALRTErrorNonFatal );
			DisposPtr( flim );
			return NULL;
		}

		//	This is how the stream offsets points to
	flim->streamOffset = 1024 + 2 + 2 + read_size;

		//	Read info
	FlimReadStream( flim, &flim->info, kFlimStreamInfo );

	{
		short *toc;
		int index;
		Size currentSize;
		long frameCount;
		short blockIndex = 0;

		toc = (short *)FlimReadStreamNewPtr( flim, kFlimStreamToc );

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
					SetPtrSize( flim->accessTable, sizeof(struct AccessItem)*(maxAccessEntries+1024) );
					if (MemError())
						Abort( "\pNot enough access entries to load TOC" );
					maxAccessEntries += 1024;
				}
			}


/*
			if (maxBlockSize<currentSize)
			{
				maxBlockSize = currentSize;
#ifdef VERBOSE
				printf( "MAX BLOCK SIZE %ld\n", maxBlockSize );
#endif
			}
*/

			frameCount++;
			flim->accessTable[blockIndex].frameCount = frameCount;
			currentSize += toc[index];
			if (currentSize>flim->blockSize)
			{
				ParamText( "\pError", "\pBuffer size is too small to load this flim. Please change buffer size in Preferences.", "", "" );
				UtilDialog( kALRTErrorNonFatal );
				DisposPtr( (Ptr)toc );
				DisposPtr( flim );
				return NULL;
			}
			flim->accessTable[blockIndex].blockSize = currentSize;

			flim->blockCount = blockIndex+1;
		}
		
//		flim->maxBlockSize = maxBlockSize;

		DisposPtr( (Ptr)toc );
	}

	//	Give back the extra memory
	SetPtrSize( flim->accessTable, flim->blockCount*sizeof(struct AccessItem) );

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

	return FlimOpen( fRefNum, maxBlockSize );
}

//	-------------------------------------------------------------------

void FlimDispos( FlimPtr flim )
{
	Size growBytes;
	
	sTotalOpen--;

//	printf( "CLOSE: TOTAL OPEN FLIM:%d\n", sTotalOpen );
	
	DisposPtr( (Ptr)(flim->accessTable) );
	DisposHandle( flim->poster );
	FSClose( flim->fRefNum );

	DisposPtr( (Ptr)flim );
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

	DisposPtr( poster );
	
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

#endif

