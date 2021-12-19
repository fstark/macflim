#ifndef RENDER_INCLUDED__
#define RENDER_INCLUDED__

//	-------------------------------------------------------------------
//	RENDERING FUNCTIONS
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	List of codecs.
//	See source code for info on data layout of encodings
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

//	-------------------------------------------------------------------
//	Initialize list of codecs
//	-------------------------------------------------------------------

void CodecInit( void );

//	-------------------------------------------------------------------
//	Return function to display specific codec's data
//	-------------------------------------------------------------------

typedef void (*DisplayProc)( char *dest, char *source, int rowBytes );
DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type );

#endif
