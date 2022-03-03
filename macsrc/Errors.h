#ifndef ERRORS_INCLUDED__
#define ERRORS_INCLUDED__

void ErrorCannotOpenFlimFile(
	OSErr err,
	const char *fileName,
	long vRefNum,
	long dirID
	);

void ErrorScreenTooSmall(
	const char *name,
	unsigned short flimWidth,
	unsigned short flimHeight,
	unsigned short screenWidth,
	unsigned short screenHeight
	);

#endif
