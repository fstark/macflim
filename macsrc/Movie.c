#include "Movie.h"

#include "Util.h"
#include "Log.h"


//	-------------------------------------------------------------------
//	FLIM FILE PARSING FUNCTIONS
//	-------------------------------------------------------------------
//	All routines for parsing the flim files
//	and provide "Block" information for playback
//	-------------------------------------------------------------------
//	For good playback, movies are read in blocks. Each of such block
//	contains a certain number of frames. The size of each block depends
//	on the flim and on playback parameters.
//	Block sizes are pre-computed in the flim structure in AccessItems
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Structure that describes each of the blocks for accessing movie
//	-------------------------------------------------------------------

struct AccessItem
{
	short frameCount;	//	Number of frames in this block
	long blockSize;		//	Exact size of the block in bytes
};

//	-------------------------------------------------------------------
//	An opened flim
//	-------------------------------------------------------------------

struct MovieRec
{
	short fRefNum;
	long maxBlockSize;

	short fletcher16;

	short version;
	long tocOffset;
	long frameCount;
	char dummy[64-2-4-4];	//	64 bytes are reserved for header/future use

	short blockCount;
	struct AccessItem *accessTable;
};


//	-------------------------------------------------------------------

#define MAX_ACCESS_ENTRIES	512	//	Internal limit that should be lifted

//	-------------------------------------------------------------------

MoviePtr MovieOpen( short fRefNum, long maxBlockSize )
{
	long read_size;
	MoviePtr movie = (MoviePtr)NewPtrNoFail( sizeof( struct MovieRec ) );

	movie->fRefNum = fRefNum;
	movie->maxBlockSize = maxBlockSize;
	
	SetFPos( movie->fRefNum, fsFromStart, 1022 );
	read_size = 2;
	FSRead( fRefNum, &read_size, &movie->fletcher16 );

	read_size = 64;

	FSRead( fRefNum, &read_size, &movie->version );
//	also reads:
//		movie->tocOffset
//		movie->frameCount
//	and the dummy buffer

	SetFPos( movie->fRefNum, fsFromStart, movie->tocOffset+1024 );
	
	{
		short *toc;
		int index;
		long maxBlockSize = 0;
		long currentSize;
		long frameCount;
		short blockIndex = 0;

		read_size = movie->frameCount*sizeof(short);
		toc = (short *)NewPtrNoFail( read_size );
		FSRead( fRefNum, &read_size, (Ptr)toc );
		if (read_size!=movie->frameCount*sizeof(short))
			Abort( "\pCannot read FLIM table of content" );

		movie->accessTable = (struct AccessItem *)NewPtrNoFail( sizeof(struct AccessItem)*MAX_ACCESS_ENTRIES );

		frameCount = 0;
		currentSize = 0;
		movie->blockCount = 0;
		for (index=0;index!=movie->frameCount;index++)
		{
			frameCount++;
			currentSize += toc[index];

			if (maxBlockSize<currentSize)
				maxBlockSize = currentSize;

			assert( blockIndex<=MAX_ACCESS_ENTRIES, "Toc too large, or buffer too small" );
			movie->accessTable[blockIndex].frameCount = frameCount;
			movie->accessTable[blockIndex].blockSize = currentSize;
			movie->blockCount = blockIndex+1;

			if (currentSize+toc[index]>=movie->maxBlockSize)
			{
				frameCount = 0;
				currentSize = 0;
				blockIndex++;
			}
		}
		
		movie->maxBlockSize = maxBlockSize;

		DisposPtr( (Ptr)toc );
	}

	MovieSeekStart( movie );

	return movie;
}

//	-------------------------------------------------------------------

void MovieDispos( MoviePtr movie )
{
	DisposPtr( (Ptr)(movie->accessTable) );
	DisposPtr( (Ptr)movie );
}

//	-------------------------------------------------------------------

void MovieSeekStart( MoviePtr movie )
{
	OSErr err = SetFPos( movie->fRefNum, fsFromStart, 1024+64 );
	assert( err==noErr, "MovieSeek" );
}

//	-------------------------------------------------------------------

int MovieGetBlockCount( MoviePtr movie )
{
	return movie->blockCount;
}

//	-------------------------------------------------------------------

int MovieGetMaxBlockSize( MoviePtr movie )
{
	return movie->maxBlockSize;
}

//	-------------------------------------------------------------------

int MovieGetBlockFrameCount( MoviePtr movie, int index )
{
	return movie->accessTable[index].frameCount;
}

//	-------------------------------------------------------------------

int MovieGetBlockSize( MoviePtr movie, int index )
{
	return movie->accessTable[index].blockSize;
}

//	-------------------------------------------------------------------

int MovieGetFileRefNum( MoviePtr movie )
{
	return movie->fRefNum;
}

	