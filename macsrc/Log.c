#include "Log.h"

#include <stdio.h>
#include <string.h>

#include "Util.h"

//	-------------------------------------------------------------------
//	Generic assertion => break into debugger
//	-------------------------------------------------------------------

void assert( int v, const char *msg )
{
	if (!v)
	{
		Str255 buffer;
		strcpy( (char *)(buffer+1), msg );
		buffer[0] = strlen( msg );
		Abort( buffer );
		DebugStr( buffer );
	}
}


static char *dlog_buffer;
static char *dlog_head;
static char *dlog_tail;
static char *dlog_end;

//	-------------------------------------------------------------------
//	Inits the log system
//	-------------------------------------------------------------------

void dinit_log( void )
{
#ifdef VERBOSE
	dlog_buffer = (char *)NewPtr( 50000 );
	if (dlog_buffer==NULL)
	{
		ExitToShell();
	}
	dlog_head = dlog_buffer;
	dlog_tail = dlog_head;
	dlog_end = dlog_buffer+50000;
#endif
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
//	Logs raw bytes
//	-------------------------------------------------------------------

void dlog( const char *data, int len )
{
#ifdef VERBOSE
	if (len<dlog_end-dlog_head)
	{
		my_memcpy( dlog_head, data, len );
		dlog_head += len;
	}
#endif
}

//	-------------------------------------------------------------------
//	Logs c string
//	-------------------------------------------------------------------

void dlog_str( const char *str )
{
	int size = strlen( str );
	dlog( str, size );
}

//	-------------------------------------------------------------------
//	Logs number
//	-------------------------------------------------------------------

void dlog_int( long num )
{
	char buffer[255];
	NumToString( num, buffer );
	buffer[buffer[0]+1] = 0;
	dlog_str( buffer+1 );
}





//	Must not be called from interrut -- writes log to stdout
void exec_log( void )
{
	while (dlog_tail!=dlog_head)
	{
		printf( "%c", *dlog_tail++ );
		fflush( stdout );
	}
}
