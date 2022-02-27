#ifndef RENDER_INCLUDED__
#define RENDER_INCLUDED__

//	-------------------------------------------------------------------
//	CODEC RENDERING FUNCTIONS
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
//	Initialize registry of codecs
//	-------------------------------------------------------------------

void CodecInit( void );

//	-------------------------------------------------------------------
//	Return function to display specific codec's data
//	-------------------------------------------------------------------

typedef void (*DisplayProc)( char *dest, char *source, int rowBytes, short input_width );
DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type );

//	-------------------------------------------------------------------
//	-------------------------------------------------------------------

//	Usure if right place
Boolean CreateOffsetTable(
	long **offsets,
	char *base_addr,
	unsigned short input_width,
	unsigned short input_height,
	unsigned short output_width
	);

#endif
