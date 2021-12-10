//	-------------------------------------------------------------------
//	GENERAL LOG FUNCTIONS
//	-------------------------------------------------------------------

void assert( int v, const char *msg );

//	-------------------------------------------------------------------
//	Basic log functions to be able to log from within an interrupt handler
//	-------------------------------------------------------------------

void dinit_log( void );
void dlog( const char *data, int len );
void dlog_str( const char *str );
void dlog_int( long num );



//	Must not be called from interrut -- writes log to stdout
void exec_log( void );
