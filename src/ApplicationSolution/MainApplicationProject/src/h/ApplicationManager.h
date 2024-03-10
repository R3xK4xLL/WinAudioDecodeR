#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <wtypes.h>
#include <string>
#include <cassert>
#include <functional>
#include "MainWinAPI.h"
#include "DecoderManager.h"
#include "ExecutionManager.h"

// Universal Naming Convention (UNC) path support via "\\?\UNC" prefix. (Example: "\\?\UNC\server\share").
constexpr auto FILE_NAMESPACE_UNC_PREFIX = L"\\\\?\\UNC\\";
constexpr auto FILE_NAMESPACE_UNC_PREFIX_SIZE = 8;

// Extended File I/O path support via "\\?\" prefix.
constexpr auto FILE_NAMESPACE_PREFIX = L"\\\\?\\";
constexpr auto FILE_NAMESPACE_PREFIX_SIZE = 4;

/// <summary>
/// Purpose: Enable to force the Application to use a Single CPU and a Single Thread.
/// </summary>
constexpr auto FORCE_SINGLE_CPU_ENABLED = false;

/// <summary>
/// Purpose: This namespace contains the Main Application elements, other than the WinAPI elements. 
/// The existence of this namespace helps to reduce unneccessary global variables.
/// </summary>
namespace MainApplication {

    /// <summary>
    /// Purpose: The Application Manager. Used to manage custom Application Data and Logic for the GUI.
    /// </summary>
    class ApplicationManager
    {
        public:
			/// <summary>
			/// Purpose: The with-args Constructor for the Application Manager.
			/// </summary>
			/// <param name="lpCriticalSection">A pointer to the Critical Section Object used by the current Process.</param>
			ApplicationManager(LPCRITICAL_SECTION lpCriticalSection);
			~ApplicationManager();
			ApplicationManager(const ApplicationManager& other) = delete; // Delete Copy Constructor
			ApplicationManager& operator=(const ApplicationManager& other) = delete; // Delete Assignment Operator (Overloaded)
			ApplicationManager(ApplicationManager&& other) noexcept = delete; // Delete The Move Constructor
			ApplicationManager& operator=(ApplicationManager&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)
			
			/// <summary>
			/// Purpose: A function the initializes the Application including creating Windows, Buttons, Progress Bars, Synchronization Support, Threads, and Timers.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd">A handle to the parent or owner window that will be used to configure various UI Elements within this function.</param>
			void Startup(HINSTANCE hInstance, HWND hwnd);
			
			/// <summary>
			/// Purpose: Stops the Application Timer and Stops all Threads.
			/// </summary>
			void Shutdown();

			/// <summary>
			/// Purpose: Resizes all of the Application Controls relative to the Parent Control.
			/// </summary>
			/// <param name="hwnd">The Parent Control</param>
			void ResizeControls(HWND hwnd);
			
			/// <summary>
			/// Purpose: Opens a Common File Dialog Box used to open Files. Handles all logic for processing selected files.
			/// </summary>
			void OpenFileDialogBox();

			/// <summary>
			/// Purpose: Opens a Shell Folder Dialog Box used to open a Folder. Handles all logic for processing the selected folder.
			/// </summary>
			void OpenSelectFolderDialogBox();

			/// <summary>
			/// Purpose: Add all of the supported file types and prepares them for Thread-based Processing.
			/// </summary>
			/// <param name="directoryPathPtr">The directory path containing the specified filename(s).</param>
			/// <param name="filenamePtr">A filename pointer containing 1 or more null-terminated filename strings.</param>
			void OpenFiles(wchar_t* directoryPathPtr, wchar_t* filenamePtr);

			/// <summary>
			/// Purpose: Opens a Path with a Callback Function. Typically used to prepare File and Folder/SubFolders Paths for Thread-based Processing.
			/// </summary>
			/// <param name="messageBoxDisplayFlagEnabled">TRUE displays Message Boxes to the User. FALSE the display of Message Boxes is suppressed.</param>
			/// <param name="boundCallback">The Callback Function used to perform on operation on the specified Path.</param>
			/// <param name="pathnamePtr">The Path used by the Callback Function.</param>
			/// <param name="setPendingEventFlag">>A flag used for setting the Pending Event, to initiate/continue Thread-based processing via the Callback Function.</param>
			void OpenPathWithCallback(bool messageBoxDisplayFlagEnabled, std::function<void(const wchar_t*, bool)> boundCallback, wchar_t* pathnamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: Processes Files and Folders that were Dragged and Dropped by a User. 
			/// If the Files and Folders are (or contain) Supported File Types, then they are added to an internally managed List of Filenames.
			/// </summary>
			/// <param name="hDrop">A Handle to an internal structure describing the Dropped Files.</param>
			void OnDrop(HDROP hDrop);
			
			/// <summary>
			/// Purpose: The purpose of this function is to check whether the specified file exists and is a supported type. 
			/// If the file is supported then it is added to an internally managed List of Filenames.
			/// </summary>
			/// <param name="filenamePtr">The name of a File OR a Folder.</param>
			/// <param name="setPendingEventFlag">A flag used for setting the Pending Event, to initiate/continue Thread-based processing.</param>
			void AddSupportedTypeSync(const wchar_t* filenamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: The purpose of this function is to check whether the specified file exists and is a supported type. 
			/// If the file is supported then it is added to an internally managed List of Filenames.
			/// </summary>
			/// <param name="filenamePtr">The name of a File OR a Folder.</param>
			/// <param name="setPendingEventFlag">
			/// A flag used for setting the Pending Event, to initiate/continue Thread-based processing AND 
			/// to initiate/continue Thread-based Folder processing within the Execution Manager.
			/// </param>
			void AddSupportedTypeAsync(const wchar_t* filenamePtr, bool setPendingEventFlag);
			
			/// <summary>
			/// Purpose: The purpose of this function is to check whether the specified File exists and is a supported type. 
			/// If the file is supported then it is added to an internally managed List of Filenames.
			/// </summary>
			/// <param name="filenamePtr"></param>
			/// <param name="setPendingEventFlag">A flag used for setting the Pending Event, to initiate/continue Thread-based processing.</param>
			void AddFileSync(const wchar_t* filenamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: A Function Stub. Not yet implemented.
			/// </summary>
			/// <param name="filenamePtr"></param>
			/// <param name="setPendingEventFlag"></param>
			void AddFileAsync(const wchar_t* filenamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: The purpose of this function is to check whether any of these Files within the specified Folder/SubFolders are supported types. 
			/// If a File is found in the Folder/SubFolders that is supported, then it is added to an internally managed List of Filenames.
			/// 
			/// NOTE: This is a synchronous function that will process all the Folder/Subfolders before returining the to the Caller.
			/// </summary>
			/// <param name="filenamePtr"></param>
			/// <param name="setPendingEventFlag">A flag used for setting the Pending Event, to initiate/continue Thread-based processing.</param>
			void AddFolderSync(const wchar_t* filenamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: The purpose of this function is to check whether any of these Files within the specified Folder/SubFolders are supported types. 
			/// If a File is found in the Folder/SubFolders that is supported, then it is added to an internally managed List of Filenames.
			/// 
			/// NOTE: This is an asynchronous function that will process all the Folder/Subfolders at a later determined time via the Execution Manager, and return immediately to the Caller.
			/// </summary>
			/// <param name="filenamePtr"></param>
			/// <param name="setPendingEventFlag">A flag used for setting the Pending Event, to initiate/continue Thread-based Folder processing within the Execution Manager.</param>
			void AddFolderAsync(const wchar_t* filenamePtr, bool setPendingEventFlag);

			/// <summary>
			/// Purpose: The purpose of this function is to set the Pending Event, which indicates that there are files pending for Thread-based Processing.
			/// Indirectly uses the Critical Section to perform this operation.
			/// </summary>
			void SetPendingEvent();

			/// <summary>
			/// Purpose: Gets the next Filename from an internally managed List, if one is available. Uses the Critical Section to perform this operation.
			/// </summary>
			/// <param name="filenameRef">The Filename is set after a successful retrieval.</param>
			/// <param name="handlePendingEvent"></param>
			/// <returns>TRUE if an available filename was found. Also modifies the filenameRef to the value and removes the filename from the List of filenames. 
			/// FALSE if a filename was not available and filenameRef is not modified. This is occurs when the List of filenames is empty.</returns>
			bool GetNextAvailableFilename(std::wstring& filenameRef, HANDLE handlePendingEvent);
			
			/// <summary>
			/// Purpose: Formats and displays text for the Edit Window. NOTE: Any wchar_t* passed as argument to this function, will be deleted. 
			/// Uses the Critical Section to perform this operation.
			/// </summary>
			/// <param name="filenamePtr">This will be deleted.</param>
			/// <param name="errorMessagePtr">This willl be deleted.</param>
			void SetEditWindowText(wchar_t* filenamePtr, wchar_t* errorMessagePtr);
			
			/// <summary>
			/// Purpose: Sets the start of the Timer Tick Count.
			/// </summary>
			/// <param name="timerStartTickCountRef">Updated with the starting Tick Count for the started Timer.</param>
			void StartTimer(ULONGLONG& timerStartTickCountRef);

			/// <summary>
			/// Purpose: Computes the elapsed Timer Tick Count since the Timer was started.
			/// </summary>
			/// <param name="timerStartTickCountRef">The starting Tick Count value for the Timer that is used to compute the Elapsed Tick Count.</param>
			void StopTimer(const ULONGLONG& timerStartTickCountRef);

			/// <summary>
			/// Purpose: This function calculate the percentage of files processed and updates the Window Title Text of the Main Application Window. 
			/// Uses the Critical Section to perform this operation.
			/// </summary>
			/// <param name="hwndMainApplicationWindow"></param>
			void UpdateWindowTitleTextPercent(HWND hwndMainApplicationWindoww) const;

			/// <summary>
			/// Purpose: This function calculate the percentage of files processed and updates the Task Progress Window. 
			/// Uses the Critical Section to perform this operation.
			/// </summary>
			void UpdateTaskProgressWindowPercent() const;

			/// <summary>
			/// Purpose: This function retrieves the 2nd command-line argument passed to the current Process.
			/// </summary>
			/// <returns>The 2nd command-line argument. If there are only 0 or 1 arguments, the function return a null pointer. T
			/// It is the Caller's responsibility to dispose of the Heap Memory associated with the returned wchar_t*.</returns>
			wchar_t* GetSecondCommandLineArgument();

			/// <summary>
			/// Purpose: Opens a Decoder for the specified filename and returns Memory-managed Smart Pointer to the Decoder.
			/// </summary>
			/// <param name="filenamePtr">The filename.</param>
			/// <returns>A Memory-managed Smart Pointer to the Decoder. The returned pointer can be NULL.</returns>
			std::unique_ptr<PureAbstractBaseDecoder> OpenDecoder(const wchar_t* filenamePtr);

			/// <summary>
			/// Purpose: Increments number of Files Processed. Typically invoked after a MSG_THREAD_FILE_PROCESSED Message, which is when a Thread finishes processsing a File.
			/// </summary>
			void IncrementFilesProcessed();
			
			/// <summary>
			/// Purpose: Set the Status Bar Text using static (compile-time) readonly text (e.g. String Literals).
			/// </summary>
			/// <param name="statusBarPartID"></param>
			/// <param name="newStatusBarTextPtr">The new text that will set in the Status Bar.</param>
			void SetStaticStatusBarText(int statusBarPartID, wchar_t* newStatusBarTextPtr);

			/// <summary>
			/// Purpose: Set the Status Bar Text using dynamic (run-time) text (e.g. C-style Strings created on the Heap).
			/// </summary>
			/// <param name="statusBarPartID"></param>
			/// <param name="newStatusBarTextPtr">The new text that will set in the Status Bar.</param>
			void SetDynamicStatusBarText(int statusBarPartID, wchar_t* newStatusBarTextPtr);
			
			/// <summary>
			/// Purpose: A function that posts an Asynchronous Message that sends Dynamically Allocated Text for use in the Status Bar.
			/// </summary>
			/// <param name="statusBarTextPtr">The string data that will be sent in the Message. 
			/// The string data is copied, therefore the Caller may delete the pointer afterwards, without consequence.
			/// </param>
			/// <param name="bufferSize"></param>
			void PostDynamicStatusBarMessage(const wchar_t* statusBarTextPtr, size_t bufferSize);

			/// <summary>
			/// Purpose: Gets the current Text for the Status Bar.
			/// </summary>
			/// <param name="hwndStatusBarWindow"></param>
			/// <param name="statusBarTextID"></param>
			/// <returns></returns>
			std::unique_ptr<wchar_t> GetStatusBarText(HWND hwndStatusBarWindow, int statusBarTextID);

			/// <summary>
			/// Purpose: Clears the internally managed List of Filenames. Uses the Critical Section to perform this operation.
			/// </summary>
			void ClearListOfFilenames();
			
			/// <summary>
			/// Purpose: Get the Max CPU Count supported by the Application Manager.
			/// </summary>
			/// <returns></returns>
			DWORD GetMaxCPUCount() const;
			
			/// <summary>
			/// Purpose: Generates and displays the Final Report in the Edit Window.
			/// </summary>
			void GenerateFinalReport();
			
			/// <summary>
			/// Purpose: Sets the Application Manager and any other dependent associated services, to the Stopping State (TRUE). Uses the Critical Section to perform this operation.
			/// </summary>
			void EnableStoppingState();

			/// <summary>
			/// Purpose: Determines whether the Application Manager and any other dependent associated Services, are in the Stopping State (TRUE) or Waiting/Running State (FALSE). 
			/// Uses the Critical Section to perform this operation.
			/// </summary>
			/// <returns></returns>
			bool IsStoppingState();

			/// <summary>
			/// Purpose: Determines whether the Application Manager and any other dependent associated Services, are in the Running State (TRUE) OR in the Waiting/Stopping State (FALSE). 
			/// Uses the Critical Section to perform this operation.
			/// </summary>
			/// <returns></returns>
			bool IsRunningState();

			#pragma region Getter_Setter_Functions_Region
			
			bool IsStoppingStateFlagEnabled() const;
			void SetStoppingStateFlagEnabled(bool value);

			HINSTANCE GetInstance() const;
			void SetInstance(HINSTANCE hInstance);

			HWND GetMainApplicationWindow() const;
			void SetMainApplicationWindow(HWND hwndMainApplicationWindow);

			HWND GetEditWindow() const;
			void SetEditWindow(HWND hwndEditWindow);

			HWND GetButtonWindow() const;
			void SetButtonWindow(HWND hwndButtonWindow);

			HWND GetTaskProgressWindow() const;
			void SetTaskProgressWindow(HWND hwndTaskProgressWindow);

			HWND GetProgressWindow(unsigned long index) const;
			void SetProgressWindow(HWND hwndProgressWindow, unsigned long index);

			WNDPROC GetEditProcedure() const;
			void SetEditProcedure(WNDPROC wndprocEditProcedure);

			HWND GetStatusBarWindow() const;
			void SetStatusBarWindow(HWND hwndStatusBarWindow);

			HANDLE GetHandleThread(unsigned long index) const;
			void SetHandleThread(HANDLE handleThread, unsigned long index);

			HANDLE GetTerminateEvent() const;
			void SetTerminateEvent(HANDLE handleTerminateEvent);

			HANDLE GetPendingEvent() const;
			void SetPendingEvent(HANDLE handlePendingEvent);

			const HANDLE* GetFinishedEventArray() const;
			HANDLE GetFinishedEvent(unsigned long index) const;
			void SetFinishedEvent(HANDLE handleFinishedEvent, unsigned long index);

			ULONGLONG GetTimerStartTickCount() const;
			void SetTimerStartTickCount(ULONGLONG timerStartTickCount);
			
			bool StopButtonPressedFlagEnabled() const;
			void SetStopButtonPressedFlag(bool value);

			#pragma endregion Getter_Setter_Functions_Region
			
		private:
			std::list<std::wstring> listOfFilenames{};
			std::list<std::wstring> filePassedList{};
			std::map<std::wstring, std::list<std::wstring>*> filenameToErrorListMap{};

			LPCRITICAL_SECTION criticalSectionPtr{ nullptr };
			DecoderManager* decoderManagerPtr{ nullptr };
			ExecutionManager* executionManagerPtr{ nullptr };

			DWORD maxCPUCount{};
			float elapsedTimerCount{};
			int filesProcessedCount{};
			int totalFilesToProcess{};

			/// <summary>
			/// Purpose: A Flag to indicate whether Thread-based Processing was force stopped by the User.
			/// </summary>
			bool stoppingStateFlagEnabled{ false };

			// Used for the storage of text that is displayed in the Edit Window.
			std::wstring editWindowDisplayTextOutput{};
			
			// A text buffer used for processing C-style strings that will be displayed in the Edit Window.
			wchar_t textOutputBuffer[MAX_TEXT_SIZE]{};

			// Stores Dynamically Allocated Text that is used in the Status Bar.
			wchar_t* statusBarTextPtr{ nullptr };
			
			HINSTANCE hInstance{ nullptr };
			HWND hwndMainApplicationWindow{ nullptr };

			// The Edit Window used to display text.
			HWND hwndEditWindow{ nullptr };

			HWND hwndButtonWindow{ nullptr };
			HWND hwndTaskProgressWindow{ nullptr };

			// An array that contains 1 Progress Window Handle per Logical CPU.
			HWND hwndProgressWindow[MAX_CPU]{ nullptr };

			WNDPROC wndprocEditProcedure{ nullptr };
			HWND hwndStatusBarWindow{ nullptr };
			HWND hwndStatusBarProgressWindow{ nullptr };

			// An array that contains 1-Thread per Logical CPU.
			HANDLE handleThread[MAX_CPU]{ nullptr };

			HANDLE handleTerminateEvent{ nullptr };
			HANDLE handlePendingEvent{ nullptr };

			// An array that contains 1 Finished Event Object per Logical CPU.
			HANDLE handleFinishedEvent[MAX_CPU]{ nullptr };

			ULONGLONG timerStartTickCount{};

			/// <summary>
			/// Purpose: A Flag used to indicate whether the Stop Button was pressed during Thread-based Processing.
			/// </summary>
			bool stopButtonPressedFlagEnabled{ false };
			
			/// <summary>
			/// Purpose: A Flag used to indicate whether a Status Bar Window is currently one of the active Controls.
			/// </summary>
			bool statusBarEnabledFlag{ false };

			// Used to configure the number of Status Bar Parts in the Status Bar.
			static constexpr auto NUMBER_OF_STATUS_BAR_PARTS = 3;

			// Used to configure the total number of Status Bar Part widths in the Status Bar.
			static constexpr auto TOTAL_STATUS_BAR_PARTS_WIDTHS = 3;
			
			/// <summary>
			/// Purpose: A helper function that initializes and configures the Application Manager at Startup.
			/// </summary>
			void ApplicationManagerStartup();

			/// <summary>
			/// Purpose: Creates Synchronization API Support Objects.
			/// </summary>
			/// <param name="handleTerminateEventRef"></param>
			/// <param name="handlePendingEventRef"></param>
			void CreateSynchronizationSupport(HANDLE& handleTerminateEventRef, HANDLE& handlePendingEventRef);

			/// <summary>
			/// Purpose: Creates a Thread for each Logical CPU. If any of the Threads fail to be created, the User is notified and the Program is terminated.
			/// </summary>
			/// <param name="hwnd"></param>
			/// <param name="handleFinishedEvent"></param>
			/// <param name="handleThread"></param>
			void CreateThreads(HWND hwnd, HANDLE handleFinishedEvent[], HANDLE handleThread[]) const;
			
			/// <summary>
			/// Purpose: The function manages the orderly termination of all previously created Threads.
			/// </summary>
			void StopThreads();
			
			/// <summary>
			/// Purpose: Asynchronously scans the specified Folder, searching for all supported File Types. 
			/// If the file is supported then it is added to an internally managed List of Filenames.
			/// Primarily used by the Execution Manager as a Callback Function. Uses the Critical Section to perform this operation.
			/// </summary>
			/// <param name="folderPtr"></param>
			void ScanFolderAsync(const wchar_t* folderPtr);

			/// <summary>
			/// Purpose: Scans the specified Folder/SubFolders, searching for all supported File Types. 
			/// If the File is supported then it is processed using the specified Callback Function.
			/// </summary>
			/// <param name="boundCallback">A Callback Function used to perform an operation on the supported File Types.</param>
			/// <param name="folderPtr">The starting Folder for the search.</param>
			void ScanFolderWithCallback(std::function<void(const wchar_t*)> boundCallback, const wchar_t* folderPtr);

			/// <summary>
			/// Purpose: Recursively Scans the specified Folder/SubFolders, searching for all supported File Types. 
			/// If the File is supported then it is processed using the specified Callback Function.
			/// </summary>
			/// <param name="boundCallbackRef">A Callback Function used to perform an operation on the supported File Types.</param>
			/// <param name="folderPtr">The starting Folder for the search.</param>
			void rScanFolderWithCallback(const std::function<void(const wchar_t*)>& boundCallbackRef, const wchar_t* folderPtr);

			/// <summary>
			/// Purpose: Adds a Filename to an internally managed List of Filenames. Uses the Critical Section to perform this operation.
			/// </summary>
			/// <param name="filenamePtr"></param>
			void AddToFileList(const wchar_t* filenamePtr);

			/// <summary>
			/// Purpose: Resizes all of the Controls relative to the Parent Control. Use this function when the Status Bar is enabled.
			/// </summary>
			/// <param name="hwnd"></param>
			void ResizeControlsStatusBarEnabled(HWND hwnd);

			/// <summary>
			/// Purpose: Resizes all of the Controls relative to the Parent Control. Use this function when the Status Bar is disabled.
			/// </summary>
			/// <param name="hwnd"></param>
			void ResizeControlsStatusBarDisabled(HWND hwnd);

			/// <summary>
			/// Purpose: An internal function to that cleans up any Dynamically Allocated Memory used for displaying Text on the Status Bar.
			/// </summary>
			void CleanupDynamicStatusBarText();

			/// <summary>
			/// Purpose: Resets the Edit Window supporting Data Structures. Uses the Critical Section to perform this operation.
			/// </summary>
			void ResetEditWindowSuppport();

			/// <summary>
			/// Purpose: Creates File(s) Errored Text for the Edit Window.
			/// </summary>
			/// <param name="outputFormat"></param>
			void CreateFileErroredText(wchar_t* outputFormat);

			/// <summary>
			/// Purpose: Creates File(s) Passed Text for the Edit Window.
			/// </summary>
			/// <param name="outputFormat"></param>
			void CreateFilePassedText(wchar_t* outputFormat);
			
			/// <summary>
			/// Purpose: Configures and creates the Edit Window.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd"></param>
			/// <param name="hwndEditWindowRef"></param>
			/// <param name="wndprocEditProcedureRef"></param>
			void CreateEditWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndEditWindowRef, WNDPROC& wndprocEditProcedureRef);

			/// <summary>
			/// Purpose: Configures and creates the Button Window.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd"></param>
			/// <param name="hwndButtonWindowRef"></param>
			void CreateButtonWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndButtonWindowRef);

			/// <summary>
			/// Purpose: Configures and creates the Status Bar Window.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd"></param>
			/// <param name="hwndStatusBarWindowRef"></param>
			/// <param name="hwndStatusBarProgressWindowRef"></param>
			void CreateStatusBarWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndStatusBarWindowRef, HWND& hwndStatusBarProgressWindowRef);

			/// <summary>
			/// Purpose: Creates a Progress Window for each Logical CPU.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd"></param>
			/// <param name="hwndProgressWindow"></param>
			void CreateCPUProgressWindow(HINSTANCE hInstance, HWND hwnd, HWND hwndProgressWindow[]) const;

			/// <summary>
			/// Purpose: Creates a Progress Window that displays overall Task Progress.
			/// </summary>
			/// <param name="hInstance"></param>
			/// <param name="hwnd"></param>
			/// <param name="hwndTaskProgressWindowRef"></param>
			void CreateTaskProgressWindow(HINSTANCE hInstance, HWND hwnd, HWND& hwndTaskProgressWindowRef);

			/// <summary>
			/// Purpose: Sets the size of the Status Bar Parts using Client Coordinates. Also, can be use used to dynamically resize the Status Bar Parts.
			/// </summary>
			/// <param name="hwnd"></param>
			/// <param name="hwndStatusBarWindowRef"></param>
			void SetStatusBarWindowPartsSize(HWND hwnd, HWND& hwndStatusBarWindowRef);
    };

}

#endif // APPLICATION_MANAGER_H
