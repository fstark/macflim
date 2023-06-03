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
//	Passed to the codec for display
//	-------------------------------------------------------------------

struct CodecControlBlock
{
	unsigned short source_width;	//	Width of source in pixels
	unsigned short source_width8;	//	Width of source in bytes
	unsigned short source_width32;	//	Width of source in longs
	unsigned short source_height;	//	Height of source
	
	unsigned char **offsets32;		//	Image offsets as offsets to screen (widthxheight/32) entries
									//	Screen is in offsets[0]

	unsigned short output_width8;	//	Width of the output in bytes
	
		//	#### Warning: code smell: does not work on Lisa
	unsigned short output_width32;	//	Width of the output in long
	
	unsigned char *baseAddr;		//	Address of the top-left of the screen
};

//	-------------------------------------------------------------------
//	Return function to display specific codec's data
//	codec is the codec
//	if same if TRUE, codec will directly copy data and won't use the offsets from the ccb
//	if ref is TRUE, the implementation will be the reference C implementation
//	-------------------------------------------------------------------

typedef void (*DisplayProc)( char *source, struct CodecControlBlock *ccb );
DisplayProc CodecGetProc( eCodec codec, Boolean same, Boolean ref );

//	-------------------------------------------------------------------

void CodecInit( void );

//	-------------------------------------------------------------------

//	Usure if right place
Boolean CreateOffsetTable(
	unsigned char ***offsets,
	unsigned char *base_addr,
	unsigned short input_width,
	unsigned short input_height,
	unsigned short output_width
	);

#endif
