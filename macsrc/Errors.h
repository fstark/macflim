#ifndef ERRORS_INCLUDED__
#define ERRORS_INCLUDED__

//	0 => ok, 1 => locate, 2=> remove
int ErrorCannotOpenFlimFile(
	OSErr err,
	const char *fileName,	//	#### Str255
	short vRefNum,
	long dirID
	);

void ErrorScreenTooSmall(
	const char *name,
	unsigned short flimWidth,
	unsigned short flimHeight,
	unsigned short screenWidth,
	unsigned short screenHeight
	);

void ErrorDebugFile(
	OSErr err,
	Str255 fileName,
	long vRefNum,
	long dirID
	);

void InfoIntegritySuccess( void );

void InfoAutoPlaySuccess( void );

#endif
