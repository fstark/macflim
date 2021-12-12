#ifndef RENDER_INCLUDED__
#define RENDER_INCLUDED__

typedef void (*DisplayProc)( char *dest, char *source, int rowBytes );	//	#### int => size_t

void CodecBuildRegistry( void );
DisplayProc CodecGetProc( int codec, int inputWidth, int outputWidth, int type );

#endif
