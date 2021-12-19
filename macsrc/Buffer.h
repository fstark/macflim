#ifndef BUFFER_INCLUDED__
#define BUFFER_INCLUDED__

//	-------------------------------------------------------------------
//	BUFFER MANAGEMENT
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	Defines the optimal size of buffers and allocate them
//	Makes sure that there are leftBytes contiguous free bytes of memory
//	after allocation. MaxSize is the maximum size of the buffers.
//	-------------------------------------------------------------------

void BufferInit( Size maxSize, Size leftBytes );

//	-------------------------------------------------------------------
//	Return buffer size
//	-------------------------------------------------------------------

Size BufferGetSize( void );

//	-------------------------------------------------------------------
//	Returns buffer (valid indexes are 0 and 1)
//	-------------------------------------------------------------------

Ptr BufferGet( int index );

//	-------------------------------------------------------------------
//	Free the buffers
//	-------------------------------------------------------------------

void BufferDispos( void );

#endif
