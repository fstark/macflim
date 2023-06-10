#include "Mouse.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <Types.h>
#include "Screen.h"
#include <Quickdraw.h>

//	-------------------------------------------------------------------
//	GLOBALS
//	-------------------------------------------------------------------

#define Mouse (*(Point *)0x830)

static Cursor sHandCursor = 
{
	{0x0000,0x4000,0x6000,0x7000,0x7800,0x7C00,0x7E00,0x7F00,0x7F80,0x7C00,0x6C00,0x4600,0x0600,0x0300,0x0300,0x0000},
	{0xC000,0xE000,0xF000,0xF800,0xFC00,0xFE00,0xFF00,0xFF80,0xFFC0,0xFFE0,0xFE00,0xEF00,0xCF00,0x8780,0x0780,0x0380},
	{1,1}
};

//	16 mouse positions, 16 lines
static unsigned long sMouseData[16][16];
static unsigned long sMouseMask[16][16];


void ComputeMouse( void )
{
	int pixel;
	int y;
	
	Handle h = (Handle)GetCursor( 6069 );
	BlockMove( *h, &sHandCursor, 68 );
	
	for (pixel=0;pixel!=16;pixel++)
	{
		for (y=0;y!=16;y++)
		{
			unsigned long data = sHandCursor.data[y];
			unsigned long mask = sHandCursor.mask[y];
			data <<= 16;
			mask <<= 16;
			data >>= pixel;
			mask >>= pixel;
			sMouseData[pixel][y] = data;
			sMouseMask[pixel][y] = ~mask;
		}
	}
}

static unsigned long sSave[16];
unsigned long *sSaveAdrs;

static void SaveMouseUnder( unsigned long *adrs )
{
	int y;
	unsigned long *s = sSave;

	sSaveAdrs = adrs;

	for (y=0;y!=16;y++)
	{
		*s++ = *adrs;
		adrs += gScreen->rowBytes/4;
	}
}

void RestoreMouse( void )
{
	int y;
	unsigned long *s = sSave;
	unsigned long *adrs = sSaveAdrs;

	for (y=0;y!=16;y++)
	{
		*adrs = *s++;
		adrs += gScreen->rowBytes/4;
	}
}

//	-------------------------------------------------------------------
//	
//	-------------------------------------------------------------------

void DrawMouse( void )
{
	short x = Mouse.h-sHandCursor.hotSpot.h;
	short y = Mouse.v-sHandCursor.hotSpot.v;
	unsigned long offset;
	unsigned long *mouseAdrs;
	int pixel;
	unsigned long *d;
	unsigned long *m;
	int rowBytes = gScreen->rowBytes;

	if (x<0) x = 0;
	if (y<0) y = 0;
	if (x>=gScreen->width-16) x = gScreen->width-16;
	if (y>=gScreen->height-16) y = gScreen->height-16;

	offset = y*(unsigned long)gScreen->rowBytes + (x>>3)&(~0x1);

	mouseAdrs = (unsigned long *)(gScreen->physAddr+offset);

	pixel = x&0xf;

	d = sMouseData[pixel];
	m = sMouseMask[pixel];

	SaveMouseUnder( mouseAdrs );

	for (y=0;y!=16;y++)
	{
		*mouseAdrs = (*mouseAdrs&*m++)|*d++;
		mouseAdrs = (unsigned long *)((char *)mouseAdrs+rowBytes);
	}
}
