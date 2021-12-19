#ifndef CHECKSUM_INCLUDED__
#define CHECKSUM_INCLUDED__

//	-------------------------------------------------------------------
//	FLIM CHECKSUM
//	-------------------------------------------------------------------

//	-------------------------------------------------------------------
//	If file contains a checksum, brings the dialog asking the user
//	if he wants to check, and performs the check.
//	return FALSE if checksum test fails, TRUE otherwise
//	-------------------------------------------------------------------
Boolean ChecksumFlimIfNeeded( Str255 fName, short vRefNum );

#endif
