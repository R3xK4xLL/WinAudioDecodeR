#include "ApplicationManager.h"

using namespace std;

// ****************************************************************************
//							Application Manager
// ****************************************************************************

MainApplication::ApplicationManager::ApplicationManager(LPCRITICAL_SECTION lpCriticalSection)
{
	this->criticalSectionPtr = lpCriticalSection;

	this->ApplicationManagerStartup();

	// Initializes the COM Library for use by the calling Thread, sets the Thread's concurrency model, and creates a new Apartment for the Thread, if one is required.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/objbase/ne-objbase-coinit
	HRESULT initializationResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(initializationResult)) {
		// Error initializing COM.
		throw exception("COM Library Initialization failed.");
	}
}

MainApplication::ApplicationManager::~ApplicationManager()
{
	// Clean up the Decoder Manager on the Heap.
	if (this->decoderManagerPtr != nullptr)
	{
		delete this->decoderManagerPtr;
		this->decoderManagerPtr = nullptr;
	}

	// Clean up the Execution Manager on the Heap.
	if (this->executionManagerPtr != nullptr)
	{
		delete this->executionManagerPtr;
		this->executionManagerPtr = nullptr;
	}

	for (map<wstring, list<wstring>*>::const_iterator iterator = this->filenameToErrorListMap.begin(); iterator != this->filenameToErrorListMap.end(); ++iterator)
	{
		// The current Entry (Key-Value Pair) in the Entry Set.
		wstring key = iterator->first;
		list<wstring>* value = iterator->second;
		delete value;
		value = nullptr;
	}

	this->CleanupDynamicStatusBarText();

	// Closes the COM Library on the current Thread, unloads all DLLs loaded by the Thread, 
	// frees any other resources that the Thread maintains, and forces all RPC connections on the Thread to close.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize
	CoUninitialize();
}

#pragma region Public_Functions_Region

void MainApplication::ApplicationManager::Startup(HINSTANCE hInstance, HWND hwndMainApplicationWindow)
{
	this->hInstance = hInstance;
	this->hwndMainApplicationWindow = hwndMainApplicationWindow;

	// Set the Mapping Mode for the Device Context to use Pixels.
	HDC hdcWindowDeviceContext = GetDC(hwndMainApplicationWindow); // Get the device context for the Window.
	SetMapMode(hdcWindowDeviceContext, MM_TEXT); // Set the mapping mode to MM_TEXT. Each logical unit is mapped to one device pixel. Positive X is to the right; positive Y is down.
	ReleaseDC(this->hwndMainApplicationWindow, hdcWindowDeviceContext); // Release the Device Context.
	
	// Create the Edit Window.
	this->CreateEditWindow(this->hInstance, this->hwndMainApplicationWindow, this->hwndEditWindow, this->wndprocEditProcedure);
	
	// Create a Task Progress Window.
	this->CreateTaskProgressWindow(this->hInstance, this->hwndMainApplicationWindow, this->hwndTaskProgressWindow);

	// Create the Button Window.
	this->CreateButtonWindow(this->hInstance, this->hwndMainApplicationWindow, this->hwndButtonWindow);

	// Create a Progress Window for each Logical CPU.
	this->CreateCPUProgressWindow(this->hInstance, this->hwndMainApplicationWindow, this->hwndProgressWindow);

	// Create a Status Bar Window that displays the various Status Messages at the bottom of the Main Application Window.
	this->CreateStatusBarWindow(this->hInstance, this->hwndMainApplicationWindow, this->hwndStatusBarWindow, this->hwndStatusBarProgressWindow);

	// Create Synchronization API Support Objects.
	this->CreateSynchronizationSupport(this->handleTerminateEvent, this->handlePendingEvent);

	// Create a Thread for each Logical CPU.
	this->CreateThreads(this->hwndMainApplicationWindow, handleFinishedEvent, this->handleThread);

	// All relevant Instance variables should be initialized before proceding forward.
	assert(this->hInstance != nullptr);
	assert(this->hwndMainApplicationWindow != nullptr);
	assert(this->hwndEditWindow != nullptr);
	assert(this->hwndButtonWindow != nullptr);
	assert(this->hwndTaskProgressWindow != nullptr);
	assert(this->hwndProgressWindow != nullptr);
	assert(this->hwndStatusBarWindow != nullptr);
	assert(this->hwndStatusBarProgressWindow != nullptr);
	assert(this->wndprocEditProcedure != nullptr);
	assert(this->handleThread != nullptr);
	assert(this->handleTerminateEvent != nullptr);
	assert(this->handlePendingEvent != nullptr);
	assert(this->handleFinishedEvent != nullptr);

	// Set the Timer.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-timerproc
	// The TIMERPROC is set to NULL, therefore the system posts a WM_TIMER message to the Application Queue.
	SetTimer(this->hwndMainApplicationWindow, TIMER_ID, TIMER, nullptr);
}

void MainApplication::ApplicationManager::Shutdown()
{
	// Destroys the specified Timer.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-killtimer
	KillTimer(this->hwndMainApplicationWindow, TIMER_ID);

	// Stop all of the Threads.
	this->StopThreads();
}

void MainApplication::ApplicationManager::ResizeControls(HWND hwnd)
{
	if (this->statusBarEnabledFlag == true)
	{
		this->ResizeControlsStatusBarEnabled(hwnd);
	}
	else 
	{
		this->ResizeControlsStatusBarDisabled(hwnd);
	}
}

void MainApplication::ApplicationManager::OpenFileDialogBox()
{
	// Use the Common File Dialog to open Files.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/commdlg/nf-commdlg-getopenfilenamew
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamew

	OPENFILENAME ofn{};
	LPOPENFILENAME lpOFN = &ofn; // A pointer to the OPENFILENAME structure.
	
	const UINT MAX_BUFFER_SIZE = 4096U; // Initial Buffer Size (4096 'wchar_t 16-bit Unicode Characters' translates to 4096 * 2 bytes = 8,192 bytes of Buffer storage).
	wchar_t* szFileNamePtr = new wchar_t[MAX_BUFFER_SIZE] {}; // Create an initial default Buffer allocation.

	ZeroMemory(lpOFN, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn); // The length in bytes of the structure.
	ofn.hwndOwner = this->hwndMainApplicationWindow; // Handle to the Parent Window.
	ofn.lpstrTitle = L"Select File(s)"; // Customize the Dialog Title.
	ofn.lpstrFilter = L"All Supported Types\0*.flac;*.fla;*.mp3;*.mp2;*.m2a;*.wv;*.ogg\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileNamePtr; // Buffer to store the selected File Path.
	ofn.nMaxFile = MAX_BUFFER_SIZE; // The size in characters/bytes of the Buffer pointed to by lpstrFile.
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
	ofn.lpfnHook = Lpofnhookproc; // A Dialog Window Hook Procedure.
	ofn.lCustData = reinterpret_cast<LPARAM>(nullptr); // Application-defined data that the System passes to the Hook Procedure identified by the lpfnHook member.

	// Open the File Dialog Box.
	if (GetOpenFileName(lpOFN))
	{
		// User clicked the OK Button.

		LPWSTR lpwstrOpenedFileBuffer = ofn.lpstrFile;
		wchar_t* openedFileBufferPtr = static_cast<wchar_t*>(lpwstrOpenedFileBuffer);

		// Below is the layout in memory, when multiple files are selected.
		// 
		// +------+----+--------+----+--------+----+-----+--------+----+----+
		// | path | \0 | file 1 | \0 | file 2 | \0 | ... | file n | \0 | \0 |
		// +------+----+--------+----+--------+----+-----+--------+----+----+

		// Below is the layout in memory, when a single file is selected.
		// 
		// +-----------+----+----+
		// | full path | \0 | \0 |
		// +-----------+----+----+

		// The zero-based offset, in characters/bytes, from the beginning of the path to the file name in the string pointed to by lpstrFile.
		// If the user selects one or more files, nFileOffset is the offset to the first file name.
		WORD firstFileNameOffset = ofn.nFileOffset;

		// The start of 1 or more null-terminated filename strings.
		wchar_t* filenamePtr = openedFileBufferPtr + firstFileNameOffset;

		// Determine whether multiple files have been selected.
		int fileCount = 0;
		wchar_t* filenameLoopVarPtr = filenamePtr;
		while (*filenameLoopVarPtr)
		{
			if (fileCount > 1)
			{
				// More than one filename encountered.
				break;
			}

			// Move the pointer to the next Substring. (Just past the previous Substring's null-terminating '\0' character).
			filenameLoopVarPtr += wcslen(filenameLoopVarPtr) + 1;
			fileCount++;
		}

		// Get the Directory Path.
		wchar_t* directoryPathPtr = nullptr;
		if (fileCount == 1)
		{
			// Case #1:Single file selection. Directory Path and Filename are combined into a single Full Path. Directory Path must be isolated.
			size_t totalSize = wcslen(openedFileBufferPtr);
			size_t fileNameSize = wcslen(filenamePtr);
			size_t directoryPathBufferSize = (totalSize - fileNameSize) + 1; // +1 to account for the null-terminating '\0' character.
			directoryPathPtr = new wchar_t[directoryPathBufferSize] {};

			// When using wcsncpy_s(), the truncating behavior can be enabled by specifying count equal to the size of the destination array minus one. 
			// The function will copy the first count wide characters and append the null wide terminator.
			wcsncpy_s(directoryPathPtr, directoryPathBufferSize, openedFileBufferPtr, directoryPathBufferSize - 1);
		}
		else
		{
			// Case #2: Multiple file selection. Directory Path is provided without a Filename attached on the end (Regular Path). Directory Path needs no further processing.
			// Extract the Directory Path (The first part of the Buffer).
			size_t directoryPathBufferSize = wcslen(openedFileBufferPtr) + 2; // +1 to account for the null-terminating '\0' character AND +1 to account for appending trailing backslash.
			directoryPathPtr = new wchar_t[directoryPathBufferSize] {};
			wcsncpy_s(directoryPathPtr, directoryPathBufferSize, openedFileBufferPtr, directoryPathBufferSize);

			// Append the trailing backslash to Directory Path.
			wcscat_s(directoryPathPtr, directoryPathBufferSize, L"\\");
		}
		
		// The addition of more Input Data (i.e. Files and Folders) is not allowed while the Application Manager is in the Stopping State.
		if (!this->IsStoppingState())
		{
			// Open the Files.
			this->OpenFiles(directoryPathPtr, filenamePtr);
		}
		else
		{
			MessageBox(this->hwndMainApplicationWindow, L"Files and Folders cannot be added during the Stop Operation. Please try again later when the Application is ready.", L"Unsupported Action", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
		}

		// Cleanup Memory.
		if (directoryPathPtr != nullptr)
		{
			delete[] directoryPathPtr;
			directoryPathPtr = nullptr;
		}

	}
	else
	{
		// User clicked the Cancel Button OR an error occurred.
		// If the user closed or canceled the Dialog Box, the CommDlgExtendedError() function return value is zero. 
		// Otherwise, a non-zero error code is returned.
		DWORD dialogErrorCode = CommDlgExtendedError();
		if (dialogErrorCode != 0)
		{
			// An error occurred, for now do nothing.
		}
	}

	if (ofn.lpstrFile != nullptr)
	{
		// Free the presently allocated Buffer memory for the Dialog Box.
		delete[] ofn.lpstrFile;
		ofn.lpstrFile = nullptr;
	}
}

void MainApplication::ApplicationManager::OpenSelectFolderDialogBox()
{
	// NOTE: COM Library initialization is required for both SHBrowseForFolder() and SHGetPathFromIDList() functions.

	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/ns-shlobj_core-browseinfow
	BROWSEINFO browseInfo{};
	ZeroMemory(&browseInfo, sizeof(browseInfo));

	browseInfo.lpszTitle = L"Select Folder OR Network Path";
	browseInfo.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;
	browseInfo.lpfn = BrowseFolderCallback;
	browseInfo.lParam = reinterpret_cast<LPARAM>(L"\\\\localhost\\"); // Default Path, used to set the initial selected folder/location in Callback Function.

	// Displays a Dialog Box that allows the User to select a Shell Folder.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shbrowseforfolderw
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shtypes/ns-shtypes-itemidlist
	LPITEMIDLIST itemIDListPIDL = SHBrowseForFolder(&browseInfo); // Returns a pointer to an ITEMIDLIST structure (PIDL), which contains a list of Item Identifiers.
	
	if (itemIDListPIDL != NULL) {
		
		// The Buffer to receive the File System Path.
		// The maximum length for a path is MAX_PATH, which is defined as 260 characters. This Buffer must be at least MAX_PATH characters in size.
		// NOTE: In Windows 10 version 1607 and later, when Long Paths are enabled the MAX_PATH limit can be exceeded, allowing for up to a total of 32,767 characters in the Maximum Path.
		const UINT MAX_BUFFER_SIZE = 32768U;
		wchar_t* fileSystemPathBuffer = new wchar_t[MAX_BUFFER_SIZE]{};

		// Use File System Path to get the selected folder path.
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetpathfromidlistw
		BOOL selectedPathResult = SHGetPathFromIDList(itemIDListPIDL, fileSystemPathBuffer); // Converts a PIDL to a File System Path.
		
		if (selectedPathResult)
		{
			// The addition of more Input Data (i.e. Files and Folders) is not allowed while the Application Manager is in the Stopping State.
			if (!this->IsStoppingState())
			{
				// Open the selected folder path.
				std::function<void(const wchar_t*, bool)> boundCallback = std::bind(&ApplicationManager::AddFolderAsync, this, std::placeholders::_1, std::placeholders::_2);
				this->OpenPathWithCallback(true, boundCallback, fileSystemPathBuffer, true);
			}
			else
			{
				MessageBox(this->hwndMainApplicationWindow, L"Files and Folders cannot be added during the Stop Operation. Please try again later when the Application is ready.", L"Unsupported Action", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
			}
		}
		else
		{
			// Error getting the selected folder path. For now, do nothing.
			MessageBox(this->hwndMainApplicationWindow, L"An error occurred retrieving the selected Select Folder OR Network Path. Please try again with a different selection.", L"Folder OR Network Path Selection Error", MB_APPLMODAL | MB_ICONERROR | MB_OK);
		}

		// Free memory used by ITEMIDLIST structure (PIDL).
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cotaskmemfree
		CoTaskMemFree(itemIDListPIDL);
		itemIDListPIDL = nullptr;

		if (fileSystemPathBuffer != nullptr)
		{
			delete[] fileSystemPathBuffer;
			fileSystemPathBuffer = nullptr;
		}
	}
	else 
	{
		// User pressed the Cancel Button. For now, do nothing.
	}

}

void MainApplication::ApplicationManager::OpenFiles(wchar_t* directoryPathPtr, wchar_t* filenamePtr)
{
	// Process the selected files.
	size_t directoryPathSize = wcslen(directoryPathPtr);
	wchar_t* fullPathPtr = nullptr;

	// Bind the Callback Function.
	std::function<void(const wchar_t*, bool)> boundCallback = std::bind(&ApplicationManager::AddFileSync, this, std::placeholders::_1, std::placeholders::_2);

	// Iterate through the file names (The remaining parts of the buffer), while NOT the null-terminating '\0' character.
	wchar_t* filenameLoopVarPtr = filenamePtr;
	while (*filenameLoopVarPtr) {

		// Determine the Buffer size required to build the Full Path.
		size_t filenameSize = wcslen(filenameLoopVarPtr); // Get the buffer size required for the current Filename.
		size_t fullPathSize = directoryPathSize + filenameSize + 1; // The Full Path buffer size.
		fullPathPtr = new wchar_t[fullPathSize] {}; // Create a new Full Path on the Heap for this iteration.

		// Build the Full Path via Concatenation.
		wcscat_s(fullPathPtr, fullPathSize, directoryPathPtr);
		wcscat_s(fullPathPtr, fullPathSize, filenameLoopVarPtr);

		// Add only the supported file types.
		this->OpenPathWithCallback(true, boundCallback, fullPathPtr, false);

		// Move the pointer to the next Substring. (Just past the previous Substring's null-terminating '\0' character).
		filenameLoopVarPtr += wcslen(filenameLoopVarPtr) + 1;

		// Cleanup allocated memory in preparation for the next iteration.
		if (fullPathPtr != nullptr)
		{
			delete[] fullPathPtr;
			fullPathPtr = nullptr;
		}
	}

	// Process the selected Files via Thread-based Processing.
	this->SetPendingEvent();
}

void MainApplication::ApplicationManager::OpenPathWithCallback(bool messageBoxDisplayFlagEnabled, std::function<void(const wchar_t*, bool)> boundCallback, wchar_t* pathnamePtr, bool setPendingEventFlag)
{
	// Construct a path by concatenating the filename argument with the string "\\\\?\\".
	// This string is used to specify the path to the file in a format that allows for 
	// longer path names than the standard Windows API limit of 260 characters.
	// URI: https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#namespaces

	// For File I/O, the "\\?\" prefix to a path string tells the Windows APIs to disable 
	// all string parsing and to send the string that follows it straight to the file system.

	// For example, if the file system supports large paths and file names, you can exceed the 
	// MAX_PATH limits that are otherwise enforced by the Windows APIs. In editions of Windows 
	// before Windows 10 version 1607, the maximum length for a path is MAX_PATH, which is defined as 260 characters.
	// In Windows 10 version 1607 and later, Enable Long Paths must be set/enabled via the Registry, to exceed MAX_PATH limitations.

	bool isValidPath = false;
	wstring path;
	
    if (PathFileExists(pathnamePtr))
	{
		// Determines whether a path to a File System Object such as a File or Folder is valid (Includes Mapped Network Drive Files and Folders).
		// A Path specified by Universal Naming Convention (UNC) is limited to a File and Folder only (i.e. \\server\share\file AND \\server\share\folder is permitted).
		// A UNC path to a Server not permitted (i.e. \\server\ ).
		// A UNC path to a Server Share is permitted, however though the API documentation says that it is not. (i.e. \\server\share\ ).
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathfileexistsw
		
		if (wcsstr(pathnamePtr, FILE_NAMESPACE_UNC_PREFIX) != NULL || wcsstr(pathnamePtr, FILE_NAMESPACE_PREFIX) != NULL)
		{
			// A prefix already exists.
			// A substring match was found.
			isValidPath = true;
			path += pathnamePtr;
		}
		else if (PathIsUNC(pathnamePtr) && PathIsNetworkPath(pathnamePtr))
		{
			// If Network Share File, Network Share Folder, or Network Share Path, do NOT prepend a prefix.
			// NOTE: Without a prefix, the Maximum Path supported is MAX_PATH (260 Characters).
			isValidPath = true;
			path += pathnamePtr;
		}
		else
		{
			// If Regular File System or Mapped Network Drive Path, prepend the prefix.
			// NOTE: With a prefix, support for Long Paths is enabled, and the Maximum Path supported is up to 32,767 characters.
			path += FILE_NAMESPACE_PREFIX;
			path += pathnamePtr;
			isValidPath = true;
		}
	}
	else if (PathIsUNC(pathnamePtr))
	{
		// Path string is a valid Universal Naming Convention (UNC) path, as opposed to a Path based on a Drive Letter.
		if (PathIsNetworkPath(pathnamePtr))
		{
			// Path string represents a Network Resource.
			// Paths that begin with two backslash characters (\\) are interpreted as Universal Naming Convention (UNC) paths.
			// Paths that begin with a Letter followed by a colon (:) are interpreted as a Mounted Network Drive (e.g. Z: ).

			if (PathIsUNCServer(pathnamePtr))
			{
				if (messageBoxDisplayFlagEnabled)
				{
					// Path tring is a valid Universal Naming Convention (UNC) for a Server Path only (e.g. \\server ).
					MessageBox(this->hwndMainApplicationWindow, L"A Universal Naming Convention (UNC) Server Path was encountered, but it is not a supported Network Resource Path or a Mounted Network Drive.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
				}
			}
			else if (PathIsUNCServerShare(pathnamePtr))
			{
				if (messageBoxDisplayFlagEnabled)
				{
					// Path string is a valid Universal Naming Convention (UNC) Share Path, (e.g. \\server\share OR \\127.0.0.1\ OR \\localhost\ ).
					MessageBox(this->hwndMainApplicationWindow, L"A Universal Naming Convention (UNC) Share Path was encountered, but it is not a supported Network Resource Path or a Mounted Network Drive.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
				}
			}
			else
			{
				if (messageBoxDisplayFlagEnabled)
				{
					// The UNC Path likely does not exist.
					MessageBox(this->hwndMainApplicationWindow, L"A Universal Naming Convention (UNC) Path was encountered that may not exist.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
				}
			}
		}
		else
		{
			if (messageBoxDisplayFlagEnabled)
			{
				// A UNC Path was encountered, but it is not a Network Resource Path or a Mounted Network Drive.
				// The UNC Path likely does not exist.
				MessageBox(this->hwndMainApplicationWindow, L"A Universal Naming Convention (UNC) Path was encountered, but it is not a supported Network Resource Path or a Mounted Network Drive and may not exist.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
			}
		}
	}
	else
	{
		if (messageBoxDisplayFlagEnabled)
		{
			// An Unsupported Drive Letter Path was encountered.
			// The Drive Letter Path likely does not exist.
			MessageBox(this->hwndMainApplicationWindow, L"A Drive Letter Path was encountered, but it is not a supported Path and may not exist.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
		}
	}

	if (isValidPath)
	{
		// Add all supported types contained within the Folder and SubFolders.
		// Use the bound Callback Function to process the Path.
		boundCallback(path.c_str(), setPendingEventFlag);
	}
	else
	{
		if (messageBoxDisplayFlagEnabled)
		{
			MessageBox(this->hwndMainApplicationWindow, L"An invalid Path was specified. Please try again.", L"Invalid Selection", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
		}
	}
}

void MainApplication::ApplicationManager::OnDrop(HDROP hDrop)
{
	// Get the total number of Files that were Dropped.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-dragqueryfilew

	UINT initialDragFileIndex = 0xFFFFFFFF; // This is set so that a count of the files dropped is returned.
	UINT droppedFileCount = DragQueryFile(hDrop, initialDragFileIndex, nullptr, 0);

	// Find the maximum required size (in Characters) for the Filename Buffer, NOT including the terminating NULL character '\0'.
	UINT maximumRequiredBufferSize = 0U;
	for (UINT dragFileIndex = 0; dragFileIndex < droppedFileCount; ++dragFileIndex)
	{
		UINT requiredBufferSizeForDragFileIndex = DragQueryFile(hDrop, dragFileIndex, nullptr, 0);
		if (requiredBufferSizeForDragFileIndex > maximumRequiredBufferSize)
		{
			// Update the maximum allowed size for the Buffer.
			maximumRequiredBufferSize = requiredBufferSizeForDragFileIndex;
		}
	}

	maximumRequiredBufferSize += 1; // To account for the terminating NULL character '\0'.

	// Create a new dynamically allocated Buffer using the required Buffer size.
	wchar_t* targetFilenameBuffer = new wchar_t[maximumRequiredBufferSize] {};
	
	bool asyncFolderProcessingFlagEnabled = true;
	int asyncAddedFolderCount = 0;

	// Bind the Callback Functions.
	std::function<void(const wchar_t*, bool)> boundCallbackFolderAsync = std::bind(&ApplicationManager::AddFolderAsync, this, std::placeholders::_1, std::placeholders::_2);
	std::function<void(const wchar_t*, bool)> boundCallbackFolderSync = std::bind(&ApplicationManager::AddFolderSync, this, std::placeholders::_1, std::placeholders::_2);
	std::function<void(const wchar_t*, bool)> boundCallbackFileSync = std::bind(&ApplicationManager::AddFileSync, this, std::placeholders::_1, std::placeholders::_2);

	// Set the cursor to the wait cursor. (Creates a standard hourglass cursor)
	HCURSOR previousCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	// Process each File or Folder that was Dropped and add only the Supported File Types.
	for (UINT dragFileIndex = 0; dragFileIndex < droppedFileCount; ++dragFileIndex)
	{
		UINT charactersCopiedCount = DragQueryFile(hDrop, dragFileIndex, targetFilenameBuffer, maximumRequiredBufferSize);
		if (charactersCopiedCount > 0)
		{
			if (WinAPIUtils::IsFolderType(targetFilenameBuffer))
			{
				if (asyncFolderProcessingFlagEnabled)
				{
					// Process the Folder Asynchronously.
					this->OpenPathWithCallback(false, boundCallbackFolderAsync, targetFilenameBuffer, true);
					asyncAddedFolderCount++;
				}
				else
				{
					// Process the Folder Synchronously.
					this->OpenPathWithCallback(false, boundCallbackFolderSync, targetFilenameBuffer, false);
				}
			}
			else
			{
				// Process the File Synchronously.
				this->OpenPathWithCallback(false, boundCallbackFileSync, targetFilenameBuffer, false);
			}
		}
	}

	// Restore the cursor to its previous state.
	SetCursor(previousCursor);

	// Release the memory that the System allocated for use in transferring file names to the Application.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-dragfinish
	DragFinish(hDrop);
	
	// Cleanup Heap Memory.
	if (targetFilenameBuffer != nullptr)
	{
		delete[] targetFilenameBuffer;
		targetFilenameBuffer = nullptr;
	}
	
	// If Synchronous Folder Processing is enabled, this logic allows the SetPendingEvent() function to be called immediately.
	// If Asynchronous Folder Processing is enabled, this logic allows the Asynchronous Folder Processing Thread to call the SetPendingEvent() function at a later time.
	if (asyncFolderProcessingFlagEnabled == false || (asyncFolderProcessingFlagEnabled == true && asyncAddedFolderCount == 0))
	{
		// Process the selected Files via Thread-based Processing.
		this->SetPendingEvent();
	}
}

void MainApplication::ApplicationManager::AddSupportedTypeSync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{	
	// Set the cursor to the wait cursor. (Creates a standard hourglass cursor)
	HCURSOR previousCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	// Check if the file specified by the path is a Directory.
	if (WinAPIUtils::IsFolderType(filenamePtr))
	{	
		// Scan the contents of the Directory using the Main GUI Thread. (Synchronous Function Call)
		std::function<void(const wchar_t*)> boundCallback = std::bind(&ApplicationManager::AddToFileList, this, std::placeholders::_1);
		this->ScanFolderWithCallback(boundCallback, filenamePtr);
	}
	else
	{
		// The file is not a Directory. Check if the file is supported.
		this->AddFileSync(filenamePtr, false);
	}

	// Restore the cursor to its previous state.
	SetCursor(previousCursor);

	if (setPendingEventFlag == true)
	{
		// Process the selected Files and Folders via Thread-based Processing.
		this->SetPendingEvent();
	}
}

void MainApplication::ApplicationManager::AddSupportedTypeAsync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{
	// Check if the file specified by the path is a Directory.
	if (WinAPIUtils::IsFolderType(filenamePtr))
	{
		// Scan the contents of the Directory on a separate background Thread. (Asynchronous Function Call)
		// Uses ScanFolderAsync() as a Callback Function.
		this->executionManagerPtr->AddFolderToQueue(filenamePtr, setPendingEventFlag);
	}
	else
	{
		// The file is not a Directory.
		this->AddFileAsync(filenamePtr, setPendingEventFlag);
	}
}

void MainApplication::ApplicationManager::AddFileSync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{
	// Check if the file is supported.
	if (this->decoderManagerPtr->IsSupportedType(filenamePtr))
	{
		// Add Supported Type.
		this->AddToFileList(filenamePtr);
	}

	if (setPendingEventFlag == true)
	{
		this->SetPendingEvent();
	}
}

void MainApplication::ApplicationManager::AddFileAsync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{
	// Function stub, not implemented yet.
	throw exception("AddFileAsync() is a function stub, not implemented yet.");
}

void MainApplication::ApplicationManager::AddFolderSync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{
	// Scan the contents of the Directory using the Main GUI Thread. (Synchronous Function Call)
	std::function<void(const wchar_t*)> boundCallback = std::bind(&ApplicationManager::AddToFileList, this, std::placeholders::_1);
	this->ScanFolderWithCallback(boundCallback, filenamePtr);

	if (setPendingEventFlag == true)
	{
		// Process the selected Folder/SubFolders via Thread-based Processing.
		this->SetPendingEvent();
	}
}

void MainApplication::ApplicationManager::AddFolderAsync(const wchar_t* filenamePtr, bool setPendingEventFlag)
{
	// Scan the contents of the Directory on a separate background Thread. (Asynchronous Function Call)
	// Uses ScanFolderAsync() as a Callback Function.
	this->executionManagerPtr->AddFolderToQueue(filenamePtr, setPendingEventFlag);

	EnableWindow(this->hwndButtonWindow, true);  // Enable the Stop Button.
}

void MainApplication::ApplicationManager::SetPendingEvent()
{
	// Wait for the Finished Events for each Thread per Logical CPU.
	// This Function call Blocks until all Finished Event Object Handles have been set to the 'signaled state' (Available).
	if (WaitForMultipleObjects(this->maxCPUCount, this->handleFinishedEvent, true, 0) == WAIT_OBJECT_0)
	{
		// Start the Timer used to periodically update the progress indicating 
		// the percentage of files that have been processed.
		StartTimer(this->timerStartTickCount);
	}
	
	SetEvent(this->handlePendingEvent); // Sets the specified Event Object to the 'signaled state' (Available).

	SendMessage(this->hwndTaskProgressWindow, PBM_SETPOS, 0, 0); // Reset the Task Progress Window.

	EnableWindow(this->hwndButtonWindow, true);  // Enable the Stop Button.

	// Update the Window Text to indicate the percentage of files that have been processed.
	UpdateWindowTitleTextPercent(this->hwndMainApplicationWindow);

	PostMessage(this->hwndMainApplicationWindow, MSG_STATUS_BAR_STATIC_UPDATE, STATUS_BAR_PART_1, reinterpret_cast<LPARAM>(L"Status: Running"));

	wstring message(L"Message: Selected Files and Folders are now being processed...");
	const size_t messageBufferSize = message.size() + 1;
	this->PostDynamicStatusBarMessage(message.c_str(), messageBufferSize);
}

bool MainApplication::ApplicationManager::GetNextAvailableFilename(wstring& filenameRef, HANDLE handlePendingEvent)
{
	bool wasRemovedFromList = true;

	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);

	// Access the Shared Resource.
	if (this->listOfFilenames.empty())
	{
		wasRemovedFromList = false;
	}
	else
	{
		filenameRef = this->listOfFilenames.front(); // Set the filename. (Copy) (Read Shared Data)
		this->listOfFilenames.pop_front(); // Remove from filename from the List. (Modify Shared Data)
	}

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);

	if (!wasRemovedFromList)
	{
		// Sets the specified Event Object to the 'non-signaled state' (Owned).
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent
		ResetEvent(handlePendingEvent);
	}

	return wasRemovedFromList;
}

void MainApplication::ApplicationManager::SetEditWindowText(wchar_t* filenamePtr, wchar_t* errorMessagePtr)
{
	if (filenamePtr != nullptr)
	{
		// Creates the text that is displayed in the Edit Window while Files are processing. (Write to the buffer)
		swprintf_s(this->textOutputBuffer, MAX_TEXT_SIZE, TEXT("[%d/%d]  "), this->filesProcessedCount, this->totalFilesToProcess); // (Read Shared Data)

		// Add the following Text and pass/error related information to the Data Structures. 
		// This data will be used later after file processing has ended, and is used to help build the Final Report.
		this->editWindowDisplayTextOutput = this->textOutputBuffer;
		
		// Determine the prefix size to use.
		int prefixSize;
		if (wcsstr(filenamePtr, FILE_NAMESPACE_UNC_PREFIX) != NULL)
		{
			// A substring match was found.
			prefixSize = FILE_NAMESPACE_UNC_PREFIX_SIZE;
		}
		else if (wcsstr(filenamePtr, FILE_NAMESPACE_PREFIX) != NULL)
		{
			// A substring match was found.
			prefixSize = FILE_NAMESPACE_PREFIX_SIZE;
		}
		else
		{
			// A substring match was NOT found.
			// Do not adjust the prefix size.
			prefixSize = 0;
		}

		// Move the filename pointer forward the amount of the prefix size, to skip over the prefix characters.
		this->editWindowDisplayTextOutput += (filenamePtr + prefixSize);

		this->editWindowDisplayTextOutput += L"\r\n\r\nProcessing... please wait for the Final Report.";

		if (errorMessagePtr != nullptr)
		{
			wstring key = filenamePtr + prefixSize;
			wstring value = errorMessagePtr;
			
			// If the Key is found, than the existing value List on the Heap is updated.
			// If the Key is not found a Key is inserted and a Value Pointer is inserted.
			list<wstring>* valueList = this->filenameToErrorListMap[key]; 
			if (valueList == nullptr)
			{
				// Create a new List (Value) on the Heap. All Lists created on the Heap must be cleaned up.
				valueList = new list<wstring>;
				this->filenameToErrorListMap[key] = valueList; // Update List Pointer (Value) in the Map.
			}

			valueList->push_back(value);
		}
		else
		{
			wstring filenamePassed = filenamePtr + prefixSize;
			this->filePassedList.push_back(filenamePassed);
		}
	}
	else
	{
		// Create the text that is displayed in the Edit Window, AFTER File Processing has ended. (Final Report)

		this->editWindowDisplayTextOutput = L"[Final Report]\r\n---\r\n";

		// Create Text for Total Files scanned in the Elapsed Time.
		swprintf(this->textOutputBuffer, MAX_TEXT_SIZE, STR_RESULT, this->filesProcessedCount, 
			(this->filesProcessedCount == 1) ? (STR_FILE) : (STR_FILES), 
			(this->filesProcessedCount > 0) ? this->elapsedTimerCount : 0);  // Write to the buffer.

		this->editWindowDisplayTextOutput += this->textOutputBuffer;

		// Adjust the Output Format Specifiers for the swprintf() function, depending upon the OS Platform
		// to accomodate for size_t variations in size on 32-bit and 64-bit platforms.
		wchar_t* outputFormat = nullptr;

		#pragma region OUTPUT_FORMAT_ADJUSTMENT_FOR_COMPILATION_TARGET
		#if defined(_WIN64)
		// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.
		
		outputFormat = L"\r\n---\r\n%llu %s %s"; // On a 64-bit Windows OS, size_t is a 64-bit wide unsigned integer.

		#elif defined(_WIN32)
		// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.
		
		outputFormat = L"\r\n---\r\n%u %s %s";	// On a 32-bit Windows OS, size_t is a 32-bit wide unsigned integer.
		
		#endif
		#pragma endregion OUTPUT_FORMAT_ADJUSTMENT_FOR_COMPILATION_TARGET

		// Create Text for Files Errored.
		this->CreateFileErroredText(outputFormat);

		// Create Text for Files Passed.
		this->CreateFilePassedText(outputFormat);
	}

	// Output the final Text displayed in the Window.
	this->editWindowDisplayTextOutput += L"\r\n";
	SetWindowText(this->hwndEditWindow, (this->editWindowDisplayTextOutput.c_str()));
	
	// Cleanup Allocations on the Heap. These were both allocated in the 'DWORD WINAPI DecoderThreadProc(LPVOID lpParameter)' function.
	if (filenamePtr != nullptr) {
		delete[] filenamePtr;
		filenamePtr = nullptr;
	}

	if (errorMessagePtr != nullptr)
	{
		delete[] errorMessagePtr;
		errorMessagePtr = nullptr;
	}
}

void MainApplication::ApplicationManager::StartTimer(ULONGLONG& timerStartTickCountRef)
{
	timerStartTickCountRef = GetTickCount64();
}

void MainApplication::ApplicationManager::StopTimer(const ULONGLONG& timerStartTickCountRef)
{
	ULONGLONG currentTimerTickCount = GetTickCount64();
	const ULONGLONG MAX_TICK_COUNT = 0xFFFFFFFFFFFFFFFF;

	// NOTE: The Timer will wrap around after approximately 18,446,744,073,709,551,615 milliseconds. 
	// This is equivalent to approximately 584,942,417,355 years, which is much longer than the expected lifespan of any computer system.
	// Technically, the check for the Timer wrapping, when using the GetTickCount64() is unnecessary for practical purposes, but was interesting to implement anyway.

	// Check for Timer Wrapping.
	if (currentTimerTickCount < timerStartTickCountRef)
	{
		// The Timer has wrapped around (i.e. it has reached the maximum value and started over from zero), and the elapsed time calculation needs to take this into account.

		ULONGLONG numberOfTicksElapsedSinceTimerWrap = MAX_TICK_COUNT - timerStartTickCountRef;
		ULONGLONG totalNumberOfTicksElapsedSinceTimerStarted = numberOfTicksElapsedSinceTimerWrap + currentTimerTickCount;
		this->elapsedTimerCount = (float)totalNumberOfTicksElapsedSinceTimerStarted;
	}
	else
	{
		// The Timer has not wrapped around.
		this->elapsedTimerCount = (float)(currentTimerTickCount - timerStartTickCountRef);
	}

	// The elapsed time is divided by 1000 to convert it from milliseconds to seconds.
	this->elapsedTimerCount /= 1000;
}

void MainApplication::ApplicationManager::UpdateWindowTitleTextPercent(HWND hwndMainApplicationWindow) const
{
	int percentage = 0;
	if (this->totalFilesToProcess != 0) // (Read Shared Data)
	{
		// Calculate the percentage of Files processed. 
		// Multiply 1st operand by 100, BEFORE dividing by the 2nd operand.
		// This is because Integer division is being used for the calculation. Otherwise, no percentage would be 
		// displayed due to the intermediate value being less than zero, and therefore getting truncated and returned as Zero.
		percentage = (this->filesProcessedCount * 100) / this->totalFilesToProcess;
	}
	
	// Create the formatted Text.
	wchar_t buffer[64]; // Create the buffer the formatted text is written to.
	swprintf(buffer, 64, TEXT("%s - %d%%"), WINDOW_TITLE, percentage); // Write to the buffer.

	// Set the text of the Main Application Window to the formatted text.
	SetWindowText(hwndMainApplicationWindow, buffer);
}

void MainApplication::ApplicationManager::UpdateTaskProgressWindowPercent() const
{
	int percentage = 0;
	if (this->totalFilesToProcess != 0) // (Read Shared Data)
	{
		// Calculate the percentage of Files processed. 
		// Multiply 1st operand by 100, BEFORE dividing by the 2nd operand.
		// This is because Integer division is being used for the calculation. Otherwise, no percentage would be 
		// displayed due to the intermediate value being less than zero, and therefore getting truncated and returned as Zero.
		percentage = (this->filesProcessedCount * 100) / this->totalFilesToProcess;
	}

	// Set percentage in the Task Progress Window.

	// Message Result is the current position of the Progress Window.
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-getpos
	LRESULT messageResult = SendMessage(this->hwndTaskProgressWindow, PBM_GETPOS, 0, 0);

	if (percentage != messageResult)
	{
		// Sets the current position for the Progress Window and redraws the Progress Bar to reflect the new position.
		// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setpos
		SendMessage(this->hwndTaskProgressWindow, PBM_SETPOS, percentage, 0);

		// Advances the current position for a Progress Window by the step increment and redraws the Progress Bar to reflect the new position.
		// https://learn.microsoft.com/en-us/windows/win32/controls/pbm-stepit
		// SendMessage(this->hwndTaskProgressWindowRef, PBM_STEPIT, 0, 0);
	}
}

wchar_t* MainApplication::ApplicationManager::GetSecondCommandLineArgument()
{
	wchar_t* bufferPtr = nullptr;
	int argcW;

	// Get the Command Line String for the current Process.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getcommandlinew
	LPWSTR commandLineString = GetCommandLine();

	// Parse the Arguments from the Command Line String into an Array.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw
	LPWSTR* argvW = CommandLineToArgvW(commandLineString, &argcW);

	// Array is not empty.
	if (argvW != NULL)
	{
		// Array contains more than 1 argument.
		if (argcW > 1)
		{
			// Allocates a new buffer of wchar_t type with a size equal to the length of the second argument plus one.
			const size_t bufferSize = wcslen(argvW[1]) + 1;
			bufferPtr = new wchar_t[bufferSize];

			// Copy the second argument to the newly allocated buffer.
			wcscpy_s(bufferPtr, bufferSize, argvW[1]);
		}

		// Frees the specified local memory object and invalidates its handle.
		// URI: https://learn.microsoft.com/en-us/windows/win32/memory/global-and-local-functions
		// URI: https://learn.microsoft.com/en-us/windows/win32/memory/heap-functions
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-localfree
		LocalFree(argvW);
	}

	return bufferPtr;
}

std::unique_ptr<PureAbstractBaseDecoder> MainApplication::ApplicationManager::OpenDecoder(const wchar_t* filenamePtr)
{
	// Delegate to the Decoder Manager.
	return this->decoderManagerPtr->OpenDecoderSmartPointer(filenamePtr);
}

void MainApplication::ApplicationManager::IncrementFilesProcessed()
{
	this->filesProcessedCount++;
}

void MainApplication::ApplicationManager::SetStaticStatusBarText(int statusBarPartID, wchar_t* newStatusBarTextPtr)
{
	// Enforce the rules for which Status Bar Parts can use Statically Allocated read-only Memory.
	if (statusBarPartID == STATUS_BAR_PART_1)
	{
		SendMessage(this->hwndStatusBarWindow, SB_SETTEXT, statusBarPartID, reinterpret_cast<LPARAM>(newStatusBarTextPtr));
	}
}

void MainApplication::ApplicationManager::SetDynamicStatusBarText(int statusBarPartID, wchar_t* newStatusBarTextPtr)
{
	// Enforce the rules for which Status Bar Parts can use Dynamically Allocated Memory.
	if (NUMBER_OF_STATUS_BAR_PARTS >= 2 && statusBarPartID == STATUS_BAR_PART_2)
	{
		unique_ptr<wchar_t> currentStatusBarTextSmartPtr = this->GetStatusBarText(hwndStatusBarWindow, statusBarPartID);
		if (wcscmp(currentStatusBarTextSmartPtr.get(), newStatusBarTextPtr) != 0)
		{
			// The new String does not match the currently managed string. Update the Status Bar Text.

			// The Pointers must point to different memory locations.
			assert(this->statusBarTextPtr != newStatusBarTextPtr);

			this->CleanupDynamicStatusBarText();
			this->statusBarTextPtr = newStatusBarTextPtr; // Update the managed pointer value.

			// Set the new Status Bar Text.
			SendMessage(this->hwndStatusBarWindow, SB_SETTEXT, statusBarPartID, reinterpret_cast<LPARAM>(this->statusBarTextPtr));
		}
		else
		{
			// The Status Bar already has the same Text Message. No need to update the Status Bar Text.
			if (newStatusBarTextPtr != nullptr)
			{
				delete[] newStatusBarTextPtr;
				newStatusBarTextPtr = nullptr;
			}
		}
	}
}

void MainApplication::ApplicationManager::PostDynamicStatusBarMessage(const wchar_t* statusBarTextPtr, size_t bufferSize)
{
	wchar_t* statusBarTextMessagePtr = new wchar_t[bufferSize];
	wcscpy_s(statusBarTextMessagePtr, bufferSize, statusBarTextPtr); // Copy to the Heap.
	PostMessage(this->hwndMainApplicationWindow, MSG_STATUS_BAR_DYNAMIC_UPDATE, STATUS_BAR_PART_2, reinterpret_cast<LPARAM>(statusBarTextMessagePtr));
}

std::unique_ptr<wchar_t> MainApplication::ApplicationManager::GetStatusBarText(HWND hwndStatusBarWindow, int statusBarPartID)
{
	// Get the length of the status bar text.
	LRESULT lresultStatusBarTextLength = SendMessage(hwndStatusBarWindow, SB_GETTEXTLENGTH, statusBarPartID, 0);
	
	#pragma region LRESULT_ADJUSTMENT_FOR_COMPILATION_TARGET
	#if defined(_WIN64)
	// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

	long long statusBarTextLength = static_cast<long long>(lresultStatusBarTextLength);
	long long bufferSize = statusBarTextLength + 1LL;

	#elif defined(_WIN32)
	// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.
	
	long statusBarTextLength = static_cast<long>(lresultStatusBarTextLength);
	long bufferSize = statusBarTextLength + 1L;

	#endif
	#pragma endregion LRESULT_ADJUSTMENT_FOR_COMPILATION_TARGET
	
	// Create a buffer that will be written to by the Status Bar Window.
	wchar_t* statusBarTextBufferPtr = new wchar_t[bufferSize] {};
	SendMessage(hwndStatusBarWindow, SB_GETTEXT, statusBarPartID, reinterpret_cast<LPARAM>(statusBarTextBufferPtr));
	
	unique_ptr<wchar_t> statusBarTextSmartPtr(statusBarTextBufferPtr); // Convert Raw Pointer to a Smart Pointer.
	return statusBarTextSmartPtr;
}

void MainApplication::ApplicationManager::ClearListOfFilenames()
{
	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);
	
	// Access the Shared Resource.
	this->listOfFilenames.clear(); // (Modify Shared Data)

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);
}

DWORD MainApplication::ApplicationManager::GetMaxCPUCount() const
{
	return this->maxCPUCount;
}

void MainApplication::ApplicationManager::GenerateFinalReport()
{
	// Generate the Final Report.
	this->StopTimer(this->timerStartTickCount); // Compute the Elapsed Time, used in the Final Report.
	this->SetEditWindowText(nullptr, nullptr); // Generate the Final Report.
	this->ResetEditWindowSuppport(); // Reset the Edit Window supporting Data Structures.
	
	PostMessage(this->hwndMainApplicationWindow, MSG_STATUS_BAR_STATIC_UPDATE, STATUS_BAR_PART_1, reinterpret_cast<LPARAM>(L"Status: Finished Processing"));

	wstring message(L"Message: Selected Files and Folders have been processed.");
	const size_t messageBufferSize = message.size() + 1;
	this->PostDynamicStatusBarMessage(message.c_str(), messageBufferSize);
}

void MainApplication::ApplicationManager::EnableStoppingState()
{
	this->SetStoppingStateFlagEnabled(true);
	
	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);

	if (this->executionManagerPtr->IsRunningStateFlagEnabled()) // (Read Shared Data)
	{
		// Access the Shared Resource(s).
		this->executionManagerPtr->SetStoppingStateFlagEnabled(true); // (Modify Shared Data)
	}

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);
}

bool MainApplication::ApplicationManager::IsStoppingState()
{
	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);

	// (Read Shared Data)
	bool isStoppingState = this->IsStoppingStateFlagEnabled() || this->executionManagerPtr->IsStoppingStateFlagEnabled();

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);

	return isStoppingState;
}

bool MainApplication::ApplicationManager::IsRunningState()
{
	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);
	
	// (Read Shared Data)
	bool isBackgroundTaskRunning = !this->IsStoppingState() && this->executionManagerPtr->IsRunningStateFlagEnabled();

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);

	return isBackgroundTaskRunning;
}

bool MainApplication::ApplicationManager::IsStoppingStateFlagEnabled() const
{
	return this->stoppingStateFlagEnabled;
}

void MainApplication::ApplicationManager::SetStoppingStateFlagEnabled(bool value)
{
	this->stoppingStateFlagEnabled = value;
}

HINSTANCE MainApplication::ApplicationManager::GetInstance() const
{
	return this->hInstance;
}

void MainApplication::ApplicationManager::SetInstance(HINSTANCE hInstance)
{
	this->hInstance = hInstance;
}

HWND MainApplication::ApplicationManager::GetMainApplicationWindow() const
{
	return this->hwndMainApplicationWindow;
}

void MainApplication::ApplicationManager::SetMainApplicationWindow(HWND hwndMainApplicationWindow)
{
	this->hwndMainApplicationWindow = hwndMainApplicationWindow;
}

HWND MainApplication::ApplicationManager::GetEditWindow() const
{
	return this->hwndEditWindow;
}

void MainApplication::ApplicationManager::SetEditWindow(HWND hwndEditWindow)
{
	this->hwndEditWindow = hwndEditWindow;
}

HWND MainApplication::ApplicationManager::GetButtonWindow() const
{
	return this->hwndButtonWindow;
}

void MainApplication::ApplicationManager::SetButtonWindow(HWND hwndButtonWindow)
{
	this->hwndButtonWindow = hwndButtonWindow;
}

HWND MainApplication::ApplicationManager::GetTaskProgressWindow() const
{
	return this->hwndTaskProgressWindow;
}

void MainApplication::ApplicationManager::SetTaskProgressWindow(HWND hwndTaskProgressWindow)
{
	this->hwndTaskProgressWindow = hwndTaskProgressWindow;
}

HWND MainApplication::ApplicationManager::GetProgressWindow(unsigned long index) const
{
	return this->hwndProgressWindow[index];
}

void MainApplication::ApplicationManager::SetProgressWindow(HWND hwndProgressWindow, unsigned long index)
{
	this->hwndProgressWindow[index] = hwndProgressWindow;
}

WNDPROC MainApplication::ApplicationManager::GetEditProcedure() const
{
	return this->wndprocEditProcedure;
}

void MainApplication::ApplicationManager::SetEditProcedure(WNDPROC wndprocEditProcedure)
{
	this->wndprocEditProcedure = wndprocEditProcedure;
}

HWND MainApplication::ApplicationManager::GetStatusBarWindow() const
{
	return this->hwndStatusBarWindow;
}

void MainApplication::ApplicationManager::SetStatusBarWindow(HWND hwndStatusBarWindow)
{
	this->hwndStatusBarWindow = hwndStatusBarWindow;
}

HANDLE MainApplication::ApplicationManager::GetHandleThread(unsigned long index) const
{
	return this->handleThread[index];
}

void MainApplication::ApplicationManager::SetHandleThread(HANDLE handleThread, unsigned long index)
{
	this->handleThread[index] = handleThread;
}

HANDLE MainApplication::ApplicationManager::GetTerminateEvent() const
{
	return this->handleTerminateEvent;
}

void MainApplication::ApplicationManager::SetTerminateEvent(HANDLE handleTerminateEvent)
{
	this->handleTerminateEvent = handleTerminateEvent;
}

HANDLE MainApplication::ApplicationManager::GetPendingEvent() const
{
	return this->handlePendingEvent;
}

void MainApplication::ApplicationManager::SetPendingEvent(HANDLE handlePendingEvent)
{
	this->handlePendingEvent = handlePendingEvent;
}

const HANDLE* MainApplication::ApplicationManager::GetFinishedEventArray() const
{
	return this->handleFinishedEvent;
}

HANDLE MainApplication::ApplicationManager::GetFinishedEvent(unsigned long index) const
{
	return this->handleFinishedEvent[index];
}

void MainApplication::ApplicationManager::SetFinishedEvent(HANDLE handleFinishedEvent, unsigned long index)
{
	this->handleFinishedEvent[index] = handleFinishedEvent;
}

ULONGLONG MainApplication::ApplicationManager::GetTimerStartTickCount() const
{
	return this->timerStartTickCount;
}

void MainApplication::ApplicationManager::SetTimerStartTickCount(ULONGLONG timerStartTickCount)
{
	this->timerStartTickCount = timerStartTickCount;
}

bool MainApplication::ApplicationManager::StopButtonPressedFlagEnabled() const
{
	return this->stopButtonPressedFlagEnabled;
}

void MainApplication::ApplicationManager::SetStopButtonPressedFlag(bool value)
{
	this->stopButtonPressedFlagEnabled = value;
}

#pragma endregion Public_Functions_Region

#pragma region Private_Member_Functions_Region

void MainApplication::ApplicationManager::ApplicationManagerStartup()
{
	// Create the Decoder Manager on the Heap.
	this->decoderManagerPtr = new DecoderManager();

	// Create the Execution Manager on the Heap.
	this->executionManagerPtr = new ExecutionManager(this->criticalSectionPtr);
	
	// Bind the Class Member function to the instance, and then add the bound Callback Function to the Execution Manager.
	auto boundCallback = std::bind(&ApplicationManager::ScanFolderAsync, this, std::placeholders::_1);
	this->executionManagerPtr->SetBoundCallback(boundCallback);

	// Create and configure the System Info Struct.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
	SYSTEM_INFO systemInfoStruct{};
	LPSYSTEM_INFO systemInfoStructPtr = &systemInfoStruct;

	// URI: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsysteminfo
	GetSystemInfo(systemInfoStructPtr);

	// Get the number of Logical Processors.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
	this->maxCPUCount = (systemInfoStruct.dwNumberOfProcessors > MAX_CPU) ? MAX_CPU : systemInfoStruct.dwNumberOfProcessors;

	// Code to use a Single CPU with a Single Thread Scenario.
	if (FORCE_SINGLE_CPU_ENABLED)
	{
		this->maxCPUCount = 1UL;
	}

	// Configure the Decoder Memory Buffers.
	if (this->maxCPUCount > 1UL)
	{
		// Opened Decoders will use Memory Buffers.
		this->decoderManagerPtr->SetDecoderMemoryBufferEnabled(true);
	}
	else
	{
		// Opened Decoders will NOT use Memory Buffers.
		this->decoderManagerPtr->SetDecoderMemoryBufferEnabled(false);
	}
}

void MainApplication::ApplicationManager::CreateSynchronizationSupport(HANDLE& handleTerminateEventRef, HANDLE& handlePendingEventRef)
{
	// Create Event Synchronization Objects. 
	// Events Objects are Synchronization Objects that may be used to notify waiting Threads when a desired condition occurs.
	// Using Event Objects, no Thread spends unnecessary Processor time in a Polling Loop looking for work to do.
	// 
	// Event Objects can have one of two states: 'signaled' or 'not-signaled'.
	// When the Event Object is in the 'signaled' state, the Wait Function returns (e.g. WaitForSingleObject() OR WaitForMultipleObjects()), the Thread may continue executing.
	// When the Event Object is in the 'not-signaled' state, the Thread blocks until the event is signaled.
	// 
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw

	// Manual-reset Event Object, which requires the use of the ResetEvent() function to set the event state to 'non-signaled'.
	// Using a Manual-reset Event Object, the SetEvent() function must be called explicitly to set the object to 'signaled' state 
	// AND the ResetEvent() function must be called to reset to the' not-signaled' state. 
	// Between setting and resetting a Manual-reset Event Object, ALL Threads waiting for it are unblocked.
	BOOL bManualReset = true;

	// The initial state of the Event Object (set to 'non-signaled') (Owned). 
	BOOL bInitialState = false;
	
	handleTerminateEventRef = CreateEvent(NULL, bManualReset, bInitialState, NULL);
	handlePendingEventRef = CreateEvent(NULL, bManualReset, bInitialState, NULL);
}

void MainApplication::ApplicationManager::CreateThreads(HWND hwnd, HANDLE handleFinishedEvent[], HANDLE handleThread[]) const
{	
	// Create a Thread for each Logical CPU.
	for (unsigned long threadIndex = 0; threadIndex < this->maxCPUCount; ++threadIndex)
	{
		BOOL bManualReset = true; // Manual-reset Event Object, which requires the use of the ResetEvent() function to set the event state to 'non-signaled'.
		BOOL bInitialState = true; // The initial state of the Event Object (set to 'signaled') (Available). 
		HANDLE finishedEvent = CreateEvent(NULL, bManualReset, bInitialState, NULL);
		handleFinishedEvent[threadIndex] = finishedEvent;

		// Try to create a new Thread.
		// This will create a Thread to execute within the Virtual Address Space of the calling Process.
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread

		// Convert the 'unsigned long' value to an 'unsigned integer pointer type' which is capable of holding the current OS platform's full pointer data width (32-bit/64-bit). 
		// This intermediate conversion allows for safely converting to a valid Generic Pointer Type (void*) (e.g. LPVOID).
		// 
		// (NOTE: On a 32-bit Windows OS uintptr_t is 32-bits wide; On a 64-bit Windows OS uintptr_t is 64-bits wide)
		uintptr_t ptrDataWidthType = static_cast<uintptr_t>(threadIndex);

		// Convert from the 'unsigned integer pointer type' to the LPVPOID Generic Pointer Type (void*).
		// (NOTE: On 32-bit Windows OS LPVPOID is 32-bits wide; On a 64-bit Windows OS LPVPOID is 64-bits wide)
		LPVOID lpParameter = reinterpret_cast<LPVOID>(ptrDataWidthType);
		
		LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL; // Use the default security descriptor.
		SIZE_T threadStackSize = 0; // Use the Default Stack Size.
		DWORD dwCreationFlags = 0; // Set the Thread to run immediately after creation.
		DWORD lpThreadId; // Thread Identifier is returned.
		handleThread[threadIndex] = CreateThread(lpThreadAttributes, threadStackSize, DecoderThreadProc, lpParameter, dwCreationFlags, &lpThreadId);

		if (handleThread[threadIndex] != NULL)
		{
			// Thread creation was successful. Configure the Thread.

			// URI: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority
			SetThreadPriority(handleThread[threadIndex], THREAD_PRIORITY_BELOW_NORMAL);

			if (this->maxCPUCount > 1UL)
			{
				// URI: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadaffinitymask
				SetThreadAffinityMask(handleThread[threadIndex], static_cast<DWORD_PTR>(1UL) << threadIndex);
			}
		}
		else {
			// Thread creation failed.

			// Get the last Error Code, which was set by the call to the CreateThread() function.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
			// URI: https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes#system-error-codes
			DWORD dwErrorCode = GetLastError();

			// Construct an Error Message the will be displayed to the User.
			LPWSTR error_message = nullptr;
			DWORD dwResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, dwErrorCode, LANG_SYSTEM_DEFAULT, (LPWSTR)&error_message, 0, nullptr);
			
			// Prompt the User with an Error Message Box.
			MessageBox(hwnd, error_message, WINDOW_TITLE, MB_ICONERROR | MB_OK);
			
			// NOTE: The ExitProcess() function is the preferred method of ending a process. This function provides a clean Process shutdown. 
			// This includes calling the entry-point function of all attached Dynamic-link libraries (DLLs) with DLL_PROCESS_DETACH, indicating that the Process is detaching from the DLL. 
			// After all attached DLLs have executed any process termination code, the ExitProcess() function terminates the current Process, including the calling Thread.
			// In contrast, if a Process terminates by calling the TerminateProcess() function, the DLLs that the Process is attached to are not notified of the Process termination.

			// Terminate the Program.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess
			ExitProcess(0);
		}
	}
}

void MainApplication::ApplicationManager::StopThreads()
{
	// Sets the specified event object to the signaled state. (synchapi.h)
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
	SetEvent(this->handleTerminateEvent); // Sets Terminate Event Object to the signaled state for all Threads. This initiates Thread shutdown.

	// Waits until ALL of the specified objects are in the signaled state or the time-out interval elapses. (synchapi.h)
	// Since the argument 'bWaitAll = true', the function returns when the state of ALL objects in the 'lpHandles' array are signaled (Available). 
	WaitForMultipleObjects(this->maxCPUCount, this->handleThread, true, INFINITE);

	// Close all of the Thread Handles.
	for (unsigned long threadIndex = 0; threadIndex < this->maxCPUCount; ++threadIndex)
	{
		CloseHandle(this->handleThread[threadIndex]);
		this->handleThread[threadIndex] = nullptr;
	}
}

void MainApplication::ApplicationManager::ScanFolderAsync(const wchar_t* folderPtr)
{
	wstring messageStart(L"Message: Searching for supported Files. Please wait...");
	const size_t messageStartBufferSize = messageStart.size() + 1;
	this->PostDynamicStatusBarMessage(messageStart.c_str(), messageStartBufferSize);

	// Show the Progress Bar Window.
	ShowWindow(this->hwndStatusBarProgressWindow, SW_SHOW);
	
	SendMessage(this->hwndStatusBarProgressWindow, PBM_SETMARQUEE, TRUE, 0);
	
	// Scan the Folder. (A possibly long-running operation).
	std::function<void(const wchar_t*)> boundCallback = std::bind(&ExecutionManager::AddToProcessedFileList, this->executionManagerPtr, std::placeholders::_1);
	this->ScanFolderWithCallback(boundCallback, folderPtr);

	// Stop the Marquee Animation.
	SendMessage(this->hwndStatusBarProgressWindow, PBM_SETMARQUEE, FALSE, 0);
	
	// Switch to Determinate Mode (remove PBS_MARQUEE style).
	LONG_PTR style_flags = GetWindowLongPtr(this->hwndStatusBarProgressWindow, GWL_STYLE);
	SetWindowLongPtr(this->hwndStatusBarProgressWindow, GWL_STYLE, style_flags & ~PBS_MARQUEE);

	// Clear the Progress Bar (Set the position to zero).
	SendMessage(this->hwndStatusBarProgressWindow, PBM_SETPOS, 0, 0);

	// Switch the Progress Bar back to Marquee Mode.
	SetWindowLongPtr(this->hwndStatusBarProgressWindow, GWL_STYLE, style_flags | PBS_MARQUEE);

	// Hide the Progress Bar Window.
	ShowWindow(this->hwndStatusBarProgressWindow, SW_HIDE);

	wstring messageFinished(L"Message: Finished searching for supported Files.");
	const size_t messageFinishedBufferSize = messageFinished.size() + 1;
	this->PostDynamicStatusBarMessage(messageFinished.c_str(), messageFinishedBufferSize);

	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);

	// Access the Shared Resource(s).

	// (Read Shared Data)
	bool isStoppingStateFlagEnabled = this->executionManagerPtr->IsStoppingStateFlagEnabled();

	// Before initiating further Thread-based Processing, check if the User manually stopped processing while the background Asynchronous Thread was busy processing.
	if (!isStoppingStateFlagEnabled && this->executionManagerPtr->IsFolderQueueEmpty())
	{
		// Get the size of Processed File List, before performing the Splice() operation.
		int processedFileListSize = static_cast<int>(this->executionManagerPtr->GetProcessedFileListRef().size());

		// This is an optimization that significantly reduces the number of function calls into the Critical Section, by the Execution Manager.
		// Splice Operation Time Complexity: O(1) Constant Time.
		// URI: https://cplusplus.com/reference/list/list/splice/
		this->listOfFilenames.splice(std::end(this->listOfFilenames), this->executionManagerPtr->GetProcessedFileListRef()); // (Modify Shared Data)

		// After splicing the Lists together, the Execution Manager List of processed File names should be empty.
		assert(this->executionManagerPtr->GetProcessedFileListRef().empty());
		
		this->totalFilesToProcess += processedFileListSize;  // (Modify Shared Data)

		// Process the selected Files via Thread-based Processing, only after all of Folders in the Queue have been processed.
		this->SetPendingEvent();
	}
	else if (isStoppingStateFlagEnabled)
	{
		// The User manually stopped processing while the Asynchronous Thread Callback Function was processing.  
		// Therefore, the Execution Manager and results from the last Callback Function run, must be cleaned up.

		// Discard any remaining list of filenames. 
		this->ClearListOfFilenames();
		
		this->totalFilesToProcess = 0; // (Modify Shared Data)
		
		// Clear the Execution Manager Folder processing Queue.
		this->executionManagerPtr->ClearFolderQueue(); // (Modify Shared Data)

		// Clear the List of processed File names.
		this->executionManagerPtr->ClearProcessedFileList();
		
		// Reset the Stopping State for the Application Manager. 
		this->SetStoppingStateFlagEnabled(false); // (Modify Shared Data)

		// Reset the Stopping State Flag for Execution Manager.
		this->executionManagerPtr->SetStoppingStateFlagEnabled(false); // (Modify Shared Data)
	}

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);
}

void MainApplication::ApplicationManager::ScanFolderWithCallback(std::function<void(const wchar_t*)> boundCallback, const wchar_t* folderPtr)
{
	this->rScanFolderWithCallback(boundCallback, folderPtr);
}

void MainApplication::ApplicationManager::rScanFolderWithCallback(const std::function<void(const wchar_t*)>& boundCallbackRef, const wchar_t* folderPtr)
{
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-win32_find_dataa
	WIN32_FIND_DATA findDataStructure{};
	ZeroMemory(&findDataStructure, sizeof(findDataStructure));

	wstring folderPath(folderPtr);

	// Ensure that the Folder path ends with a backslash.
	if (folderPath.back() != L'\\')
	{
		folderPath += L'\\'; // Append a single backslash to the Folder path.
	}

	wstring pathName = folderPath + TEXT("*"); // Append the search wildcard just after the last single backslash (e.g. \* ).
	HANDLE handleFirstFile = FindFirstFile(pathName.c_str(), &findDataStructure); // Start the File Search. URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew

	if (handleFirstFile != INVALID_HANDLE_VALUE)
	{
		BOOL fileWasFound = true;
		while (fileWasFound)
		{
			// Check for the Stopping State.
			if (this->IsStoppingStateFlagEnabled())
			{
				break; // Break from Loop to the Base Case.
			}

			// Update the Path Name.
			pathName = folderPath + findDataStructure.cFileName;

			// Check whether the current Path Name is a Folder or File.
			if (WinAPIUtils::IsFolderType(pathName.c_str()))
			{
				// Check that the current Folder is NOT a Hidden Folder (e.g. Hidden folder names in Windows start with a period).
				if (findDataStructure.cFileName[0] != '.')
				{
					this->rScanFolderWithCallback(boundCallbackRef, pathName.c_str()); // Recursive Call. (Problem Reduction Step)
				}
			}
			else
			{
				// A File was found in the current Directory. (Base Case)
				if (this->decoderManagerPtr->IsSupportedType(pathName.c_str()))
				{
					// Use the Callback Function to perform an operation on the Supported Type (e.g. Adding a Supported Type to a List).
					boundCallbackRef(pathName.c_str());
				}
			}

			// Continue the File Search.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilew
			fileWasFound = FindNextFile(handleFirstFile, &findDataStructure);
		}

		// No more Files or Folders were found in the current Directory. (Base Case)
		// Closes the file search handle opened by the FindFirstFile() function. URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findclose
		FindClose(handleFirstFile);
	}

	// No Files were found in the current Directory. (Base Case)
}

void MainApplication::ApplicationManager::AddToFileList(const wchar_t* filenamePtr)
{
	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);
	
	// Access the Shared Resource(s).

	// Copies the C-style String to a new wstring object in the List.
	this->listOfFilenames.push_back(filenamePtr); // (Modify Shared Data)
	this->totalFilesToProcess++;  // (Modify Shared Data)
	
	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);
}

void MainApplication::ApplicationManager::ResizeControlsStatusBarEnabled(HWND hwnd)
{
	// Set the Window Positions.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos

	// Get the coordinates of a window's client area. The client coordinates specify the upper-left and lower-right corners of the client area.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/windef/ns-windef-rect
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect

	// In the RECT Struct, the left and top members are zero. The right and bottom members contain the width and height of the window.
	RECT rectStruct;
	GetClientRect(hwnd, &rectStruct);
	int rectWidth = rectStruct.right - rectStruct.left; // (x-coordinate of the lower-right corner of the Rectangle - x-coordinate of the upper-left corner of the Rectangle)
	int rectHeight = rectStruct.bottom - rectStruct.top; // (y-coordinate of the lower-right corner of the Rectangle - y-coordinate of the upper-left corner of the Rectangle)

	// Set the Edit Window position (Using Pixels).
	int cxEditWindowPixelWidth = rectWidth - (WINDOW_PADDING * 2); // Pixels
	int cyEditWindowPixelHeight = (rectHeight - (WINDOW_PADDING * 3)) - (3 * BUTTON_HEIGHT + (1 * WINDOW_PADDING)); // Pixels
	SetWindowPos(this->hwndEditWindow, HWND_TOP, 0, 0, cxEditWindowPixelWidth, cyEditWindowPixelHeight, SWP_NOMOVE | SWP_NOZORDER);

	// Set the Task Progress Window position (Using Client Coordinates AND Pixels).
	int taskProgressWindowPaddingWidth = (WINDOW_PADDING * 2);
	int rectWidthTaskProgressWindow = rectWidth - taskProgressWindowPaddingWidth;
	int leftSidePositionTaskProgressWindow = WINDOW_PADDING; // Client Coordinate
	int topPositionTaskProgressWindow = (rectHeight - WINDOW_PADDING) - (3 * BUTTON_HEIGHT + (1 * WINDOW_PADDING)); // Client Coordinate
	int cxTaskProgressWindowPixelWidth = rectWidthTaskProgressWindow; // Pixels
	int cyTaskProgressWindowPixelHeight = BUTTON_HEIGHT; // Pixels
	SetWindowPos(this->hwndTaskProgressWindow, HWND_TOP, leftSidePositionTaskProgressWindow, topPositionTaskProgressWindow, cxTaskProgressWindowPixelWidth, cyTaskProgressWindowPixelHeight, SWP_NOZORDER);

	// Set the Button Window position (Using Client Coordinates).
	int leftSidePositionButtonWindow = (rectWidth - WINDOW_PADDING) - BUTTON_WIDTH;  // Client Coordinate
	int topPositionButtonWindow = (rectHeight - WINDOW_PADDING) - (2 * BUTTON_HEIGHT); // Client Coordinate
	SetWindowPos(this->hwndButtonWindow, HWND_TOP, leftSidePositionButtonWindow, topPositionButtonWindow, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	// Set each Progress Window position (Using Client Coordinates AND Pixels).
	int rectWidthProgressWindow = ((rectWidth - (WINDOW_PADDING * (2 + this->maxCPUCount))) - BUTTON_WIDTH) / this->maxCPUCount;
	for (unsigned long index = 0; index < this->maxCPUCount; ++index)
	{
		int leftSidePositionProgressWindow = (WINDOW_PADDING * (index + 1)) + (index * rectWidthProgressWindow); // Client Coordinate
		int topPositionProgressWindow = (rectHeight - WINDOW_PADDING) - (2 * BUTTON_HEIGHT); // Client Coordinate
		int cxProgressWindowPixelWidth = rectWidthProgressWindow; // Pixels
		int cyProgressWindowPixelHeight = BUTTON_HEIGHT; // Pixels
		SetWindowPos(this->hwndProgressWindow[index], HWND_TOP, leftSidePositionProgressWindow, topPositionProgressWindow, cxProgressWindowPixelWidth, cyProgressWindowPixelHeight, SWP_NOZORDER);
	}

	// Set the Status Bar Window position (Using Client Coordinates AND Pixels).
	int leftSidePositionStatusBarWindow = 0; // Client Coordinate
	int topPositionStatusBarWindow = 0; // Client Coordinate
	int cxStatusBarWindowPixelWidth = 0; // Pixels
	int cyStatusBarWindowPixelHeight = 0; // Pixels
	SetWindowPos(this->hwndStatusBarWindow, HWND_TOP, leftSidePositionStatusBarWindow, topPositionStatusBarWindow, cxStatusBarWindowPixelWidth, cyStatusBarWindowPixelHeight, SWP_NOZORDER);
	
	// Resize the Status Bar Parts of the Status Bar Dynamically.
	this->SetStatusBarWindowPartsSize(hwnd, this->hwndStatusBarWindow);
	
	RECT rectStructStatusBarPart{};
	SendMessage(this->hwndStatusBarWindow, SB_GETRECT, STATUS_BAR_PART_3, reinterpret_cast<LPARAM>(&rectStructStatusBarPart));
	int rectWidthStatusBarPart = rectStructStatusBarPart.right - rectStructStatusBarPart.left; // (x-coordinate of the lower-right corner of the Rectangle - x-coordinate of the upper-left corner of the Rectangle)
	int rectHeightStatusBarPart = rectStructStatusBarPart.bottom - rectStructStatusBarPart.top; // (y-coordinate of the lower-right corner of the Rectangle - y-coordinate of the upper-left corner of the Rectangle)
	
	// Set the Status Bar Progress Window position (Using Client Coordinates AND Pixels).
	int leftSidePositionStatusBarProgressWindow = rectStructStatusBarPart.left + (2 * WINDOW_PADDING); // Client Coordinate
	int topPositionStatusBarProgressWindow = rectStructStatusBarPart.top + (rectHeightStatusBarPart / 4); // Client Coordinate
	int cxStatusBarProgressWindowPixelWidth = rectWidthStatusBarPart - (4 * WINDOW_PADDING); // Pixels
	int cyStatusBarProgressWindowPixelHeight = rectHeightStatusBarPart / 2; // Pixels
	SetWindowPos(this->hwndStatusBarProgressWindow, HWND_TOP, leftSidePositionStatusBarProgressWindow, topPositionStatusBarProgressWindow, cxStatusBarProgressWindowPixelWidth, cyStatusBarProgressWindowPixelHeight, SWP_NOZORDER);
}

void MainApplication::ApplicationManager::ResizeControlsStatusBarDisabled(HWND hwnd)
{
	// Set the Window Positions.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos

	// Get the coordinates of a window's client area. The client coordinates specify the upper-left and lower-right corners of the client area.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/windef/ns-windef-rect
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect

	// In the RECT Struct, the left and top members are zero. The right and bottom members contain the width and height of the window.
	RECT rectStruct;
	GetClientRect(hwnd, &rectStruct);
	int rectWidth = rectStruct.right - rectStruct.left; // (x-coordinate of the lower-right corner of the Rectangle - x-coordinate of the upper-left corner of the Rectangle)
	int rectHeight = rectStruct.bottom - rectStruct.top; // (y-coordinate of the lower-right corner of the Rectangle - y-coordinate of the upper-left corner of the Rectangle)

	// Resize Controls (Without Status Bar)!
	// Set the Edit Window position (Using Pixels).
	int cxEditWindowPixelWidth = rectWidth - (WINDOW_PADDING * 2); // Pixels
	int cyEditWindowPixelHeight = (rectHeight - (WINDOW_PADDING * 3)) - (2 * BUTTON_HEIGHT + (1 * WINDOW_PADDING)); // Pixels
	SetWindowPos(hwndEditWindow, HWND_TOP, 0, 0, cxEditWindowPixelWidth, cyEditWindowPixelHeight, SWP_NOMOVE | SWP_NOZORDER);
	
	// Set the Task Progress Window position (Using Client Coordinates AND Pixels).
	int taskProgressWindowPaddingWidth = (WINDOW_PADDING * 2);
	int rectWidthTaskProgressWindow = rectWidth - taskProgressWindowPaddingWidth;
	int leftSidePositionTaskProgressWindow = WINDOW_PADDING; // Client Coordinate
	int topPositionTaskProgressWindow = (rectHeight - WINDOW_PADDING) - (2 * BUTTON_HEIGHT + (1 * WINDOW_PADDING)); // Client Coordinate
	int cxTaskProgressWindowPixelWidth = rectWidthTaskProgressWindow; // Pixels
	int cyTaskProgressWindowPixelHeight = BUTTON_HEIGHT; // Pixels
	SetWindowPos(hwndTaskProgressWindow, HWND_TOP, leftSidePositionTaskProgressWindow, topPositionTaskProgressWindow, cxTaskProgressWindowPixelWidth, cyTaskProgressWindowPixelHeight, SWP_NOZORDER);
	
	// Set the Button Window position (Using Client Coordinates).
	int leftSidePositionButtonWindow = (rectWidth - WINDOW_PADDING) - BUTTON_WIDTH;  // Client Coordinate
	int topPositionButtonWindow = (rectHeight - WINDOW_PADDING) - BUTTON_HEIGHT; // Client Coordinate
	SetWindowPos(hwndButtonWindow, HWND_TOP, leftSidePositionButtonWindow, topPositionButtonWindow, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	
	// Set each Progress Window position (Using Client Coordinates AND Pixels).
	int rectWidthProgressWindow = ((rectWidth - (WINDOW_PADDING * (2 + this->maxCPUCount))) - BUTTON_WIDTH) / this->maxCPUCount;
	for (unsigned long index = 0; index < this->maxCPUCount; ++index)
	{	
		int leftSidePositionProgressWindow = (WINDOW_PADDING * (index + 1)) + (index * rectWidthProgressWindow); // Client Coordinate
		int topPositionProgressWindow = (rectHeight - WINDOW_PADDING) - BUTTON_HEIGHT; // Client Coordinate
		int cxProgressWindowPixelWidth = rectWidthProgressWindow; // Pixels
		int cyProgressWindowPixelHeight = BUTTON_HEIGHT; // Pixels
		SetWindowPos(hwndProgressWindow[index], HWND_TOP, leftSidePositionProgressWindow, topPositionProgressWindow, cxProgressWindowPixelWidth, cyProgressWindowPixelHeight, SWP_NOZORDER);
	}
}

void MainApplication::ApplicationManager::CleanupDynamicStatusBarText()
{
	if (this->statusBarTextPtr != nullptr)
	{
		delete[] this->statusBarTextPtr;
		this->statusBarTextPtr = nullptr;
	}
}

void MainApplication::ApplicationManager::ResetEditWindowSuppport()
{
	// Reset the Edit Window supporting Data Structures.

	// Request ownership of the Critical Section.
	EnterCriticalSection(this->criticalSectionPtr);

	// Access the Shared Resource.
	this->totalFilesToProcess = 0; // (Modify Shared Data)

	// Release ownership of the Critical Section.
	LeaveCriticalSection(this->criticalSectionPtr);
	
	this->filesProcessedCount = 0;
	this->editWindowDisplayTextOutput = TEXT("");
	this->filePassedList.clear();

	// Clean up the Heap Memory used for the Map.
	for (map<wstring, list<wstring>*>::const_iterator iterator = this->filenameToErrorListMap.begin(); iterator != this->filenameToErrorListMap.end(); ++iterator)
	{
		// The current Entry (Key-Value Pair) in the Entry Set.
		wstring key = iterator->first;
		list<wstring>* value = iterator->second;
		delete value;
		value = nullptr;
	}

	this->filenameToErrorListMap.clear();
}

void MainApplication::ApplicationManager::CreateFileErroredText(wchar_t* outputFormat)
{
	// Create Text for Files Errored.
	size_t errorCount = this->filenameToErrorListMap.size(); // NOTE: size_t varies in size on 32-bit and 64-bit platforms.
	swprintf(this->textOutputBuffer, MAX_TEXT_SIZE, outputFormat, errorCount, (errorCount == 1) ? (STR_FILE) : (STR_FILES), STR_ERROR);  // Write to the buffer.

	this->editWindowDisplayTextOutput += this->textOutputBuffer;

	// Iterate over the Entry Set of the Map.
	for (map<wstring, list<wstring>*>::const_iterator iterator = this->filenameToErrorListMap.begin(); iterator != this->filenameToErrorListMap.end(); ++iterator)
	{
		// The current Entry (Key-Value Pair) in the Entry Set.
		wstring key = iterator->first;
		list<wstring>* value = iterator->second;

		// Add the filename to the output.
		this->editWindowDisplayTextOutput += L"\r\n";
		this->editWindowDisplayTextOutput += key;

		if (!value->empty())
		{
			// Iterate over the value List.
			for (list<wstring>::const_iterator it = value->begin(); it != value->end(); ++it)
			{
				// Add each error message for the filename to the output.
				this->editWindowDisplayTextOutput += L"\t<";
				this->editWindowDisplayTextOutput += *it;
				this->editWindowDisplayTextOutput += L">";
			}
		}
	}
}

void MainApplication::ApplicationManager::CreateFilePassedText(wchar_t* outputFormat)
{
	// Create Text for Files Passed.
	size_t passCount = this->filePassedList.size(); // NOTE: size_t varies in size on 32-bit and 64-bit platforms.
	swprintf(this->textOutputBuffer, MAX_TEXT_SIZE, outputFormat, passCount, (passCount == 1) ? (STR_FILE) : (STR_FILES), STR_PASS); // Write to the buffer.

	this->editWindowDisplayTextOutput += this->textOutputBuffer;
	this->filePassedList.sort();
	for (list<wstring>::const_iterator it = this->filePassedList.begin(); it != this->filePassedList.end(); ++it)
	{
		this->editWindowDisplayTextOutput += TEXT("\r\n");
		this->editWindowDisplayTextOutput += *it;
	}
}

void MainApplication::ApplicationManager::CreateEditWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndEditWindowRef, WNDPROC& wndprocEditProcedureRef)
{
	// ANSI_VAR_FONT specifies a proportional font based on the Windows character set. MS Sans Serif is typically used.
	HFONT hFontEditWindow = (HFONT)GetStockObject(ANSI_VAR_FONT);

	// Create the Edit Window.
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/edit-controls
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/edit-control-styles

	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY;
	hwndEditWindowRef = CreateWindow(WC_EDIT, nullptr, dwStyle, WINDOW_PADDING, WINDOW_PADDING, 0, 0, hwnd, nullptr, hInstance, nullptr);
	// Assign a Window Procedure to the Edit Window.
	// The SetWindowLong() function creates the Window Subclass by changing the Window Procedure associated with a particular Window Class, 
	// causing the System to call the new Window Procedure instead of the previous one. 
	// An application must pass any messages not processed by the new Window Procedure to the previous Window Procedure by calling CallWindowProc(). 
	// This allows the application to create a chain of Window Procedures.
	// The GWL_WNDPROC argument sets a new address for the Window Procedure.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlonga
	wndprocEditProcedureRef = (WNDPROC)SetWindowLongPtr(hwndEditWindowRef, GWLP_WNDPROC, (LONG_PTR)EditWndProc);

	SendMessage(hwndEditWindowRef, WM_SETFONT, (WPARAM)hFontEditWindow, 1);
	SendMessage(hwndEditWindowRef, WM_SETTEXT, NULL, (LPARAM)STR_START_EDIT_WINDOW);
}

void MainApplication::ApplicationManager::CreateButtonWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndButtonWindowRef)
{
	// SYSTEM_FONT specifies the System font. This is a proportional font based on the Windows character set,
	// and is used by the Operating System to display Window titles, Menu names, and text in Dialog Boxes.
	HFONT hFontButtonWindow = (HFONT)GetStockObject(ANSI_VAR_FONT);

	// Create the Button Window.
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/buttons
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/button-styles
	hwndButtonWindowRef = CreateWindow(WC_BUTTON, STR_BUTTON_TEXT, WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_CENTER | BS_VCENTER, 0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, nullptr, hInstance, nullptr);
	
	SendMessage(hwndButtonWindowRef, WM_SETFONT, (WPARAM)hFontButtonWindow, true);
}

void MainApplication::ApplicationManager::CreateStatusBarWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndStatusBarWindowRef, HWND& hwndStatusBarProgressWindowRef)
{
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/status-bar-reference
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes
	// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/progress-bar-control-reference

	// Enable Disable the Status Bar.
	this->statusBarEnabledFlag = true;

	hwndStatusBarWindowRef = CreateWindow(STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, nullptr, hInstance, nullptr);
	
	// Set the size of Status Bar Parts in the Status Bar, using Client Coordinates.
	this->SetStatusBarWindowPartsSize(hwnd, hwndStatusBarWindowRef);

	// Set the text for each Status Bar Part. 
	// (The maximum number of allowed Status Bar Parts is set in the NUMBER_OF_STATUS_BAR_PARTS Constant)
	SendMessage(hwndStatusBarWindowRef, SB_SETTEXT, STATUS_BAR_PART_1, reinterpret_cast<LPARAM>(L"Status: Ready"));

	assert(this->statusBarTextPtr == nullptr); // Should be a null pointer at time of Status Bar creation.
	this->statusBarTextPtr = new wchar_t[MAX_TEXT_SIZE] {};
	wcscpy_s(this->statusBarTextPtr, MAX_TEXT_SIZE, L"R∃xK∀xLL © 2024 - Released under the MIT License."); // Copy to the Heap.
	SendMessage(hwndStatusBarWindowRef, SB_SETTEXT, STATUS_BAR_PART_2, reinterpret_cast<LPARAM>(this->statusBarTextPtr));
	
	hwndStatusBarProgressWindowRef = CreateWindow(PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE, 0, 0, 0, 0, hwndStatusBarWindowRef, nullptr, hInstance, nullptr);

	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setmarquee
	SendMessage(hwndStatusBarProgressWindowRef, PBM_SETMARQUEE, FALSE, 0);

	// Hide the Progress Bar Window.
	ShowWindow(hwndStatusBarProgressWindowRef, SW_HIDE);

}

void MainApplication::ApplicationManager::CreateCPUProgressWindow(HINSTANCE hInstance, HWND hwnd, HWND hwndProgressWindow[]) const
{
	for (unsigned long index = 0; index < this->maxCPUCount; ++index)
	{
		HWND progressWindow = CreateWindow(PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0, 0, 0, 0, hwnd, nullptr, hInstance, nullptr);
		hwndProgressWindow[index] = progressWindow;
	}
}

void MainApplication::ApplicationManager::CreateTaskProgressWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndTaskProgressWindowRef)
{
	// Create a Window that will display a single Progress Window, that displays the overall processing progress of the Application.
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/progress-bar-control-reference

	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes
	// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles

	HWND progressWindow = CreateWindow(PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0, 0, 0, 0, hwnd, nullptr, hInstance, nullptr);
	hwndTaskProgressWindowRef = progressWindow;
	
	// Send a Message to manually set the range of the Progress Window. (Default range is 0 to 100)
	// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setrange32
	int progressWindowLowMinimum = 0;
	int progressWindowHighMaximum = 100;
	SendMessage(hwndTaskProgressWindowRef, PBM_SETRANGE32, progressWindowLowMinimum, progressWindowHighMaximum);

	// Send a Message to set the step increment of Progress Window.
	int stepIncrement = 10;
	SendMessage(hwndTaskProgressWindowRef, PBM_SETSTEP, stepIncrement, 0);
}

void MainApplication::ApplicationManager::SetStatusBarWindowPartsSize(HWND hwnd, HWND& hwndStatusBarWindowRef)
{
	// Use Client Coordinates to set the positions of the Status Bar Parts.
	RECT rectStruct;
	GetClientRect(hwnd, &rectStruct);
	int rectWidth = rectStruct.right - rectStruct.left; // (x-coordinate of the lower-right corner of the Rectangle - x-coordinate of the upper-left corner of the Rectangle)
	int rectHeight = rectStruct.bottom - rectStruct.top; // (y-coordinate of the lower-right corner of the Rectangle - y-coordinate of the upper-left corner of the Rectangle)

	// Tell the Status Bar to set the Status Bar Parts.
	int statusBarPartWidthPositions[NUMBER_OF_STATUS_BAR_PARTS] = { 0 };
	for (int statusBarPartIndex = 0; statusBarPartIndex < NUMBER_OF_STATUS_BAR_PARTS; ++statusBarPartIndex)
	{
		statusBarPartWidthPositions[statusBarPartIndex] = (rectStruct.right / TOTAL_STATUS_BAR_PARTS_WIDTHS) * (statusBarPartIndex + 1); // Client Coordinate
	}

	int totalNumberOfWidthPositions = sizeof(statusBarPartWidthPositions) / sizeof(int);
	SendMessage(hwndStatusBarWindowRef, SB_SETPARTS, (WPARAM)totalNumberOfWidthPositions, (LPARAM)statusBarPartWidthPositions);
}

#pragma endregion Private_Member_Functions_Region