
#include "Tips.h"
#include "Preferences.h"
#include "Util.h"
#include "Resources.h"

#include <Dialogs.h>
#include <ToolUtils.h>

DialogPtr gTipsDialog;

static void DisplayNextTip()
{
	Str255 theCurrentTip;
	Str255 theCurrentBtn;
	short theType;
	Handle theItem;
	Rect theRect;

	if (!IsVisibleTips())
		return ;

	GetIndString( theCurrentTip, 128, PreferencesGetNextTipIndex()+1 );
	if (theCurrentTip[0]==0)
	{
		PreferencesSetNextTipIndex( 0 );
		GetIndString( theCurrentTip, 128, PreferencesGetNextTipIndex()+1 );
	}
	
	PreferencesSetNextTipIndex( PreferencesGetNextTipIndex()+1 );

	GetDItem( gTipsDialog, kTipItemID, &theType, &theItem, &theRect );
	SetIText( theItem, theCurrentTip );


	GetIndString( theCurrentBtn, 129, PreferencesGetNextTipBtnIndex()+1 );
	if (theCurrentBtn[0]==0)
	{
		PreferencesSetNextTipBtnIndex( 0 );
		GetIndString( theCurrentBtn, 129, PreferencesGetNextTipBtnIndex()+1 );
	}
	
	PreferencesSetNextTipBtnIndex( PreferencesGetNextTipBtnIndex()+1 );

	GetDItem( gTipsDialog, kNextTipItemID, &theType, &theItem, &theRect );
	SetCTitle( theItem, theCurrentBtn );

	PreferenceSave();
}

Boolean IsTips( DialogPtr aDialog )
{
	return aDialog && aDialog==gTipsDialog;
}

void DisposTips()
{
	if (gTipsDialog)
	{
		HideWindow( gTipsDialog );
		DisposDialog( gTipsDialog );
		gTipsDialog = NULL;
	}
}

Boolean IsVisibleTips()
{
	return !!gTipsDialog;
}

void ToggleTips()
{
	if (!gTipsDialog)
	{
		short theType;
		Handle theItem;
		Rect theRect;

		gTipsDialog = GetNewDialog( kDLOGTipsID, NULL, (WindowPtr)-1 );

		assert( !!gTipsDialog, "Tips dialog not found" );

		GetDItem( gTipsDialog, kShowTipItemID, &theType, &theItem, &theRect );
		SetCtlValue( theItem, PreferencesGetShowTipsStartup() );

		DisplayNextTip();
		UtilPlaceWindow( gTipsDialog, 0.95 );
		ShowWindow( gTipsDialog );
	}
	else
		DisposTips();
}

Boolean DoTipsSelect( short anItem, EventRecord *anEvent )
{
	switch (anItem)
	{
		case kNextTipItemID:
			DisplayNextTip();
			return TRUE;
		case kShowTipItemID:
		{
			short theType;
			Handle theItem;
			Rect theRect;
			GetDItem( gTipsDialog, kShowTipItemID, &theType, &theItem, &theRect );
			PreferencesSetShowTipsStartup( !GetCtlValue( theItem ) );
			SetCtlValue( theItem, PreferencesGetShowTipsStartup() );
			PreferenceSave();
			return TRUE;
		}
	}

	return FALSE;
}
