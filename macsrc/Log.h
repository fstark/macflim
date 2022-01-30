#ifndef LOG_INCLUDED__
#define LOG_INCLUDED__

//	-------------------------------------------------------------------
//	GENERAL LOG FUNCTIONS
//	-------------------------------------------------------------------
//	Basic log functions to be able to log from within an interrupt handler
//	Log functions are only available if compiled in VERBOSE
//	-------------------------------------------------------------------

#include "Config.h"

#ifdef VERBOSE
	#define MAX_LOG_SIZE	50000L	//	50 Kb of logs max

	//	-------------------------------------------------------------------
	//	Inits the log system
	//	-------------------------------------------------------------------

	void dinit_log( void );

	//	-------------------------------------------------------------------
	//	Logs raw bytes
	//	-------------------------------------------------------------------

	void dlog( const char *data, int len );

	//	-------------------------------------------------------------------
	//	Logs c string
	//	-------------------------------------------------------------------

	void dlog_str( const char *str );

	//	-------------------------------------------------------------------
	//	Logs number
	//	-------------------------------------------------------------------

	void dlog_int( long num );

	//	-------------------------------------------------------------------
	//	Must not be called from interrut -- writes log to stdout
	//	-------------------------------------------------------------------
	void exec_log( void );

#else
	#define MAX_LOG_SIZE	0L	//	No log

	#define dinit_log( x ) do {} while(0)
	#define dlog( x, y ) do {} while(0)
	#define dlog_str( x ) do {} while(0)
	#define dlog_int( x ) do {} while(0)

	//	Must not be called from interrut -- writes log to stdout
	#define exec_log( x ) do {} while(0)
#endif

#endif
