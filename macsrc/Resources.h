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

#define kALRTErrorNonFatal				140
#define kALRTErrorNoBWScreen			142		//	No BW screen present

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
	#define kPreferenceShowAll			2
	#define kPreferenceSetTypeCreator	3
	#define kPreferenceMaxBufferSize	4

#define kDLOGFatal						136

#define kDLOGOpenFlimError	 			137

#define kDLOGPreferencesRestart			143
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
//	up to 136

#define kPICTGrowZone					138

#define kPICTPolaroidSelfPlayBadge		139

#define kPolaroidSoundBadgeID			140
#define kPolaroidSilentBadgeID			141	//	Needs to be consecutive

//	-------------------------------------------------------------------
//	DLOG
//	-------------------------------------------------------------------

#define kSFAddFolderDialogID				2001
	#define kSFAddFolderChooseDirectoryID	11

#endif
