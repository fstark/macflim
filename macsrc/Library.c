//	#### TODO: cleanup to use GetLEF everywhere
//	#### TODO: make sure index is 2 parameter (and not 3rd)

#include "Library.h"
#include "Util.h"
#include "Resources.h"
#include "Flim.h"
#include "buffer.h"
#include "Errors.h"
#include "Self Player.h"

#include <stdio.h>
#include <string.h>

#define LIBRARY_COLUMNS	3

struct LibraryEntryFlim
{
//	The data saved on disk
	char name[32];		//	P01F
	short vRefNum;		//	DWRD
	short dirID;		//	DWRD
	short width;		//	DWRD
	short height;		//	DWRD
	Boolean dummy0:1;	//	BBIT
	Boolean dummy1:1;	//	BBIT
	Boolean dummy2:1;	//	BBIT
	Boolean dummy3:1;	//	BBIT
	Boolean dummy4:1;	//	BBIT
	Boolean dummy5:1;	//	BBIT
	Boolean dummy6:1;	//	BBIT
//	Boolean selfPlay:1;	//	BBIT	-- Too complicated as it needs to check metadata
	Boolean silent:1;	//	BBIT
	char dummy;			//	HBYT
	long frameCount;	//	DLNG
	long ticks;			//	DLNG
	short byterate;		//	DWRD
	char fName[1];		//	PSTR
};

typedef enum
{
	kQuality128,
	kQualityMacXL,
	kQuality512,
	kQualityPlus,
	kQualitySE,
	kQualitySE30,
	kQualityPortable,
	kQualityUnknown
}	eQuality;

static short LibraryEnryFlimGetQualityIndex( struct LibraryEntryFlim *e )
{
	Boolean fitsCompact = e->width<=512 && e->height<=342;
	Boolean fitsPortable = e->width<=640 && e->height<=400;
	Boolean fitsXL	 = e->width<=704 && e->height<=364;

	if (e->silent)
	{
			//	Can't be 128, 512 or XL if not silent
		if (fitsCompact && e->byterate<=500)
			return kQuality128;	//	#### NEEDS TO CHECK FILE SIZE TOO <400Kb
		if (fitsXL && e->byterate<=600)
			return kQualityMacXL;
		if (fitsCompact && e->byterate<=480+400)
			return kQuality512;
	}

		//	Plus
	if (fitsCompact && e->byterate<=1500+400)
		return kQualityPlus;

		//	Portable and SE differs in size
	if (e->byterate<=2500+400)
		if (fitsCompact)
			return kQualitySE;
		else
			if (fitsPortable)
				return kQualityPortable;

		//	Se30
	if (fitsCompact && e->byterate<=6000+400)
		return kQualitySE30;

		//	Rest, we don't know
	return kQualityUnknown;
}

struct LibraryEntry
{
	Handle flim;			//	To a LibraryEntryFlim
//	Str255 name;
//	Str255 fName;			//	vRefNum? dirId?
//	short vRefNum;
	short resID;			//	Resource ID
	short NOPEindex;			//	sequential ID
	Handle pict;			//	Image if loaded
	Boolean selected;		//	Selected
	Boolean needsUpdate;	//	If need update
};

struct LibraryRecord
{
	short count;
	short resFile;

		//	Content
	short columns;
	short lines;

	short contentWidth;
	short contentHeight;

	short visibleHeight;

	short offsetHeight;		//	How much did we scroll?

	WindowPtr window;
	ControlHandle scroll;	//	Vertical scroll bar
	
	Boolean scrollVisible;	//	lib->contentHeight>=lib->visibleHeight
	
	struct LibraryEntry entries[1];
};

#define PICT_WIDTH		128
#define PICT_HEIGHT		86
#define POLAROID_BORDER	10
#define POLAROID_TEXT	16
#define POLAROID_BASELINE 12
#define INTER_X			8
#define INTER_Y			8
#define MARGIN_X		15
#define MARGIN_Y		8		//	Suspects it should be removed for better key scroll behavior in first and last line

//	outer border, white border, inner border, pict, inner border, white border, outer border
#define POLAROID_WIDTH (2+POLAROID_BORDER+PICT_WIDTH+POLAROID_BORDER+2)
#define POLAROID_HEIGHT (2+POLAROID_BORDER+PICT_HEIGHT+POLAROID_TEXT+2)

//	Offscreen Grafport to draw the flims Polaroid without flickering
static GrafPort sOffscreen;

static void LibraryOffscreenInit()
{
	Rect r;
	
	SetRect( &r, 0, 0, POLAROID_WIDTH, POLAROID_HEIGHT );
	OpenPort( &sOffscreen );
	sOffscreen.portRect = r;
	sOffscreen.portBits.bounds = r;
	RectRgn( sOffscreen.clipRgn, &r );
	RectRgn( sOffscreen.visRgn, &r );
	sOffscreen.portBits.rowBytes = (POLAROID_WIDTH+15)/16*2;
	sOffscreen.portBits.baseAddr = MyNewPtr( sOffscreen.portBits.rowBytes*(long)POLAROID_HEIGHT );
	assert( sOffscreen.portBits.baseAddr!=NULL, "LibraryOffscreenInit" );
	EraseRect( &r );
}

//	#### Never called
static void LibraryOffscreenDispos()
{
	ClosePort( &sOffscreen );
	MyDisposPtr( sOffscreen.portBits.baseAddr );
}

static Size LibraryGetMemorySize( short count )
{
	return sizeof( struct LibraryRecord )+(count-1)*sizeof( struct LibraryEntry );
}

static void UtilRealloc( Ptr *p, Size s )
{
	MySetPtrSize( *p, s );
	if (MemError())
	{
		Ptr n = MyNewPtr( s );
		if (!n)
			Abort( "\pOut of memory" );
		BlockMove( *p, n, MyGetPtrSize( *p ) );
		*p = n;
	}
}

static void AddEntry( LibraryPtr *l )
{
	(*l)->count++;
	UtilRealloc( (Ptr *)l, LibraryGetMemorySize( (*l)->count ) );
}

#define LIBRARY_FILE "\pMacFlim Library"

static short LibraryGetWidth( int columns )
{
	//	Last +1 for the polaroid shadow
	return MARGIN_X+columns*POLAROID_WIDTH+(columns-1)*INTER_X+MARGIN_X+1;
}

static short LibraryGetHeight( int lines )
{
	return MARGIN_Y+lines*POLAROID_HEIGHT+(lines-1)*INTER_Y+MARGIN_Y+1;
}

static short LibraryGetLines( int count, int columns )
{
	return (count+columns-1)/columns;
}

static void LibraryAdjustScroll( LibraryPtr lib )
{
	if (lib->scroll && lib->scrollVisible)
		SetCtlValue( lib->scroll, lib->offsetHeight );
}

static void LibraryUpdateScrollVisible( LibraryPtr lib )
{
	WindowPtr w = lib->window;

	lib->scrollVisible = lib->visibleHeight<lib->contentHeight;

	if (!w || !lib->scroll)
		return ;

	if (lib->scrollVisible)
	{
		SetCtlMin( lib->scroll, 0 );
		SetCtlMax( lib->scroll, lib->contentHeight-lib->visibleHeight );

		MoveControl( lib->scroll, w->portRect.right-15, w->portRect.top-1 );
		SizeControl( lib->scroll, 16, w->portRect.bottom-w->portRect.top-13 );
		if (!(*lib->scroll)->contrlVis)
			ShowControl( lib->scroll );
	}
	else
	{
		if ((*lib->scroll)->contrlVis)
			HideControl( lib->scroll );
	}
}

static void LibaryUpdateContentSizes( LibraryPtr lib )
{
		//	Number of lines than can be displayed
	lib->lines = LibraryGetLines( lib->count, lib->columns );

		//	Size of content
	lib->contentWidth = LibraryGetWidth( lib->columns );
	lib->contentHeight = LibraryGetHeight( lib->lines );


	if (lib->offsetHeight>0)
	{
		if (lib->offsetHeight+lib->visibleHeight>lib->contentHeight)
		{
			lib->offsetHeight = lib->contentHeight-lib->visibleHeight;
			if (lib->offsetHeight<0)
				lib->offsetHeight = 0;
		}
	}

		//	xxx
	if (lib->contentHeight<lib->visibleHeight)
		lib->contentHeight = lib->visibleHeight;
}

static void LibrarySetColumnCount( LibraryPtr lib, short columns )
{
	lib->columns = columns;
	LibaryUpdateContentSizes( lib );
	LibraryUpdateScrollVisible( lib );
}

void LibraryGetFlimRect( LibraryPtr lib, int index, Rect *r )
{
	int x;
	int y;
	short deltaX;
	short deltaY;

	y = index/lib->columns;
	x = index%lib->columns;

//	deltaX = MARGIN_X+x*(PICT_WIDTH+2*POLAROID_BORDER)+(x-1)*INTER_X;
	deltaX = MARGIN_X+x*(POLAROID_WIDTH+INTER_X);

//	deltaY = MARGIN_Y+y*(PICT_HEIGHT+POLAROID_BORDER+POLAROID_TEXT)+(y-1)*INTER_Y;
	deltaY = MARGIN_Y+y*(POLAROID_HEIGHT+INTER_Y);

		//	OUTER FRAME
	r->left = deltaX;
	r->right = deltaX+POLAROID_WIDTH;
	r->top = deltaY;
	r->bottom = deltaY+POLAROID_HEIGHT;
}

void LibraryGetFlimRectGlobal( LibraryPtr lib, int index, Rect *r )
{
	LibraryGetFlimRect( lib, index, r );
	OffsetRect( r, 0, -lib->offsetHeight ); 
	LocalToGlobal( &topLeft(*r) );
	LocalToGlobal( &botRight(*r) );
}

static void GetScrollBarRect( LibraryPtr lib, Rect *r )
{
	short x = lib->window->portRect.right-15;
	short h = (lib->window->portRect.bottom-lib->window->portRect.top)-15;

	SetRect( r,
		lib->window->portRect.right-15,
		lib->window->portRect.top-1,
		lib->window->portRect.right+1,
		lib->window->portRect.bottom-14 );
}

LibraryPtr LibraryOpenDefault( void )
{
	LibraryPtr lib;
	short count;
	int i;
	short resFile;

		//	#### Needs a LibraryInit(), probably
	LibraryOffscreenInit();

		//	Load or create library
	resFile = OpenResFile( LIBRARY_FILE );

	if (resFile==-1)
	{
		Create( LIBRARY_FILE, 0, 'FLPL', 'FLIB' );
		CreateResFile( LIBRARY_FILE );
		resFile = OpenResFile( LIBRARY_FILE );
		if (resFile==-1)
			Abort( "\pCannot create library" );
	}

		//	Size in-memory library
	count = Count1Resources('FLIM');

	lib = (LibraryPtr)MyNewPtr( LibraryGetMemorySize( count ) );
	lib->count = count;
	lib->resFile = resFile;

	lib->window = NULL;
	lib->scroll = NULL;

	for (i=0;i!=count;i++)
	{
		short resID;
		OSType dummy0;
		Str255 dummy1;
		Handle h = Get1IndResource( 'FLIM', i+1 );

		GetResInfo( h, &resID, &dummy0, dummy1 );
//		DisposHandle( h );			//	Could keep it in .flim...
		lib->entries[i].resID = resID;
//		lib->entries[i].index = i;
//		lib->entries[i].flim = NULL;
		lib->entries[i].flim = h;
		lib->entries[i].pict = NULL;
		lib->entries[i].selected = FALSE;
		lib->entries[i].needsUpdate = FALSE;
	}

	lib->visibleHeight = 295;
	lib->offsetHeight = 0;
	LibrarySetColumnCount( lib, LIBRARY_COLUMNS );

	lib->window = GetNewWindow( kWINDLibraryID, NULL, (WindowPtr)-1 );
	
	SizeWindow( lib->window, lib->contentWidth, lib->visibleHeight, FALSE );
	UtilPlaceWindow( lib->window, 0.5 );

	{
		Rect r;
		
		ShowWindow( lib->window );
		GetScrollBarRect( lib, &r );
		lib->scroll = NewControl( lib->window, &r, "\p", FALSE, 0, 0, 10, scrollBarProc, 0 );
		LibraryUpdateScrollVisible( lib );
	}


	return lib;
}

//	#### Not called yet
void LibraryDispos( LibraryPtr lib )
{
	DisposeControl( lib->scroll );
	DisposeWindow( lib->window );
	MyDisposPtr( lib );
}

static void LibraryLoadEntry( LibraryPtr lib, short index )
{
	struct LibraryEntry *e = lib->entries+index;

	if (e->flim==NULL)
	{
		UseResFile( lib->resFile );
		e->flim = GetResource( 'FLIM', e->resID );
		assert( e->flim!=NULL, "no such flim" );
	}

	HLock( e->flim );
	
	if (e->pict==NULL)
	{
		e->pict = GetResource( 'PICT', e->resID );
		assert( e->pict!=NULL, "no picture" );
	}
}

static void LibraryUnloadEntry( LibraryPtr lib, short index )
{
	struct LibraryEntry *e = lib->entries+index;

	if (e->flim!=NULL)
		HUnlock( e->flim );
}

static void LibraryGetName( LibraryPtr lib, Str255 name, short index )
{
	struct LibraryEntryFlim *lef;

	LibraryLoadEntry( lib, index );
	
	lef = (struct LibraryEntryFlim *)*(lib->entries[index].flim);
	
	StrCpyPP( name, (void*)lef->name );
	
	LibraryUnloadEntry( lib, index );
}

static void LibraryGetFlimName( LibraryPtr lib, Str255 name, short index )
{
	struct LibraryEntryFlim *lef;

	LibraryLoadEntry( lib, index );
	
	lef = (struct LibraryEntryFlim *)*(lib->entries[index].flim);

	StrCpyPP( name, (void*)lef->fName );

	LibraryUnloadEntry( lib, index );
}

static Handle LibraryGetPICT( LibraryPtr lib, short index )
{
	LibraryLoadEntry( lib, index );
	LibraryUnloadEntry( lib, index );
	return lib->entries[index].pict;
}

WindowPtr LibraryGetWindow( LibraryPtr lib )
{
	return lib->window;
}

int LibraryGetFlimUnder( LibraryPtr lib, Point pt )
{
	int i;
	Rect r;
	
	pt.v += lib->offsetHeight;
	
	for (i=0;i!=lib->count;i++)
	{
		LibraryGetFlimRect( lib, i, &r );

		if (PtInRect( pt, &r ))
			return i;
	}
	return -1;
}

static void LibraryGetRect( LibraryPtr lib, int index, Rect *rect )
{
	int x = index%lib->columns;
	int y = index/lib->columns;

	int deltaX = MARGIN_X+x*(POLAROID_WIDTH+INTER_X);
	int deltaY = MARGIN_Y+y*(POLAROID_HEIGHT+INTER_Y);

	if (lib->scrollVisible)
	{
		deltaX -=7;
	}

	rect->top = deltaY;
	rect->bottom = deltaY+POLAROID_HEIGHT;
	rect->left = deltaX;
	rect->right = deltaX+POLAROID_WIDTH;
}

static void LibraryPolaroidRender( LibraryPtr lib, struct LibraryEntry *entry )
{
	struct LibraryEntryFlim **lef;
	GrafPtr savePort;
	
	Rect r;
	SetRect( &r, 0, 0, POLAROID_WIDTH, POLAROID_HEIGHT );

	GetPort( &savePort );
	SetPort( &sOffscreen );

		//	OUTER FRAME
	PenSize( 1, 1 );
	PenPat( black );
	FrameRect( &r );

		//	BACKGROUND
	InsetRect( &r, 1, 1 );
	if (entry->selected)
		FillRect( &r, dkGray );
//		FillRect( &r, white );
	else
		FillRect( &r, white );

		//	PICTURE
	r.top = 1+POLAROID_BORDER+1;
	r.bottom = r.top+PICT_HEIGHT;
	r.left = 1+POLAROID_BORDER+1;
	r.right = r.left+PICT_WIDTH;
	DrawPicture( LibraryGetPICT( lib, entry-lib->entries ), &r );

		//	PICTURE BORDER
	InsetRect( &r, -1, -1 );
	PenPat( black );
	PenSize( 1, 1 );
	FrameRect( &r );

		//	NAME
	{
		Str255 name;
		short w;
		
		LibraryGetName( lib, name, entry-lib->entries );
		TextFont( applFont );
		TextSize( 9 );
		w = StringWidth( name );

		MoveTo( r.left+PICT_WIDTH/2-w/2, r.bottom+POLAROID_BASELINE );

		TextMode( srcCopy );

		DrawString( name );
	}
	
	if (entry->selected)
	{
		char buffer[128];
		short width;
		short height;
		long seconds;
		short byterate;

		PicHandle badge;
		Rect badgeRect;

		LibraryLoadEntry( lib, entry-lib->entries );			//	Needs to make sure the FLIM resource is loaded
		lef = (struct LibraryEntryFlim **)(entry->flim);

		//	Tune and resolution
		width = (*lef)->width;
		height = (*lef)->height;
		seconds = (*lef)->ticks/60;
		byterate = (*lef)->byterate;

		sprintf( buffer+1, "%02ld:%02ld %dx%d %dBpt", seconds/60, seconds%60, width, height, byterate );
		buffer[0] = strlen( buffer+1 );
		MoveTo( POLAROID_BORDER+2, POLAROID_BORDER+9 );
		TextFont( courier );
		TextSize( 9 );
		TextMode( notSrcCopy );
		DrawString( buffer );
		
		//	Sound badge
		badge = GetPicture( kPICTPolaroidSoundBadgeID+(*lef)->silent );
		assert( badge!=NULL, "GetPicture sound badge" );
		badgeRect = (*badge)->picFrame;
		OffsetRect( &badgeRect, -badgeRect.left, -badgeRect.top );
		OffsetRect( &badgeRect, POLAROID_BORDER+PICT_WIDTH-badgeRect.right+1+2, POLAROID_BORDER+1 );
		DrawPicture( badge, &badgeRect );

		//	Quality Badge
		badge = GetPicture( kPICTPolaroidBadgesID+LibraryEnryFlimGetQualityIndex( *lef ) );
		assert( badge!=NULL, "GetPicture badge" );
		badgeRect = (*badge)->picFrame;
		OffsetRect( &badgeRect, -badgeRect.left, -badgeRect.top );
		OffsetRect( &badgeRect, POLAROID_BORDER-1+2, POLAROID_BORDER+PICT_HEIGHT-badgeRect.bottom+3 );
		DrawPicture( badge, &badgeRect );

/*	Removed due to implementation complexity
		//	Self play
		if ((*lef)->selfPlay)
		{
			badge = GetPicture( kPICTPolaroidSelfPlayBadge );
			assert( badge!=NULL, "GetPicture selfplay badge" );
			badgeRect = (*badge)->picFrame;
			OffsetRect( &badgeRect, -badgeRect.left, -badgeRect.top );
			OffsetRect( &badgeRect, POLAROID_BORDER+PICT_WIDTH-badgeRect.right+1+2, POLAROID_BORDER+PICT_HEIGHT-badgeRect.bottom+3 );
			DrawPicture( badge, &badgeRect );
		}
*/
		LibraryUnloadEntry( lib, entry-lib->entries );
	}
	
	SetPort( savePort );
}

static Boolean Overlaps( Rect *r0, Rect *r1 )
{
	if (r0->right<r1->left)
		return FALSE;

	if (r1->right<r0->left)
		return FALSE;

	if (r0->bottom<r1->top)
		return FALSE;

	if (r1->bottom<r0->top)
		return FALSE;

	return TRUE;
}

void LibraryDrawFlim( LibraryPtr lib, int index )
{
	Rect r;
	WindowPtr w = lib->window;
	Rect r1 = (*w->visRgn)->rgnBBox;
	Rect r2 = (*w->clipRgn)->rgnBBox;
	Rect r0;

	SectRect( &r1, &r2, &r0 );

	LibraryGetRect( lib, index, &r );	

//	if (!RectInRgn(&r,*w->visRgn))	//	Doesn't work, no idea why
	if (!Overlaps( &r, &r0 ))
		return ;

	if (index==14)
		BreakHere();

//printf( "RENDER %d\n", index );

	LibraryPolaroidRender( lib, lib->entries+index );

	CopyBits(
		&sOffscreen.portBits,
		&(*w).portBits,
		&sOffscreen.portRect,
		&r,
		srcCopy,
		NULL );
//	SetPort( w );
	MoveTo( r.left, r.bottom );
	LineTo( r.right, r.bottom );
	LineTo( r.right, r.top+1 );
}

void LibraryDrawFlimIfNeeded( LibraryPtr lib, int index )
{
	if (lib->entries[index].needsUpdate)
	{
		lib->entries[index].needsUpdate = FALSE;
		LibraryDrawFlim( lib, index );
	}
}

static void MyDrawGrowIcon( WindowPtr aWindow, Boolean withBg )
{
	Point pt;
	Rect r;
	
	SetPt( &pt, aWindow->portRect.right-14, aWindow->portRect.bottom-14 );

	//	white background
	if (withBg)
	{
		PenPat( black );
		MoveTo( pt.h-1, pt.v );
		LineTo( pt.h-1, pt.v+13 );
		SetRect( &r, pt.h, pt.v, pt.h+14, pt.v+14 );
		FillRect( &r, white );
	}

	PenPat( white );
	SetRect( &r, pt.h+2, pt.v+2, pt.h+2+7, pt.v+2+7 );
	InsetRect( &r, 1, 1 );
	FrameRect( &r );
	InsetRect( &r, -2, -2 );
	FrameRect( &r );
	SetRect( &r, pt.h+4, pt.v+4, pt.h+4+9, pt.v+4+9 );
	InsetRect( &r, 1, 1 );
	FrameRect( &r );
	InsetRect( &r, -2, -2 );
	FrameRect( &r );

	PenPat( black );
	SetRect( &r, pt.h+2, pt.v+2, pt.h+2+7, pt.v+2+7 );
	FrameRect( &r );
	SetRect( &r, pt.h+4, pt.v+4, pt.h+4+9, pt.v+4+9 );
	FrameRect( &r );
}

//	Draws all the *content* of the window (ie: the polaroids)
static void LibraryDrawWindowContent( LibraryPtr lib )
{
	Rect r;
	int i;

//	printf( "LibraryDrawWindowContent:\n" );

	SetOrigin( 0, lib->offsetHeight );

	r.top = 0;
	r.left = 0;
	r.right = lib->contentWidth;
	r.bottom = lib->contentHeight;
	if (lib->scrollVisible)
		r.right -= 15;
	FillRect( &r, gray );

	for (i=0;i!=lib->count;i++)
		LibraryDrawFlim( lib, i );

	SetOrigin( 0, 0 );

//	printf( "---\n" );
}

void LibraryDrawContentIfNeeded( LibraryPtr lib )
{
	GrafPtr savePort;
	int i;

//	printf( "LibraryDrawContentIfNeeded:\n" );

	GetPort( &savePort );
	SetPort( lib->window );

	SetOrigin( 0, lib->offsetHeight );
	for (i=0;i!=lib->count;i++)
		LibraryDrawFlimIfNeeded( lib, i );
	SetOrigin( 0, 0 );

	SetPort( savePort );

//	printf( "---\n" );
}

void LibraryDrawWindow( LibraryPtr lib )
{
	Rect r;
	int i;

//	printf( "LibraryDrawWindow:\n" );

	SetOrigin( 0, lib->offsetHeight );

	r.top = 0;
	r.left = 0;
	r.right = lib->contentWidth;
	r.bottom = lib->contentHeight;
	if (lib->scrollVisible)
		r.right -= 15;
	FillRect( &r, gray );

	SetOrigin( 0, 0 );

	MyDrawGrowIcon( lib->window, lib->scrollVisible );

	SetOrigin( 0, lib->offsetHeight );

	for (i=0;i!=lib->count;i++)
		LibraryDrawFlim( lib, i );

	SetOrigin( 0, 0 );

	DrawControls( lib->window );

//	printf( "---\n" );
}

Boolean LibraryAddFlim( LibraryPtr *lib, Str255 fName, short vRefNum, long dirID )
{
	struct LibraryEntry *e;
	struct LibraryEntryFlim *lef;
	FlimPtr flim;
	
	flim = FlimOpenByName( fName, vRefNum, dirID, kHFS );
	if (!flim)
		return FALSE;

	AddEntry( lib );
	e = (*lib)->entries+(*lib)->count-1;

	e->flim = NewHandle( 1024 );
	HLock( e->flim );
	lef = (struct LibraryEntryFlim *)*(e->flim);

	lef->vRefNum = vRefNum;
	lef->dirID = dirID;
	{
		struct FlimInfo *info = FlimGetInfo( flim );
		lef->width = info->width;
		lef->height = info->height;
		lef->silent = info->silent;
		lef->frameCount = info->frameCount;
		lef->ticks = info->ticks;
		lef->byterate = info->byterate;
	}

	StrCpyPP( (void *)(lef->name), fName );
	StrCpyPP( (void *)(lef->fName), fName );
	SetHandleSize( e->flim, sizeof(struct LibraryEntryFlim)+fName[0]-1 );
	e->pict = (Handle)FlimCreatePoster( flim );
	e->selected = FALSE;
	e->needsUpdate = TRUE;
//	e->index = (*lib)->count-1;

	UseResFile( (*lib)->resFile );
	e->resID = Unique1ID( 'FLIM' );
	AddResource( e->flim, 'FLIM', e->resID , "\p" );
	AddResource( e->pict, 'PICT', e->resID, "\p" );
	HUnlock( e->flim );

	WriteResource( e->flim  );
	WriteResource( e->pict  );

	FlimDispos( flim );

	return TRUE;
}

static Boolean LibraryRemoveFlim( LibraryPtr lib, int index )
{
	struct LibraryEntry *entry;
	int i;

	LibraryLoadEntry( lib, index );
	LibraryUnloadEntry( lib, index );

	entry = lib->entries+index;

	RmveResource( entry->flim );
	RmveResource( entry->pict );
	DisposHandle( entry->flim );
	DisposHandle( entry->pict );

	for (i=index;i!=lib->count-1;i++)
		lib->entries[i] = lib->entries[i+1];

	lib->count--;
	LibraryUnloadEntry( lib, index );

	return TRUE;
}

void LibraryRemoveSelectedFlims( LibraryPtr lib )
{
	int i=lib->count;
	
	while (i--)
		if (LibraryGetSelection( lib, i ))
			LibraryRemoveFlim( lib, i );
}

static struct LibraryEntryFlim *GetLEF( LibraryPtr lib, int index )
{
	assert( index>=0 && index<lib->count, "Bad Index" );

	LibraryLoadEntry( lib, index );
	return (struct LibraryEntryFlim *)*(lib->entries[index].flim);
}

void LibraryAutostartSelectedFlims( LibraryPtr lib )
{
	int i;

	SelfGetMiniPlayer();

	for (i=0;i!=lib->count;i++)
		if (LibraryGetSelection( lib, i ))
		{
			struct LibraryEntryFlim *lef = GetLEF( lib, i );
			SelfInstallPlayer( (void *)lef->name, lef->vRefNum, lef->dirID );
			LibraryUnloadEntry( lib, i );
		}
}

void LibraryFlipSelection( LibraryPtr lib, int index )
{
	assert( index>=0 && index<lib->count, "Bad Index" );
	lib->entries[index].selected = !lib->entries[index].selected;
}

Boolean LibraryGetSelection( LibraryPtr lib, int index)
{
	assert( index>=0 && index<lib->count, "Bad Index" );
	return lib->entries[index].selected;
}

int LibraryGetFirstSelected( LibraryPtr lib )
{
	int i;
	for (i=0;i!=lib->count;i++)
		if (LibraryGetSelection( lib, i ))
			return i;
	return -1;
}

int LibraryGetLastSelected( LibraryPtr lib )
{
	int i;
	for (i=lib->count-1;i>=0;i--)
		if (LibraryGetSelection( lib, i ))
			return i;
	return -1;
}

Boolean LibraryIsSelectionEmpty( LibraryPtr lib )
{
	return LibraryGetFirstSelected( lib )==-1;
}

int LibraryGetSelectionCount( LibraryPtr lib )
{
	int i;
	int count = 0;

	for (i=0;i!=lib->count;i++)
		if (LibraryGetSelection( lib, i ))
			count++;

	return count;
}

void LibrarySetSelection( LibraryPtr lib, int index, Boolean selected )
{
	assert( index>=0 && index<lib->count, "Bad Index" );
	if (lib->entries[index].selected!=selected)
	{
		lib->entries[index].selected = selected;
		lib->entries[index].needsUpdate = TRUE;
	}
}

void LibrarySelectAll( LibraryPtr lib )
{	int i;
	
	for (i=0;i!=lib->count;i++)
		LibrarySetSelection( lib, i, TRUE );
		
}

void LibraryUnselectAll( LibraryPtr lib )
{	int i;
	
	for (i=0;i!=lib->count;i++)
		LibrarySetSelection( lib, i, FALSE );
		
}

void LibraryUnselectAllOthers( LibraryPtr lib, int index )
{	int i;
	
	for (i=0;i!=lib->count;i++)
		if (i!=index)
			LibrarySetSelection( lib, i, FALSE );
}

void LibraryInval( LibraryPtr lib )
{	int i;
	
	GrafPtr savePort;
	GetPort( &savePort );
	SetPort( lib->window );

	SetOrigin( 0, lib->offsetHeight );

	for (i=0;i!=lib->count;i++)
		if (lib->entries[i].needsUpdate)
		{
			Rect r;
			lib->entries[i].needsUpdate = FALSE;
//			LibraryDrawFlim( lib, i );

			LibraryGetRect( lib, i, &r );

			InvalRect( &r );
		}		

	SetOrigin( 0, 0 );
	SetPort( savePort );
}

FlimPtr LibraryOpenFlim( LibraryPtr lib, int index )
{
	FlimPtr flim;
	struct LibraryEntryFlim *lef = GetLEF( lib, index );
	Str255 name;

//xxx this is wrong, because lef is not HLocked

	//	#### if volumes have been re-ordered, this may be slow and even do floppy access...
	//	We could get the vRefNum back from the flim and update the library
	flim = FlimOpenByNameAnyVolumes( (void*)lef->fName, lef->vRefNum, lef->dirID, kHFS );
	if (!flim)
	{
		ErrorCannotOpenFlimFile( FlimError(), lef->fName, lef->vRefNum, lef->dirID );
		LibraryUnloadEntry( lib, index );
		return NULL;
	}
	
	LibraryUnloadEntry( lib, index );
	return flim;
}

short LibaryGetMinWidth( LibraryPtr lib )
{
	return LibraryGetWidth( 1 );
}

short LibaryGetMinHeight( LibraryPtr lib )
{
	return LibraryGetHeight( 1 );
}

short LibaryGetMaxWidth( LibraryPtr lib )
{
	short w = screenBits.bounds.right-screenBits.bounds.left-4;
	int i = 10;

	while (LibraryGetWidth( i )>w)
		i--;

	return LibraryGetWidth(i);
}

short LibaryGetMaxHeight( LibraryPtr lib )
{

	short h = screenBits.bounds.bottom-screenBits.bounds.top-30;
	return h;
}

short LibaryGetVisibleHeight( LibraryPtr lib )
{
	return lib->visibleHeight;
}

static short LibraryGetMaxColumns( LibraryPtr lib, short pixelWidths )
{
	int i = 10;
	
	while (LibraryGetWidth( i )>pixelWidths)
		i--;

	return i;
}

short LibraryGetBestWidth( LibraryPtr lib, short width )
{
	return LibraryGetWidth(LibraryGetMaxColumns( lib, width+POLAROID_WIDTH/2 ));
}

void LibraryWindowResized( LibraryPtr lib )
{
	WindowPtr w = lib->window;

	lib->visibleHeight = w->portRect.bottom-w->portRect.top;
	LibrarySetColumnCount( lib, LibraryGetMaxColumns( lib, w->portRect.right-w->portRect.left ) );

	LibraryUpdateScrollVisible( lib );
}

short LibraryScrollTo( LibraryPtr lib, short offset )
{
	short currentOffset = lib->offsetHeight;
	short delta;

	lib->offsetHeight = offset;
	if (lib->offsetHeight<0)
		lib->offsetHeight = 0;
	if (lib->offsetHeight+lib->visibleHeight>lib->contentHeight)
		lib->offsetHeight = lib->contentHeight-lib->visibleHeight;

	delta = lib->offsetHeight-currentOffset;

	if (delta)
	{
		Rect r = lib->window->portRect;
		RgnHandle rgn = NewRgn();
		RgnHandle saveRgn = NewRgn();
		GrafPtr savePort;

		GetPort( &savePort );
		SetPort( lib->window );

		r.right -= 15;		//	Don't scroll the scroll bar
	
		LibraryAdjustScroll( lib );



		ScrollRect( &r, 0, -delta, rgn );

//	We don't do InvalRgn, as the update will not be handled before the end of the tracking
//		InvalRgn( rgn );

		GetClip( saveRgn );
		//	As the drawing will occur with a SetOrigin, we need to counter-balance this here
		//	(This is quite kludgy)
		OffsetRgn( rgn, 0, lib->offsetHeight );
		SetClip( rgn );
		LibraryDrawWindowContent( lib );
		SetClip( saveRgn );
		DisposeRgn( saveRgn );

		DisposeRgn( rgn );
		
		SetPort( savePort );
	}
}

short LibraryScroll( LibraryPtr lib, short delta )
{
	LibraryScrollTo( lib, lib->offsetHeight+delta );
}

int LibraryGetColumns( LibraryPtr lib )
{
	return lib->columns;
}

int LibraryGetCount( LibraryPtr lib )
{
	return lib->count;
}

void LibraryXXX( LibraryPtr lib )
{
	LibaryUpdateContentSizes( lib );
	LibraryUpdateScrollVisible( lib );
	LibraryAdjustScroll( lib );
}

void LibraryScrollToIndex( LibraryPtr lib, int index )
{
	short line = index/lib->columns;
	Rect r;

	LibraryGetRect( lib, index, &r );

	if (line==0 || line==lib->lines-1)
		InsetRect( &r, 0, -(MARGIN_Y+1) );	//	Without the +1, there is a missing pixel at the bottom. Weird
	else
		InsetRect( &r, 0, -(INTER_Y-1) );	//	The -1 is so we don't get the shadow of the previous line

	if (r.top<lib->offsetHeight)
		LibraryScrollTo( lib, r.top );
	if (r.bottom>lib->offsetHeight+lib->visibleHeight)
		LibraryScrollTo( lib, r.bottom-lib->visibleHeight );
}

void LibraryGet( LibraryPtr lib, int index, Str255 fName, short *vRefNum, long *dirID )
{
	struct LibraryEntryFlim *lef = GetLEF( lib, index );
	StrCpyPP( fName, (void*)lef->fName );
	*vRefNum = lef->vRefNum;
	*dirID = lef->dirID;
	LibraryUnloadEntry( lib, index );
}
