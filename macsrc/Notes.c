//	Can contain various notes

#if 0

TODO:
	PREF for SELF?
	VERSION for SELF?

#### Use Command-C during playback to copy the current frame in the Clipboard. You can print it and put it on your wall!



A974 : Button

* Manage play commands (at least) abort (Button) from VBL
* Mouse selection is a bit off (click on right border of polaroid)
* Check all resource IDs in Resources.h
* Don't add duplicate
* Do not show flim already in the Library in SFPGetFile
* Sort flims
* Drag/drop reorder
* When selected, play should indicates the number of selected flims
* When a single one, play indicates the name in the Play Menu
* Add a "Restore Default" in preference dialog.
* Changing flim types should not remove auto play
* Empty state for Library	

* [DONE] Play commands (restart/prev/next/etc) in ApplyPlay
* [DONE] Remove VBL from preferences
* [DONE] Adds for type/creator when opening flim files
* [DONE] Put "Show all flims" into the SFPGetFile dialog itself (remove option trick), or add it in preferences
* [DONE] Poster should use flim filter
* [DONE] Double-clic on library crashes
* [DONE] Select directory of SFPGetFile is wrong
* [DONE] Better handling of keys when multiple windows
* [DONE] "Cannot open flim" : dialog uncentererd + no mouse
* [DONE] Incomprehensible crash if buffers too small
* [DONE] Flim 'A' makes MacFlim crash
* [DONE] Escape deselects all.
* [DONE] When no selection, play should be "Play All Library".
* [DONE] Key scrolling keeps margins around polaroids
* [DONE] Progress when adding flims
* [DONE] Fixed Abort() when multiple windows
* [DONE] Manage non-BW screens (how to detect?)
* [DONE] Slow scroll redraw
* [DONE] Arrow key redraw performance
* [DONE] Redo menus
* [DONE] Play flims from finder
* [DONE] Make window front when clicking in content
* [DONE] Add tip menu (show/hide)
* [DONE] Make Tips window front
* [DONE] Tips window close
* [DONE] Restore tips
* [DONE] Implement Loop
* [DONE] Implement Force Silent
* [DONE] "This flim is of the wrong type/creator" => add name
* [DONE] Check flim integrity
* [DONE] Play multiple flims
* [DONE] Manage selection issue (clip)
* [DONE] Mouse selection when scrolled
* [DONE] Imperfect mouse selection (missing bottom of clip)
* [DONE] Scrolling performance
* [DONE] Update of scrollbar with delete
* [DONE] Arrow keys selection
* [DONE] Implement multi-selection (shift)
* [DONE] Polaroid badge position
* [DONE] Better library place window
* [DONE] Add directory
* [DONE] Change open file to add "check all files"
* [DONE] About panel
* [DONE] Preferences
* [DONE] Restore Mini Player
* [DONE] Delete flim
* [DONE] Autosize flim window
* [DONE] Scroll flim window
* [DONE] Resize flim window
* [DONE] Crash setting flim type
* [BUG] Click on bottom of polaroid deselects
* [DONE] Space pauses the flim at startup
* [DONE] Use enter to play flims
* [DONE] Add a "Library/Play" menu item
* [DONE] Add a "Library/Make Autoplay" menu item
* [DONE] Implement "Edit/Select All" menu item
* [DONE] "Zoom in/out" effect when playing.
* [FAILED] Add 'self play' badge
* [DONE] Create autostart from library
* [DONE] Clicking arrows deselects
* [DONE] Arrow key scroll
* [DONE] Arrows don't work on Mac Plus

* naming variables : "from"/"to" (not "from"/"dest")

* Have an achievement system?
*   Rookie : Played a flim
*   Addict : Played 100 flims
* 	Early Adopter: Playing a flim on a Mac128K on Jan 24th (how to store that?)
*	Gotta Catch Them All: Played a flim on Mac XL, Mac 128, Mac 512, Mac 512Ke, Mac Plus, Mac SE, Mac Portable, Mac SE/30
*   You're Tearing Me Apart : Played a flim on a Lisa
*	dQw4w9WgXcQ : You didn't let me down.
*   Man of Taste : Played a flim on Jan 1st, 00:00
*   The Read Deal : played a flim on real hardware (how to check that?)


MFS = vRefNum (volume reference number) + name

SFGetFile used to return a vRefNum + a name

To support HFS on old code, SFGetFile returns a "wdRefNum", a "working directory ref num",
which is a memory based mapped of "working directory" (ie: it creates a pseudo disk for
every directory it returns).

After having called SF[P]GetFile, one has to call GetWDInfo
to get the read vRefNum and read dirID of the file

Attention: some call have "dirID" that reference the dirID of the object,
in case it is a directory, so you have to use the parID (parent ID).

#endif
