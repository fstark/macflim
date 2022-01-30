#include "Log.h"

#ifdef VERBOSE

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

//	-------------------------------------------------------------------

#include "Util.h"

//	-------------------------------------------------------------------

static char *dlog_buffer;
static char *dlog_head;
static char *dlog_tail;
static char *dlog_end;

//	-------------------------------------------------------------------

void dinit_log( void )
{
	dlog_buffer = (char *)NewPtr( 50000 );
	if (dlog_buffer==NULL)
	{
		ExitToShell();
	}
	dlog_head = dlog_buffer;
	dlog_tail = dlog_head;
	dlog_end = dlog_buffer+50000;
}

//	-------------------------------------------------------------------
//	Custom memcpy
//	-------------------------------------------------------------------

static void my_memcpy( char *d, const char *f, int len )
{
	while (len--)
		*d++ = *f++;
}

//	-------------------------------------------------------------------
//	Custom memcmp (unused)
//	-------------------------------------------------------------------

static int my_memcmp( char *d, const char *f, int len )
{
	while (len--)
		if (*d++!=*f++)
			return d[-1]-f[-1];
	return 0;
}

//	-------------------------------------------------------------------

void dlog( const char *data, int len )
{
	if (len<dlog_end-dlog_head)
	{
		my_memcpy( dlog_head, data, len );
		dlog_head += len;
	}
}

//	-------------------------------------------------------------------

void dlog_str( const char *str )
{
	int size = strlen( str );
	dlog( str, size );
}

//	-------------------------------------------------------------------

void dlog_int( long num )
{
	char buffer[255];
	NumToString( num, buffer );
	buffer[buffer[0]+1] = 0;
	dlog_str( buffer+1 );
}

//	-------------------------------------------------------------------

void exec_log( void )
{
	while (dlog_tail!=dlog_head)
	{
		printf( "%c", *dlog_tail++ );
		fflush( stdout );
	}
}

#endif
