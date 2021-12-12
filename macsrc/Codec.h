#ifndef RENDER_INCLUDED__
#define RENDER_INCLUDED__

//	-------------------------------------------------------------------
//	RENDERING FUNCTIONS
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	The list of codecs. See source code for info on data layout
//	-------------------------------------------------------------------

typedef enum
{
	kNull = 0x00,
	kZ16,
	kZ32,
	kInvert,
	kCopy,

	kCodecCount
}	eCodec;

typedef void (*DisplayProc)( char *dest, char *source, int rowBytes );	//	#### int => size_t

void CodecInit( void );
DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type );

#endif
