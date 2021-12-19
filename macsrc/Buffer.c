#include "Buffer.h"
#include "Log.h"
#include "Util.h"

#include <stdio.h>

static Ptr sBuffer[2];
Size sBufferSize;

void BufferInit( Size maxSize, Size leftBytes )
{
	Size growBytes;
	Str255 str;
	Size mem = MaxMem(&growBytes);
	
	mem += growBytes;
	
	//NumToString( mem, str );
	//DebugStr( str );
	
//printf( "BufferInit\n" );

//printf( "FreeMem = %ld\n", FreeMem() );
//printf( "Mem = %ld\n", mem );
//printf( "MaxMem = %ld\n", MaxMem(&growBytes) );
//printf( "Growbytes = %ld\n", growBytes );
//printf( "leftBytes = %ld\n", leftBytes );

//printf( "ALLOC=%p\n", NewPtr( 4000L ) );

	//	Look for biggest size
	sBufferSize = (mem-leftBytes)/2;

//printf( "sBufferSize = %ld\n", sBufferSize );

//	return;

	if (sBufferSize>maxSize)
		sBufferSize = maxSize;

sBufferSize/=2;

if (sBufferSize==0)
	sBufferSize = mem/2-100;

	sBuffer[0] = NewPtr( sBufferSize );
	sBuffer[1] = NewPtr( sBufferSize );

	if (sBuffer[0]==NULL || sBuffer[1]==NULL)
		Abort( "\pMemory Allocation Failed" );

//ExitToShell();
}

Size BufferGetSize( void )
{
	return sBufferSize;
}

//	-------------------------------------------------------------------
//	Returns buffer (valid indexes are 0 and 1)
//	-------------------------------------------------------------------

Ptr BufferGet( int index )
{
	assert( index==0 || index==1, "BufferGet" );
	return sBuffer[index];
}

//	-------------------------------------------------------------------
//	Free the buffers
//	-------------------------------------------------------------------

void BufferDispos( void )
{
	if (sBuffer[0]) DisposPtr( sBuffer[0] );
	if (sBuffer[1]) DisposPtr( sBuffer[1] );
	sBuffer[0] = sBuffer[1] = NULL;
	sBufferSize = 0;
}
