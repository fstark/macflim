#ifndef LIBRARY_INCLUDED__
#define LIBRARY_INCLUDED__

//	-------------------------------------------------------------------
//	Library handling routines
//	-------------------------------------------------------------------

struct LibraryRecord;
typedef struct LibraryRecord *LibraryPtr;

#include "Flim.h"

LibraryPtr LibraryOpenDefault( void );
void LibraryDispos( LibraryPtr lib );

WindowPtr LibraryGetWindow( LibraryPtr lib );
void LibraryDrawFlimIfNeeded( LibraryPtr lib, int index );
void LibraryDrawContentIfNeeded( LibraryPtr lib );
void LibraryDrawWindow( LibraryPtr lib );

Boolean LibraryAddFlim( LibraryPtr *lib, Str255 fName, short vRefNum, long dirID );
void LibraryRemoveSelectedFlims( LibraryPtr lib );
Boolean LibraryAutostartSelectedFlims( LibraryPtr lib );

int LibraryGetFlimUnder( LibraryPtr lib, Point pt );
void LibraryDrawFlim( LibraryPtr lib, int index );
void LibraryFlipSelection( LibraryPtr lib, int index );
int LibraryGetFirstSelected( LibraryPtr lib );
int LibraryGetLastSelected( LibraryPtr lib );
Boolean LibraryGetSelection( LibraryPtr lib, int index);
Boolean LibraryIsSelectionEmpty( LibraryPtr lib );
void LibrarySetSelection( LibraryPtr lib, int index, Boolean selected );
void LibrarySelectAll( LibraryPtr lib );
void LibraryUnselectAll( LibraryPtr lib );
void LibraryUnselectAllOthers( LibraryPtr lib, int index );
int LibraryGetSelectionCount( LibraryPtr lib );
void LibraryInval( LibraryPtr lib );

FlimPtr LibraryOpenFlim( LibraryPtr lib, int index );	//	Needs to call FlimDispos after

short LibaryGetMinWidth( LibraryPtr lib );
short LibaryGetMinHeight( LibraryPtr lib );
short LibaryGetMaxWidth( LibraryPtr lib );
short LibaryGetMaxHeight( LibraryPtr lib );
short LibaryGetVisibleHeight( LibraryPtr lib );
short LibraryGetBestWidth( LibraryPtr lib, short width );

void LibraryWindowResized( LibraryPtr lib );

short LibraryScroll( LibraryPtr lib, short delta );
short LibraryScrollTo( LibraryPtr lib, short offset );
void LibraryScrollToIndex( LibraryPtr lib, int index );

int LibraryGetColumns( LibraryPtr lib );
int LibraryGetCount( LibraryPtr lib );

void LibraryGetFlimRect( LibraryPtr lib, int index, Rect *r );
void LibraryGetFlimRectGlobal( LibraryPtr lib, int index, Rect *r );

void LibraryXXX( LibraryPtr lib );

void LibraryGet( LibraryPtr lib, int index, Str255 fName, short *vRefNum, long *dirID );

#endif
