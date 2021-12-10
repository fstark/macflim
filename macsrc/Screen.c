//	-------------------------------------------------------------------
//	SCREEN HANDLING FUNCTIONS
//	-------------------------------------------------------------------
//	Save/restore screen
//	-------------------------------------------------------------------

#include "Screen.h"
#include "Config.h"
#include "Log.h"


#define RectHeight(r)		((r).bottom-(r).top)


void KillScreen( ScreenPtr scrn, short killcode )
{
	while (!Button())
		ScreenFlash( scrn, 0, (killcode+1)*16 );
	ExitToShell();
}

//	-------------------------------------------------------------------
//	Allocated memory and saves screen
//	-------------------------------------------------------------------

void SaveScreen( Ptr *ptr )
{
	long count = ((long)screenBits.rowBytes)*RectHeight(screenBits.bounds);

	*ptr = NewPtr( count );

	if (*ptr)
		BlockMove( screenBits.baseAddr, *ptr, count );
}

//	-------------------------------------------------------------------
//	Restore saved screen and deallocates memory
//	-------------------------------------------------------------------

void RestoreScreen( Ptr *ptr )
{
	if (ptr)
	{
		long count = ((long)screenBits.rowBytes)*RectHeight(screenBits.bounds);
		BlockMove( *ptr, screenBits.baseAddr, count );
		DisposPtr( *ptr );
		*ptr = NULL;
	}
}




    // {0x00,0x38,0x44,0x4c,0x54,0x64,0x44,0x38,0x00},
    // {0x00,0x10,0x30,0x50,0x10,0x10,0x10,0x38,0x00},
    // {0x00,0x38,0x44,0x04,0x38,0x40,0x40,0x7c,0x00},
    // {0x00,0x38,0x44,0x04,0x18,0x04,0x44,0x38,0x00},
    // {0x00,0x08,0x18,0x28,0x48,0x7c,0x08,0x08,0x00},
    // {0x00,0x7c,0x40,0x40,0x78,0x04,0x44,0x38,0x00},
    // {0x00,0x38,0x44,0x40,0x78,0x44,0x44,0x38,0x00},
    // {0x00,0x7c,0x04,0x04,0x08,0x10,0x10,0x10,0x00}
char sFont[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

static char Reverse( char c )
{
	int i;
	char r=0;

	for (i=0;i!=8;i++)
	{	
		r <<= 1;
		r |= c&01;
		c >>= 1;
	}
	
	return r^0xff;
}

static void ScreenFontInit( void )
{
	static Boolean inited = false;
	if (!inited)
	{
		int i;
		int j;
		
		for (i=0;i!=128;i++)
			for (j=0;j!=8;j++)
				sFont[i][j] = Reverse(sFont[i][j]);
		inited = true;
	}
}


static int logx =0;
static int logy =0;

static void InternalDrawChar( char c, char *p, size_t rowBytes )
{
	int i;
	
	ScreenFontInit();
	
    c &= 0x7f;

	for (i=0;i!=8;i++)
	{
		*p =sFont[c][i];
		p += rowBytes;
	}
	
	logx++;
}

static void ScreenPrint( ScreenPtr screen, char *p, const char *s )
{
	char c;
	while (c=*s++)
    {
        if (c>=32)
        {
            InternalDrawChar( c, p, screen->rowBytes );
            p++;
            if (logx==64)
            {
                p -= logx;
                p += 8*screen->rowBytes*64;
                logy++;
                logx = 0;
            }
        }
        else
            if (c=='\n')
            {  
                p -= logx;
                p += 8*screen->rowBytes*64;
                logx = 0;
                logy++;
            }
    }
}

static char *LogPtr( ScreenPtr screen )
{
	return screen->physAddr + (logy<<3)*screen->rowBytes;
}

void ScreenLogHome( ScreenPtr screen )
{
	logx = logy = 0;
}

void ScreenLogMoveTo( ScreenPtr screen, int x, int y )
{
	logx = x;
	logy = y;
}

void ScreenLogString( ScreenPtr screen, const char *s )
{
	ScreenPrint( screen, LogPtr( screen ), s );
}

#include <stdarg.h>

void ScreenLog( ScreenPtr screen, const char *format, ... )
{
	char buffer[1024];
	va_list ap;
	va_start( ap, format );
	vsprintf( buffer, format, ap );
	ScreenLogString( screen, buffer );
	va_end( ap );
}








//	-------------------------------------------------------------------
//	CODEC DISPLAY FUNCTIONS
//	-------------------------------------------------------------------
//	Display functions
//	All codec display functions are implemented twice:
//	- A slow, C-based, reference implementation that works on
//	  screen larger than 512x342 (usedful for development and testing)
//	- A faster, 68x assembly version, suitable for real-world mac
//	-------------------------------------------------------------------



//	-------------------------------------------------------------------
//	Codec 0x00 : null
//	-------------------------------------------------------------------

void Null_ref( char *dest, char *source, int rowbytes );
void Null_ref( char *dest, char *source, int rowbytes )
{
}

//	-------------------------------------------------------------------
//	Codec 0x01 : z16 (reference implementation)
//	  Copies series of vertical variable height 16 pixels information,
//	Format:
//	  A series of chunks
//	  2 byte header : (0x0000 to end)
//		9 bits offset from screen top
//		7 bits count of data
//	  1 byte        : offset (starts at top left of screen)
//	  1 byte        : count (0=>end of data)
//	  count words   : data to be copied at vertical 16 pixels line
//	-------------------------------------------------------------------

void UnpackZ16_ref( char *dest, char *source, int rowbytes );
void UnpackZ16_ref( char *dest, char *source, int rowbytes )
{
	register unsigned char *d = (unsigned char *)dest;
	register unsigned char *dmax = d+64;

	register unsigned short *s = (unsigned short *)source;
	register unsigned short header;
	register unsigned short rowshorts = rowbytes/2;

	while (header=*s++)
	{
		register unsigned short offset;
		register int count = 0;
		register unsigned short *adrs;

		offset = header>>7;
		
		d += offset;

			//	Adjust for screen larger than 512 pixels
		while (d>dmax)
		{
			d += rowbytes-64;
			dmax += rowbytes;
		}
		
		count = header&0x7f;

		adrs = (unsigned short*)d;

		while (count--)
		{
			*adrs = *s++;
			adrs += rowshorts;
		}
	}
}

void UnpackZ16_64( char *dest, char *source, int rowbytes );
void UnpackZ16_64( char *dest, char *source, int rowbytes )
{

	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screen address
		movea.l source,a3			;	a3 == source data

@chunk:
		move.w	(a3)+,d7			;	header
		beq.s	@exit				;	0x0000 => end of frame
									
		move.w	d7,d6
		lsr.w	#7,d6				;	High 9 bits of header is offset
		add		d6,a4				;	new screen address

		movea.l	a4,a2				

		and.w	#0x7f,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#1,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#2,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#3,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#4,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#5,d7
		beq 	@chunk

        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
		cmp 	#6,d7
		beq 	@chunk

		sub 	#7,d7
loop:
        move.w	(a3)+,(a2)
        add 	#64,a2		;	Take stride into account
@endloop:
		dbra d7,@loop
		bra @chunk

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Codec 0x02 : z32 (reference implementation)
//	  Copies series of vertical variable height 32 pixels information,
//	Format:
//	  A series of chunks
//	  4 byte header : (0x00000000 to end)
//		2 bytes     : count of data to copy offset from screen top, minus 1
//		2 bytes     : offset from the top of the screen, plus 4
//	  count quads   : data to be copied at vertical 32 pixels line
//	-------------------------------------------------------------------

void UnpackZ32_ref( char *dest, char *source, int rowbytes );
void UnpackZ32_ref( char *dest, char *source, int rowbytes )
{
	register unsigned long *base = (unsigned long *)dest;
	register unsigned long *s = (unsigned long *)source;
	register long header;
	register long rowlongs = rowbytes/4;

	while (header=*s++)
	{
		register unsigned int offset;
		register unsigned int copy;
		register unsigned long *d;
		
		offset = (header&0xffff)/4-1;
		
#ifdef REFERENCE_ASSERTS
		if (offset>=21888/4)
			assert( false, "Bad Z32 Offset" );
#endif
		
		offset = (offset/16)*rowlongs+(offset%16);
		
		copy = (header>>16)+1;
#ifdef REFERENCE_ASSERTS
		assert( copy>0, "Bad Z32 count (neg)" );
		assert( copy<=342, "Bad Z32 count" );
#endif

#ifdef REFERENCE_ASSERTS
		if (offset<0 || offset>=7524) // Lisa Hack
			printf( "BAD OFFSET %d HEADER %lx\n", (int)offset, (int)header );
#endif

		d = base+offset;

		while (copy--)
		{
			//*d = *s++;
			s++;
			*d = 0xaaaaaaaaL;
			//printf( "%lx\n", (long)d );
			d += rowlongs;
		}
	}
}

void UnpackZ32_64( char *dest, char *source, int rowbytes );
void UnpackZ32_64( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		subq.l #4,a4				;	minus4, as all offets are +4
		movea.l source,a3			;	a3 == source data

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		add.w	d7,a2				;	Low end of d7 is offset

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	#64,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

void UnpackZ32_80( char *dest, char *source, int rowbytes );
void UnpackZ32_80( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		movea.l source,a3			;	a3 == source data

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		subq.w #4,d7				;	offsets are +4
		move.w d7,d0
		and.w #0x3f,d0
		add.w d0,a2
		lsr.w #6,d7

				;	d0 = d7*80
//		move.w #80,d0				;	Explicit multiplication is 3% slower
//		mulu d7,d0

		move.w d7,d0				;	d0 = d7
		lsl.w #2,d0					;	d0 = d7 * 4
		add.w d7,d0					;	d0 = d7 * 5
		lsl.w #4,d0					;	d0 = d7 * 80

		add.w d0,a2

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	#80,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

void UnpackZ32_asm( char *dest, char *source, int rowbytes );
void UnpackZ32_asm( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		movea.l source,a3			;	a3 == source data
		move.w rowbytes,d1			;	d1 == rowbytes

@loop:
		move.l	(a3)+,d7			;	header
		beq.s	@exit				;	0x00000000 => end of frame

		movea.l	a4,a2				;	Screen base
		subq.w #4,d7				;	offsets are +4
		move.w d7,d0
		and.w #0x3f,d0
		add.w d0,a2
		lsr.w #6,d7

				;	d0 = d7*rowbytes
		move.w d7,d0
		mulu d1,d0
		add.l d0,a2					;	.l so we can add offsets > 32767

		swap	d7

@loop2:
        move.l	(a3)+,(a2)			;	Transfer data
        add 	d1,a2				;	Take stride into account
		dbra.w	d7,@loop2

        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}
}

//	-------------------------------------------------------------------
//	Codec 0x03 : invert (reference implementation)
//	Inverts the whole screen
//	-------------------------------------------------------------------

void Invert_ref( char *dest, char *source, int rowbytes );
void Invert_ref( char *dest, char *source, int rowbytes )
{
	int stride = (rowbytes-64)/4;
	register long *p = (long *)dest;

	register int y = 343;
	while (--y)
	{
		register int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += stride;
	}
}

void Invert_64( char *dest, char *source, int rowbytes );
void Invert_64( char *dest, char *source, int rowbytes )
{
	register long *p = (long *)dest;

	register int y = 343;
	while (--y)
	{
		register int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
	}
}

//	-------------------------------------------------------------------
//	Codec 0x04 : copy lines (reference implementation)
//	Copies a serie of horizontal lines
//	2 bytes     : count of bytes to copy
//	2 bytes     : offset to copy to
//	count bytes : data to copy
//	-------------------------------------------------------------------

void CopyLines_ref( char *dest, char *source, int rowbytes );
void CopyLines_ref( char *dest, char *source, int rowbytes )
{
	unsigned short len = ((unsigned short*)source)[0];
	unsigned short offset = ((unsigned short*)source)[1];

	if (offset<0 || offset>=21888)
		ExitToShell();
	if (len<0 || len>21888)
		ExitToShell();
	if ((len%64)!=0)
		ExitToShell();
	if (offset+len>21888)
		ExitToShell();

	source += 4;

//	offset = (offset/16)*(rowbytes/4)+(offset%16);
	offset = (offset/16)*(rowbytes/2)+(offset%16);	//	Fix for Lisa, rowbytes is not mult of 4
	offset>>=1;										//	Fix for Lisa, we are back to normal

	dest += offset;
	while (len)
	{
		BlockMove( source, dest, 64 );
		source += 64;
		dest += rowbytes;
		len -= 64;
	}
}

void CopyLines_64( char *dest, char *source, int rowbytes );
void CopyLines_64( char *dest, char *source, int rowbytes )
{
	short len = ((short*)source)[0];
	short offset = ((short*)source)[1];

	BlockMove( source+4, dest+offset, len );
}

//	-------------------------------------------------------------------
//	SCREEN HANDLING FUNCTION
//	-------------------------------------------------------------------
//	Dispatches to the right codec functions
//	Provides a few direct access routines
//	-------------------------------------------------------------------


//	-------------------------------------------------------------------
//	Fills scrn with information to display on the physical screen
//	-------------------------------------------------------------------

ScreenPtr ScreenInit( ScreenPtr scrn, size_t rowbytes )
{
	int i;

	int w = screenBits.bounds.right-screenBits.bounds.left;
	int h = screenBits.bounds.bottom-screenBits.bounds.top;

	assert( scrn!=NULL, "Screen not created" );

	assert( rowbytes==64, "Flim not supported" );

	scrn->physAddr = screenBits.baseAddr;
	scrn->height = h;

	scrn->rowBytes = screenBits.rowBytes;

	scrn->stride4 = (scrn->rowBytes-64)/4;

	scrn->baseAddr = scrn->physAddr + ((w-512)/2)/8;
	scrn->baseAddr += scrn->rowBytes*((h-342)/2);

//	Make sure the address is not odd
//	(for instance on a Lisa, the screen is 90 bytes wide, so there is a 13 bytes offset)
	scrn->baseAddr = (char *)(((long)(scrn->baseAddr))&0xfffffffeL);

	for (i=0;i!=kCodecCount;i++)
		scrn->procs[i] = Null_ref;

		//	This is slow, but work on every reasonable B&W macs
	scrn->procs[0] = Null_ref;
	scrn->procs[1] = UnpackZ16_ref;
	scrn->procs[2] = UnpackZ32_ref;
	scrn->procs[3] = Invert_ref;
	scrn->procs[4] = CopyLines_ref;

#ifndef REFERENCE
	scrn->procs[2] = UnpackZ32_asm;

	if (scrn->rowBytes==64)				//	Vintage macs
	{
		scrn->procs[1] = UnpackZ16_64;
		scrn->procs[2] = UnpackZ32_64;
		scrn->procs[3] = Invert_64;
		scrn->procs[4] = CopyLines_64;
	}

	if (scrn->rowBytes==80)				//	Macintosh Portable
	{
		scrn->procs[2] = UnpackZ32_80;
	}
#endif
	return scrn;
}

//	-------------------------------------------------------------------
//	Clear full physical screen to black (slow)
//	-------------------------------------------------------------------

void ScreenClear( ScreenPtr scrn )
{
	long *p = (long *)scrn->physAddr;
	long n = scrn->height*(long)scrn->rowBytes/4+1;
	
//	printf( "SCREEN BASE: %0lx\n", scrn->physAddr );

	while (--n)
	{
		*p++ = 0xffffffffL;
	}
}

//	-------------------------------------------------------------------
//	Flashes the screen (slow)
//	-------------------------------------------------------------------

void ScreenFlash( ScreenPtr scrn, size_t from, size_t lines )
{
	long *p = (long *)(scrn->baseAddr+from*scrn->rowBytes);
	size_t stride4 = scrn->stride4;

	int y = lines+1;
	while (--y)
	{
		int x = 17;
		while (--x)
			*p++ ^= 0xffffffffL;
		p += stride4;
	}
}

//	-------------------------------------------------------------------
//	Uncompress a frame on the screen
//	-------------------------------------------------------------------

void ScreenUncompressFrame( ScreenPtr scrn, char *source )
{
	char codec = source[3];
	if (codec<0 || codec>=kCodecCount)
		ExitToShell();
//	printf( "[%d/%lx/%d]", (int)codec, (long)scrn->baseAddr, (int)scrn->rowBytes );
	(scrn->procs[codec])( scrn->baseAddr, source+4, scrn->rowBytes );
}
