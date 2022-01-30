//	The tip-of-the-day management

//	TRUE if tips window currently visible
Boolean IsVisibleTips( void );

//	True if dialog is tip dialog
Boolean IsTips( DialogPtr aDialog );

//Boolean DoTipEvent( EventRecord *anEvent );

//	Shows/Hide the tips window
void ToggleTips( void );

//	Handle interaction with the dialog
void DoTipsSelect( short anItem, EventRecord *anEvent );

//	Clean-up
void DisposTips( void );
