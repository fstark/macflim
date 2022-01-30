#ifndef RESOURCES_INCLUDED__
#define RESOURCES_INCLUDED__

//	-------------------------------------------------------------------
//	Windows
//	-------------------------------------------------------------------

#define kWindowLibraryID				128

//	-------------------------------------------------------------------
//	Menus
//	-------------------------------------------------------------------

#define kMenuBarID	0

#define kMenuAppleID					128
enum {	kMenuItemAboutID = 1 };

#define	kMenuFileID						129
enum {	kMENUItemShowTips = 1,
		kskip00,
		kMENUItemPreferences,
		kskip01,
		kMenuItemQuitID };

#define kMenuEditID						130
enum {	kMENUItemSelectAll = 7 };

#define kMENULibrary					131
enum {	kMenuItemAddFlimID = 1,
		kMenuItemAddFolderID,
		kSkip20,
		kMENUItemPlay,
		kMENUItemRemove,
		kMENUItemCheckIntegrity,
		kMENUItemAutostart,
		kSkip21,
		kMENUItemLoopCheck,
		kMENUItemSilentCheck };


//	-------------------------------------------------------------------
//	Preferences
//	-------------------------------------------------------------------

#define kPreferenceID					128

//	-------------------------------------------------------------------
//	ALRT
//	-------------------------------------------------------------------

#define kAlertOpenFlimErrorID 			137
#define kALRTErrorNonFatal				140
#define kALRTErrorNoBWScreen			142		//	No BW screen present
#define kALRTPreferencesRestart			143		//	Need to restart to apply prefs

//	-------------------------------------------------------------------
//	Dialogs
//	-------------------------------------------------------------------

#define kDialogHelpID 					128

#define kDLOGCheckProgress 				129
	#define kProgressItem 				1

#define kDialogSetTypeID				130
	#define kSetTypeButtonOk			1

#define kDialogCheckIntegrityID			131
	#define kCheckIntegrityButtonOk		2

#define kDLOGAbout 						134
#define kDLOGThx 						132

#define kDialogPreferenceID 			135
	#define kPreferenceButtonOk			1
	#define kPreferenceCheckVBL			2
	#define kPreferenceMaxBufferSize	3

#define kDLOGFatal						136

#define kDLOGProgress					144

//	-------------------------------------------------------------------
//	Alerts
//	-------------------------------------------------------------------

#define kAlertNoChecksumID				133

#define kAlertCorruptedID				131

#define kALRTConfirmAutoplay			139

//	-------------------------------------------------------------------
//	PICT
//	-------------------------------------------------------------------

#define kPolaroidBadgesID				129

#define kPolaroidSoundBadgeID			136
#define kPolaroidSilentBadgeID			137	//	Needs to be consecutive

#define kPICTGrowZone					138

#define kPICTPolaroidSelfPlayBadge		139

//	-------------------------------------------------------------------
//	DLOG
//	-------------------------------------------------------------------

#define kSFAddFlimDialogID					2000
	#define kSFAddFlimHelpTextID			11

#define kSFAddFolderDialogID				2001
	#define kSFAddFolderHelpTextID			11
	#define kSFAddFolderChooseDirectoryID	12

#endif
