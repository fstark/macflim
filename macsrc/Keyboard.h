extern Boolean sEscape;
extern Boolean sSkip;
extern Boolean sPrevious;
extern Boolean sRestart;
extern Boolean sPause;
extern Boolean sMuted;
extern Boolean sHelp;
extern Boolean sPreferences;
extern Boolean sDebug;

Boolean TestKey( unsigned char *keys, char k );
void CheckKeys( void );

//	useless
Boolean CheckPause( unsigned char *keys );
Boolean CheckMute( unsigned char *keys );


