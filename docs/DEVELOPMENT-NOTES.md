WinAudioDecodeR
---------------

## Development Notes

- Author: [R∃xK∀xLL](https://github.com/R3xK4xLL)
	- WinAudioDecodeR (2024)
		
- [About MIT License](https://en.wikipedia.org/wiki/MIT_License)

## Background

Before starting on this project, I had not done any C/C++ Development in well over a decade. The last version of the language standard that I used was C++03. Also, I had no prior development experience with directly using the WinAPI (Native Windows API).

This perhaps lead to some excessive documentation and comments within the codebase, as I stepped through and analyzed the existing solution, and tried to understand its functionality, while refactoring and redesigning it along the way.

## Project Goals

- Use an existing Open Source project as a starting point for a new Open Source Project, honoring the terms of the original Open Source license.
- Update and modernize some of the code to use some C\++11, C\++14, ..., C\++xx standard features. Use this as an opportunity to learn about some of the new language features and to use these more modern language standards.
- Make the Application more Object-oriented and less Procedural. This involved a significant refactor to isolate the original code and move its functionality to Classes.
- Use AI technologies for AI-assisted pair programming, and integrate those technologies into my existing software development workflow.
- Learn how the FLAC Library and FLAC API are used in a C/C\++ Program.
- Learn about and use the WinAPI. The WinAPI is a Native Flat API, and is the lowest-level of interaction between Applications and the Windows Operating System (except for Device Drivers which are even lower-level).
- Challenge myself to solve problems at a lower-level of Application Development, in a problem domain (Audio), that I have not worked in before.
- Learn how to use GitHub to host the Project and source code.
		
- See the [CHANGELOG.md](CHANGELOG.md) for a curated overview of changes.


## Credits

- Original Codebase Author: [James Chapman](https://github.com/jfchapman)
	- AudioTester v1.7 (2015)
		- [http://www.vuplayer.com/other.php](http://www.vuplayer.com/other.php)
		- AudioTester direct download: [http://www.vuplayer.com/files/audiotester.zip](http://www.vuplayer.com/files/audiotester.zip)