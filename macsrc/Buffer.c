#include "Buffer.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include "Log.h"
#include "Util.h"
#include <stdio.h>

//	-------------------------------------------------------------------
//	GLOBALS
//	-------------------------------------------------------------------

static Ptr sBuffer[2];
Size sBufferSize;

//	-------------------------------------------------------------------
//	Finds and allocated the optimal buffer size
//	-------------------------------------------------------------------

void BufferInit( Size maxSize, Size leftBytes )
{
	Size growBytes;
	Str255 str;
	Size mem;
	
	if (!maxSize || maxSize>300000)
		maxSize = 300000;

		//	Due to System 2.0 on 64K ROM, we can't have MaxApplZone
		//	So MaxMem returns the size in the Appl zone, and growBytes
		//	the maximal extension
	mem = MaxMem(&growBytes);

		//	As we didn't deallocate anything, the available memory is
		//	located at the end of the zone, and will coalesce with growBytes,
		//	so the sum of both should be available
	mem += growBytes;

		//	leftBytes is the amount of bytes we need for proper execution
		//	There are very few allocations in the code
		//	and it should be graceful when failing to allocate
		//	The biggest need is to load the TOC of flims (up to 2 bytes per frame)
		//	aka 7.2Kb/mins, and the data for the "access" data structure (<TOC)

		//	How much we can use for buffers
	mem = mem-leftBytes;

		//	Size of each buffer
	sBufferSize = mem/2;

		//	Reduce buffers to the needed size
	if (sBufferSize>maxSize)
		sBufferSize = maxSize;

		//	Allocate
	sBuffer[0] = MyNewPtr( sBufferSize );
	sBuffer[1] = MyNewPtr( sBufferSize );

	if (sBuffer[0]==NULL || sBuffer[1]==NULL)
	{
		Abort( "\pMemory Allocation Failed" );
	}

//	MessageLong( sBufferSize );	
//	printf( "MEMORY = %ld\n", sBufferSize );
}

//	-------------------------------------------------------------------

void BufferDispos( void )
{
	if (sBuffer[0]) MyDisposPtr( sBuffer[0] );
	if (sBuffer[1]) MyDisposPtr( sBuffer[1] );
	sBuffer[0] = sBuffer[1] = NULL;
	sBufferSize = 0;
}

//	-------------------------------------------------------------------

Size BufferGetSize( void )
{
	return sBufferSize;
}

//	-------------------------------------------------------------------

Ptr BufferGet( int index )
{
	assert( index==0 || index==1, "BufferGet" );
	return sBuffer[index];
}
