#ifndef RESOURCES_INCLUDED__
#define RESOURCES_INCLUDED__

//	-------------------------------------------------------------------
//	Windows
//	-------------------------------------------------------------------

#define kWINDLibraryID					128

//	-------------------------------------------------------------------
//	Menus
//	-------------------------------------------------------------------

#define kMENUBarID						0

#define kMENUAppleID					128
enum {	kMENUItemAboutID = 1 };

#define	kMENUFileID						129
enum {	kMENUItemShowTips = 1,
		kskip00,
		kMENUItemPreferences,
		kskip01,
		kMENUItemQuitID };

#define kMENUEditID						130
enum {	kMENUItemSelectAll = 7 };

#define kMENULibraryID					131
enum {	kMENUItemAddFlimID = 1,
		kSkip20,
		kMENUItemPlayID,
		kMENUItemRemoveID,
		kMENUItemCheckIntegrityID,
		kMENUItemAutostartID,
		kSkip21,
		kMENUItemLoopCheckID,
		kMENUItemSilentCheckID };

#define kMENUDebugID					999
enum {	kMENUItemDebugID = 1 };

//	-------------------------------------------------------------------
//	Preferences
//	-------------------------------------------------------------------

#define kPREFResourceID					128

//	-------------------------------------------------------------------
//	ALRT
//	-------------------------------------------------------------------

#define kALRTCorruptedID				200
#define kALRTConfirmAutoplayID			201
#define kALRTErrorNoBWScreenID			202		//	No BW screen present

//	-------------------------------------------------------------------
//	DLOGs
//	-------------------------------------------------------------------

#define kDLOGScreenTooSmallErrorID		128

#define kDLOGCheckProgressID			129
	#define kProgressItem 				1

#define kDLOGSetTypeID					130
	#define kSetTypeButtonOk			1

#define kDLOGCheckIntegrityID			131
	#define kCheckIntegrityButtonOk		2

#define kDLOGThxID 						132

#define kDLOGNoChecksumID				133

#define kDLOGAboutID					134

#define kDLOGPreferenceID 				135
	#define kPreferenceButtonOk			1
	#define kPreferenceShowAll			2
	#define kPreferenceSetTypeCreator	3
	#define kPreferenceMaxBufferSize	4

#define kDLOGFatalID					136		//	Not used, for reference

#define kDLOGOpenFlimErrorID 			137

#define kDLOGErrorNonFatal				140

#define kDLOGTipsID 					141
	#define kNextTipItemID				1
	#define kTipItemID					2
	#define kShowTipItemID				3

#define kDLOGPreferencesRestart			143
#define kDLOGProgress					144

//	-------------------------------------------------------------------
//	PICTs
//	-------------------------------------------------------------------

#define kPICTPolaroidBadgesID			129
//	up to 136

#define kPICTGrowZone					138

#define kPICTPolaroidSelfPlayBadge		139

#define kPICTPolaroidSoundBadgeID			140
#define kPICTPolaroidSilentBadgeID			141	//	Needs to be consecutive

//	-------------------------------------------------------------------
//	SF DLOGs
//	-------------------------------------------------------------------

#define kSFAddFolderDialogID				2001
	#define kSFAddFolderChooseDirectoryID	11

#endif
