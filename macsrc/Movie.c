#include "Movie.h"

#include "Util.h"
#include "Log.h"
#include "Config.h"
#include "Machine.h"
#include <stdio.h>


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
	Size blockSize;		//	Exact size of the block in bytes
};

//	-------------------------------------------------------------------
//	An opened flim
//	-------------------------------------------------------------------

struct MovieRec
{
	short fRefNum;
	Size blockSize;

	short fletcher16;

	short version;
	Size tocOffset;
	long frameCount;
	short rowBytes;	//	unused
	short vLines;	//	unused
	Boolean silent;
	char dummy[64-2-4-4-2-2-1];	//	64 bytes are reserved for header/future use

	short blockCount;
	struct AccessItem *accessTable;
};

//	-------------------------------------------------------------------

MoviePtr MovieOpen( short fRefNum, Size maxBlockSize )
{
	Size read_size;
	MoviePtr movie = (MoviePtr)NewPtrNoFail( sizeof( struct MovieRec ) );
	Size maxAccessEntries;

		//	Note: this could be made completely dynamic with a bit of work
	if (MachineIsMinimal())
		maxAccessEntries = 512L;
	else
		maxAccessEntries = 4096L;

	movie->fRefNum = fRefNum;
	movie->blockSize = maxBlockSize;
	
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
		Size currentSize;
		long frameCount;
		short blockIndex = 0;

		read_size = movie->frameCount*sizeof(short);
		toc = (short *)NewPtrNoFail( read_size );
		FSRead( fRefNum, &read_size, (Ptr)toc );
		if (read_size!=movie->frameCount*sizeof(short))
			Abort( "\pCannot read FLIM table of content" );

		movie->accessTable = (struct AccessItem *)NewPtrNoFail( sizeof(struct AccessItem)*maxAccessEntries );

		frameCount = 0;
		currentSize = 0;
		movie->blockCount = 0;
		for (index=0;index!=movie->frameCount;index++)
		{
			//	If we need to go to the next block, so be it
			if (currentSize+toc[index]>=movie->blockSize)
			{
				frameCount = 0;
				currentSize = 0;
				blockIndex++;
				if (blockIndex==maxAccessEntries)
				{
					SetPtrSize( movie->accessTable, sizeof(struct AccessItem)*(maxAccessEntries+1024) );
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
			movie->accessTable[blockIndex].frameCount = frameCount;
			currentSize += toc[index];
			movie->accessTable[blockIndex].blockSize = currentSize;

			movie->blockCount = blockIndex+1;
		}
		
//		movie->maxBlockSize = maxBlockSize;

		DisposPtr( (Ptr)toc );
	}

	//	Give back the extra memory
	SetPtrSize( movie->accessTable, movie->blockCount*sizeof(struct AccessItem) );

	MovieSeekStart( movie );

	return movie;
}

//	-------------------------------------------------------------------

void MovieDispos( MoviePtr movie )
{
	Size growBytes;
	
	DisposPtr( (Ptr)(movie->accessTable) );
	DisposPtr( (Ptr)movie );
	MaxMem( &growBytes );
}

//	-------------------------------------------------------------------

void MovieSeekStart( MoviePtr movie )
{
	OSErr err = SetFPos( movie->fRefNum, fsFromStart, 1024+64 );
	assert( err==noErr, "MovieSeek" );
}

//	-------------------------------------------------------------------

Size MovieGetBlockCount( MoviePtr movie )
{
	return movie->blockCount;
}

//	-------------------------------------------------------------------

int MovieGetBlockFrameCount( MoviePtr movie, int index )
{
	return movie->accessTable[index].frameCount;
}

//	-------------------------------------------------------------------

Size MovieGetBlockSize( MoviePtr movie, int index )
{
	return movie->accessTable[index].blockSize;
}

//	-------------------------------------------------------------------

int MovieGetFileRefNum( MoviePtr movie )
{
	return movie->fRefNum;
}

//	-------------------------------------------------------------------

Boolean MovieGetSilent( MoviePtr movie )
{
	return movie->silent;
}



	