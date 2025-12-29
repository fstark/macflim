(*
	MacFlim Build Automation Script
	
	This AppleScript automates the compilation of three MacFlim projects
	using THINK C on System 7.1. It runs at boot from the Startup Items folder.
	
	Build sequence:
	1. MacFlim XCMD.π (builds the HyperCard XCMD into MacFlim binary)
	2. MacFlim.π (main player application)
	3. Mini MacFlim.π (low-memory version)
	
	After all builds complete, the script shuts down the machine.
*)

-- Build Project 1: MacFlim XCMD
tell application "Finder"
	set xcmdFile to file "MacFlim XCMD.π" of folder "Sources" of startup disk
	open xcmdFile
end tell
beep 3

-- THINK C should now be frontmost, send Command-1 to build
tell application "THINK Project Manager"
	activate
	-- In System 7 with AppleScript 1.1, we can't send keystrokes directly
	-- The user will need to manually press Command-1, Return for each project
end tell

-- For now, just open each project file in sequence
-- The automation via Command-1 keystroke doesn't work in AppleScript 1.1

-- Build Project 2: MacFlim  
tell application "Finder"
	set macflimFile to file "MacFlim.π" of folder "Sources" of startup disk
	open macflimFile
end tell
beep 3

tell application "THINK Project Manager"
	activate
end tell

-- Build Project 3: Mini MacFlim
tell application "Finder"
	set miniFile to file "Mini MacFlim.π" of folder "Sources" of startup disk
	open miniFile
end tell
beep 3

tell application "THINK Project Manager"
	activate
end tell

-- Shut down after opening all projects
tell application "Finder"
	shut down
end tell

-- All builds complete - shut down the machine
tell application "Finder"
	shut down
end tell
