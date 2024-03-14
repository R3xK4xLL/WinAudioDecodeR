# Changelog

WinAudioDecodeR
---------------

All notable changes to this Project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this Project loosely adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

- Placeholder for unreleased changes. Coming soon.

## [0.1.0] - 2024-03-14

### Added

- Created a new Solution and Project in Visual Studio.

- Added vcpkg support to Solution and Project.

- Added the Display of a Message Box, when a Program Thread Creation error occurs. Afterwards, the Program is terminated.

- Added support for Single CPU (w/ Single Thread) System Configurations.
	- Uses newly implemented support for opening File Streams using Direct I/O Modes.
	- This type of System Configuration would not likely not be encountered in the modern computing era, but it is now supported.

- Added support for displaying multiple Error Messages per File.

- Added support for displaying overall Task Progress in a Progress Window.
	- Made adjustments to the manual layout of various Controls to allow for the new Progress Window to be integrated into the Client Area.
	- Now the user will have additional visual information. Especially useful for longer running jobs.

- Added support for the display of a Status Bar Window at the bottom of the main Application Window.
	-  Made adjustments to the manual layout of various Controls to allow for the new Status Bar Window to be integrated into the Client Area.

- Added support for both the display of static text and dynamic text in the Status Bar Window.
	- Both static and dynamic text on the Status Bar, can now be set using the Asynchronous PostMessage() function.
	- Static/Dynamic Text updates throughout the Program, including from the GUI and Processing Threads is now supported.

- Added Menu support with the following Menu Items:
	- File Menu
		- Open File...
		- Open Folder...
		- Exit
	- Help Menu
		- Help
		- About...

- Added support for opening Network Shared Files and Mapped Network Drive Files.

- Added support for an Explorer-style Open File Dialog Box that is used when Opening Files from the Menu Bar with File-->'Open File...' .
	- Added a custom Hook Procedure for an Explorer-style Dialog Box.
		- Added support for very large files selections by the User within the Dialog Box.
			- The Hook Procedure can Dynamically Allocate Memory when the user's selected files exceed the size of the presently allocated filename buffer.
			- Stress tested up to a DWORD Max Size of 4,294,967,295 characters with 8,589,934,590 bytes of allocated Memory for the wchar_t filename buffer.
				- A Dialog Box filename buffer of this size is overkill in most real-world usage scenarios, but it was interesting to implement.
				- The filename buffer max size only works on a 64-bit OS and 64-bit Hardware due to the addressable Memory limitations of 32-bit Hardware and a 32-bit OS.

- Added support for an Shell-style Open Folder Dialog Box that is used when Opening a Folder from the Menu Bar with File-->'Open Folder...' .
	- Added BrowseFolderCallback function that set the initial default folder/location when the Dialog Box is opened.
	- Added support for opening Network Shared Folders and Mapped Network Drive Folders.
	- Shell-style Open Folder Dialog Box is centered within the main Application Window when opened.

- Added an Execution Manager to support backround Asynchronous Thread-based Folder processing.
	- It was observered that opening or dragging and dropping large Folder/SubFolder structures would cause the Main GUI Thread to hang, resulting in the GUI becoming unresponsive.
	- Dragging and Dropping multiple large folder structures is now supported without hanging the Main GUI Thread.
	- Large folder structures can be opened via the Open Dialog Box, without hanging the Main GUI Thread.
	- The Asynchronous search/traverseral of large folder structures can now be explicitly stopped by the User.

- Added additional Application Logic for the concept of the Stopping State.
	- While the Application is in the Stopping State, adding further Input Data (File and Folders) is not allowed. This is typically performed by adding Input Data via Drag N Drop, the File Dialog Box, and the Folder Dialog Box.
		- Added the Display of a Message Box informing the User the operation is not allowed, when a User attempts to add Input Data while in the Stopping State.

- Added support for Opening File/Folders Paths via Callback Functions to the Drag-and-Drop function, the Open File Dialog Box function, and the Open Folder Dialog Box function.
	- The usage of Callback Functions for opening Paths, allows for code reuse involving preparing a Path for opening. The Path opening behavior can now be varied by Callback Function.

- Added Marquee-style Progress Window to the Status Bar.
	- Used to show the User indeterminate progress while Folder structures are being traversed.

- Created Application Manager Class that is used to manage custom Application Data and Support Logic.
	- Refactored out many Global Variables from Main Program area and encapsulated them within the Application Manager as Instance variables.
			- Added Getter/Setter functions to the Application Manager to support encapsulation.
	- Added COM Library support to Application Manager.
		- COM Library is initialized in the Constructor and the COM Library is closed in the Destructor.
		- COM Library added to support SHBrowseForFolder() and SHGetPathFromIDList() functions.
	
- Created Decoder Manager Class that is used to manage custom Decoder Creation and Support Logic.

- Implemented splicing a List (Linked-List) created by the Execution Manager's Thread with the Application Manager's List (Linked-List) that is used for Thread-based Processing.
	- Using the Splice() function on the two Linked-Lists is a O(1) Constant Time operation.
	- This optimization significantly reduced the number of calls from the Execution Manager into the Critical Section, while Asynchronously processing Folders.

- Created a new About Dialog Box.
	- Created Dialog Box Procedure function.
	- Dialog Box is centered within the main Application Window when opened.

### Changed

- Updated Software Library Dependencies.
	- Ogg (v1.3.2 \--> v1.3.5)
	- Vorbis (v1.3.4 \--> v1.3.7)
	- FLAC (1.3.1 \--> v1.4.3)
	- WavPack (v4.70 \--> v5.6.0)

- Updated various sections of code to take advantage of modern (C\++11 to C\++14) language features.
	- Usage of Smart Pointers.
	- Usage of 'nullptr' to replace usages of Zero '0' or NULL Macro for pointers.
	- Usage of the 'default' keyword in Constructor/Destructor declarations.
	- Usage of uniform initialization and value initialization in Class Definitions (for member variables), and throughout other implementation code.
	- Usage of 'delete' keyword in Class Definitions for Copy Constructor, Assignment Operator (Overloaded), Move Constructor, and Move Assignment Operator (Overloaded).
	- Replaced usage of \__int64 (Legacy MS VC++ Compiler Type) with long long (C++11 Type).
	- Replaced many MACRO Preprocessor Directives (#DEFINE CONSTANT) that were used for Constants with the usage of (constexpr CONSTANT).

- Refactored variable names, function names, classes, and various code sections for readability and maintainability. Resolved many Compiler Warnings.
	- During refactoring and redesign, an attempt was made to preserve and continue support for 32-bit Builds. Source code was updated and adjusted as needed to keep legacy 32-bit support intact.
	- Significantly reduced the usage of Global Variables throughout the codebase.
	- Refactored the usage of numeric data types to eliminate most numeric data type conversion (loss of information) Compiler Warnings.
	- Added extensive inline comments and XML Documentation comments, to codebase.
	- Added inline URI hyperlink comments to many C/C++ Library and WinAPI functions, variables, objects.
		- This was useful for analyzing and quickly referencing Library and API function information, when needed.
	- Added explicit Include/Macro Guards to Header Files.
	- Added verbose comments throughout the codebase. 
	- Added verbose C++ code statements throughout the codebase.
	- Added Parenthesis to complex expression to explicitly indicate Operator Precedence And Associativity Rules.
		- Explicit usage of parentheses raises the readability and makes it clear how a non-trivial compound expression should evaluate.

- Updated various sections of code to take advantage of newer WinAPI features.
	- Upgraded from 32-bit Timer usage (e.g. GetTickCount() function) to 64-bit Timer usage (e.g. GetTickCount64() function).
	
- Isolated the main WinAPI functionality (WinMain(), Message Event Loop, Window Procedures, Thread Procedure, etc) from the rest of the codebase, by making it depend upon other code through the use of abstraction like Classes/Objects.

- Updated the handling of Drag and Drop File processing.
	- Removed the existing fixed 4096 character buffer for Drag N' Dropped filenames.
	- Replaced with support for Dynamic Buffer Allocation for the character filenames Buffer. The filenames Buffer Size is now based upon the maximum size of the filenames Dragged and Dropped by the User.
	- Stress tested up to a UINT Max Size of 4,294,967,295 characters with 8,589,934,590 bytes of allocated Memory for the wchar_t filenames buffer.
		- A character filename buffer of this size is overkill in most real-world usage scenarios, but it was interesting to implement.
		- The filename buffer max size only works on a 64-bit OS and 64-bit Hardware due to the addressable Memory limitations of 32-bit Hardware and a 32-bit OS.
	- Added support for Dragging and Dropping the following: Network Share Files, Mapped Network Drive Files, Network Shared Folders, and Mapped Network Drive Folders.

- Refactored CStream I/O Wrapper Class.
	- Extensive code refactoring and increased readability.
	- Added private helper functions.
	- Added support for opening File Streams using Direct I/O Modes, either full-buffered OR unbuffered are supported.
	- Added the usage of security enhanced function for reading Files.
	- Added the option of reading into a File Memory Buffer using either smaller fixed-length Blocks (64k) OR reading a single large Block that matches the size of the File in bytes.

- Changed the Initialization of the Critical Section Object from using the InitializeCriticalSection() function to using the InitializeCriticalSectionAndSpinCount() function.
	- The Critical Section Object is now initialized to take advantage of spin counts to enhance synchronized Multi-threaded performance.

- Refactored ScanFolder() and rScanFolder() functions with Callback Function parameters.
	- Callback Function support added to ScanFolder() functions.
	- This new implementation allows for varying the operation performed on found Supported Types, using a specified Callback Function.

- Changed FlacDecoder Class
	- Added support for detecting and responding to Bad Metadata errors (FLAC__STREAM_DECODER_ERROR_STATUS_BAD_METADATA).
	- Added commments throughout the Flac Decoder Class, many of which reference the FLAC Library API.
		- Increased the readability and maintainability of the existing code.
	- Added support for detecting the presence of ID3v1 Tags when Sync Errors occur at the end of a FLAC Stream.
		- [See Forum Post for details](https://hydrogenaud.io/index.php/topic,30673.0.html)
	- Refactored FLAC Decoder Error Message construction and formatting, for error messages that involve time-based values.
		- Replaced usage of 'unsigned int' and swprintf_s() function calls, with the usage of std::wostringstream and the wcscpy_s() function.
		- The usage of 'unsigned int' with the swprintf_s() format specifier(s) %u or %d, limited time-based calculations to a maximum of 65,535 seconds.
		- Using C++ 'unsigned long long' and the wostringstream Object to write to a string buffer, removes this time-based limitation from error messages.

- Changed MP3Decoder Class
	- Refactored and added many comments to the MP3Decoder Class. 
		- Significantly increased the readability and maintainability of the existing code.

- Changed WavPackDecoder Class
	- Added support for using WavPack 5.x.x Library functions and features.
	- Refactored and added many comments to the WavPackDecoder Class.
		- Increased the readability and maintainability of the existing code.
	- Added support for opening Files with WavpackOpenFileInputEx64() function and the usage of WavpackStreamReader64 Reader. Added Reader Callback Functions to support 64-bit Reader.
		- The 64-bit build supports opening Files larger than 2GB.
		- The 32-bit build is limited to opening Files 2GB or less.
	- Added support for WavPack Files that contain DSD Audio Data.
		- MD5 Checksum Algorithm updated to support DSD Audio.

- Changed OggVorbisDecoder Class
	- Refactored and added many comments to the OggVorbisDecoder Class.

### Removed

- Removed explicit call to InitCommonControls() function in WinMain() function.
	- [See Article Reference #1 for details](https://geoffchappell.com/studies/windows/shell/comctl32/api/commctrl/initcommoncontrols.htm)
	- [See Article Reference #2 for details](https://geoffchappell.com/studies/windows/shell/comctl32/api/commctrl/initcommoncontrolsex.htm)

### Fixed

- Resolved many Compiler Warnings.
	- Refactored the usage of numeric data types to eliminate most numeric data type conversion (loss of information) Compiler Warnings.
