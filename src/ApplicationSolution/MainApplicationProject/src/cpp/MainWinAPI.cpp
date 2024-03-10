#pragma region LICENSE_REGION

/*

WinAudioDecodeR

Copyright © 2024 R∃xK∀xLL
Copyright © 2015 James Chapman

Licensed under the terms of the MIT License, see LICENSE file for details.

*/

#pragma endregion LICENSE_REGION

#pragma region COMPILER_DIRECTIVES_REGION

// Use a specific version of the Common Controls Library (ComCtl32.dll) to set the overall appearance of the Application.
// NOTE: By default, Applications use the User controls defined in User32.dll and the Common Controls defined in ComCtl32.dll version 5.
// 
// The Common Controls Library version 6 contains both the User Controls and the Common Controls.
// Using Common Controls Library version 6 or later, enables your Application to use Visual Styles.
// To use Visual Styles, you must add an Application Manifest or Compiler Directive that indicates that Common Controls Library version 6 should be used, if it is available.
// An Application Manifest enables an Application to specify which versions of an Assembly it requires.
// URI: https://learn.microsoft.com/en-us/windows/win32/controls/cookbook-overview
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Instruct the Linker to include the Shlwapi.lib Library during the linking phase.
// When the Compiler encounters this Directive, it adds a reference to the specified Library (Shlwapi.lib) in the final executable.
// URI: https://learn.microsoft.com/en-us/cpp/preprocessor/comment-c-cpp?view=msvc-170#lib
#pragma comment(lib, "Shlwapi.lib")

#pragma endregion COMPILER_DIRECTIVES_REGION

#include "MainWinAPI.h"
#include "ApplicationManager.h"
#include "ExecutionManager.h"

using namespace std;

#ifndef MAIN_WINAPI_GLOBALS_CPP
#define MAIN_WINAPI_GLOBALS_CPP

// Declare and initialize Global Variables for use with WinAPI Program.

HINSTANCE g_hInstance{ nullptr };
HWND g_hwndMainApplicationWindow{ nullptr };

// Global variable for the Critical Section (Provides Mutual-Exclusion Synchronization).
CRITICAL_SECTION g_criticalSection{ nullptr };

uint64_t g_decoderProgressTracker[MAX_CPU]{};

// The Global Application Manager.
MainApplication::ApplicationManager* g_applicationManagerPtr{ nullptr };

#endif // MAIN_WINAPI_GLOBALS_CPP

#pragma region WinAPI_Functions_Region

// ****************************************************************************
//							WinAPI Functions
// ****************************************************************************

// Useful Tutorial and Overview of how to create a basic WinAPI Program.
// URI: https://learn.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	g_hInstance = hInstance;

	// Initialize the Critical Section Object for this Process. 
	// The Threads of a single Process can use Critical Section Objects for Mutual-exclusion Synchronization.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection
	//InitializeCriticalSection(&g_criticalSection);
	
	// Common Spin Count Values:
	// 
	// Background: The spin count parameter determines how many times a Thread will spin (busy-wait) before 
	// transitioning to a 'Wait State' when trying to acquire a locked Critical Section.
	//
	// Zero (0):
	// 
	//		- On Single-processor Systems, the spin count is ignored, and the Critical Section behaves as if it has no spin count.
	//		- The thread immediately goes to sleep if the Critical Section is locked.
	// 
	// Low Spin Counts (e.g. 100 - 1,000):
	// 
	//		- Suitable for critical sections with low contention.
	//		- Threads spin briefly before waiting.
	// 
	// Medium Spin Counts (e.g. 1,000 - 10,000):
	// 
	//		- Appropriate for moderately contended Critical Sections.
	//		- Balances spinning and waiting.
	// 
	// High Spin Counts (e.g. 10,000 - 100,000):
	//		
	//		- Useful for highly contended critical sections.
	//		- Threads spin extensively before waiting.
	//		- Be cautious with very high spin counts; they may cause cache contention.
	//

	// Initialize the Critical Section Object for this Process. 
	// The Threads of a single Process can use Critical Section Objects for Mutual-exclusion Synchronization.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
	bool returnStatus = InitializeCriticalSectionAndSpinCount(&g_criticalSection, 1000);
	if (returnStatus == 0)
	{
		// An error occured while initializing the Critical Section.
		MessageBox(NULL, L"Initialization of the Critical Section Failed!", L"Application Startup Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Create an Application Manager on the Heap.
	g_applicationManagerPtr = new MainApplication::ApplicationManager(&g_criticalSection);

	// Create and Initialize a Window Class for the Main Application Window.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexw
	WNDCLASSEX windowClass;
	ZeroMemory(&windowClass, sizeof(WNDCLASSEX)); // Set all members of the WNDCLASSEX structre to zero.
	windowClass.cbSize = sizeof(WNDCLASSEX); // Use the size of the WNDCLASSEX structure to set the WNDCLASSEXW size in bytes.
	windowClass.hCursor = LoadCursor(0, IDC_ARROW); // A Handle to the Cursor to be displayed when the Mouse Pointer is over the Window.
	windowClass.hInstance = hInstance; // A Handle to the instance of the Module associated with the Window Class.
	windowClass.lpfnWndProc = WndProc; // A pointer to the Window Procedure for the Window Class.
	windowClass.lpszClassName = WINDOW_TITLE; // A null-terminated string that specifies the name of the Window Class.
	windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW; // A Handle to the Brush that is used to paint the background of the Window.
	windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LARGE_APPLICATION_ICON)); // A Handle to the Icon that is associated with the Window Class.
	windowClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL_APPLICATION_ICON)); // A Handle to the small Icon that is associated with the Window Class.
	windowClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1); // Specifies the resource ID from the Resource file, used for the Window Class Default Menu.
	
	// Registers a Window Class for subsequent use in calls to the CreateWindow() or CreateWindowEx() function.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassexw
	if (RegisterClassEx(&windowClass) == 0)
	{
		// An error occured while registering the Window Class.
		MessageBox(NULL, L"Window Registration Failed!", L"Application Startup Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Retrieves a handle to the top-level window whose class name and window name match the specified strings. 
	// This function does not search child windows. This function does not perform a case-sensitive search.
	// Return: 
	// If the function succeeds, the return value is a class atom that uniquely identifies the class being registered. 
	// If the function fails, the return value is NULL.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-findwindoww
	HWND foundWindowHandle = FindWindow(WINDOW_TITLE, NULL);
	if (foundWindowHandle != NULL)
	{
		// Only allow one Main Application Window instance.
		MessageBox(NULL, L"Multiple Window Instances encountered.", L"Application Startup Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	// Creates an overlapped, pop-up, or child window with an extended window style.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw

	// An Overlapped Window is a top-level Window (non-child Window) that has a Title Bar, Border, and Client Area.
	// It is meant to serve as an Application's Main Window. It can also have a Window Menu, Minimize and Maximize Buttons, and Scroll Bars. 
	// An Overlapped Window used as a Main Application Window typically includes all of these components.
	
	DWORD dwExStyle = WS_EX_ACCEPTFILES; // An extended Window Style that specifies that the Window accepts drag-and-drop files.
	LPCWSTR lpClassName = WINDOW_TITLE; // The name of the Window Class to use for the Window.
	LPCWSTR lpWindowName = WINDOW_TITLE; // The Window name.
	DWORD dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW; // Creates a Window that has a Title Bar, a Sizing Border, and Minimize and Maximize Buttons.
	int X = CW_USEDEFAULT; // Set the initial horizontal position of the Window to the default position.
	int Y = 0; // Set the initial vertical position of the Window to the default position.
	int nWidth = CW_USEDEFAULT; // Set the Window to the default width.
	int nHeight = CW_USEDEFAULT; // Set the Window to the default height.
	HWND hWndParent = NULL; // Set the Window to not have a parent Window.
	HMENU hMenu = NULL; // Set the Window to not have a Menu.
	g_hwndMainApplicationWindow = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, NULL);
	
	// Configure the Main Application Window.
	if (g_hwndMainApplicationWindow != NULL)
	{
		// Configure the System Menu. (Displayed when the icon in the upper-left corner of the Window Title Bar is clicked.)
		HMENU menu = GetSystemMenu(g_hwndMainApplicationWindow, false);
		InsertMenu(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, ID_ABOUT, STR_ABOUTMENU);

		// Create the customized Application using the Main Application Window Handle as the parent.
		g_applicationManagerPtr->Startup(g_hInstance, g_hwndMainApplicationWindow);
		
		// Show the Window and force a WM_PAINT Message to be sent.
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
		ShowWindow(g_hwndMainApplicationWindow, nShowCmd);
		UpdateWindow(g_hwndMainApplicationWindow);

		// Get the 2nd Argument from the Command-Line. (NOTE: The 2nd Argument is expected to a Filename.)
		wchar_t* secondCommandLineArgument = g_applicationManagerPtr->GetSecondCommandLineArgument();
		if (secondCommandLineArgument != NULL)
		{
			SendMessage(g_hwndMainApplicationWindow, MSG_CMDLINE, (WPARAM)secondCommandLineArgument, 0);
		}
	}
	else
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Application Startup Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// The GetMessage() function retrieves a message from the calling thread's message queue. 
	// The function dispatches incoming sent messages until a posted message is available for retrieval.
	// GetMessage() blocks until a message is posted before returning.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage

	// A Message Structure that receives message information from the thread's message queue.
	MSG msg;

	// If the GetMessage() function retrieves the WM_QUIT message, the return value is zero. 
	// Otherwise, all other message return value are non-zero. If there is an error, the return value is - 1.
	BOOL bReturn;

	// Run the Message loop. The GetMessage() function is a blocking call (i.e. when the Message Queue is empty).
	// Incoming Messages on the Message Queue are processed in the While Loop, until either the WM_QUIT Message is encountered OR an error is encountered.
	// NOTE: The hWnd argument to the GetMessage() function is NULL, therefore both window messages and thread messages are processed.
	while (bReturn = GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (bReturn == -1)
			// Handle the error and exit.
			bReturn = 0;
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg); // The WinAPI DispatchMessage() function. Calls the Window Procedure function WndProc().
		}
	}

	// Clean up the Application Manager on the Heap.
	if (g_applicationManagerPtr != nullptr)
	{
		delete g_applicationManagerPtr;
		g_applicationManagerPtr = nullptr;
	}
	
	// Release all resources used by the Critical Section Object.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-deletecriticalsection
	DeleteCriticalSection(&g_criticalSection);

	// Return the Exit Code passed as a parameter in the WM_QUIT message. (Exit the Program)
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT messageResult = 0;

	// A typical window procedure is simply a large switch statement that switches on the message code. 
	// Add cases for each message that you want to handle.
	switch (uMsg)
	{
		case WM_CREATE:
			// This Message is sent when an Application requests that a Window be created by calling the CreateWindow() function.
			// This Message is sent BEFORE the CreateWindow() function returns.
			// The Window Procedure of the new Window receives this Message AFTER the Window is created, but BEFORE the Window becomes visible.
			// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
			break;
		case WM_CLOSE:
		{
			// This Message is sent as a signal that a Window or an Application should terminate.
			// WM_CLOSE is sent when the user presses the Close Button [x] or types Alt-F4.
			// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-close
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow

			bool enableExitMessageBox = false;
			if (enableExitMessageBox)
			{
				// Prompt the User with a Message Box. If the user clicks the Yes button, destroys the Window.
				if (MessageBox(hwnd, STR_MESSAGE_BOX_CLOSE, WINDOW_TITLE, MB_YESNOCANCEL) == IDYES)
				{
					// This Function sends a WM_DESTROY Message to the Window.
					DestroyWindow(hwnd);
				}
			}
			else
			{
				// This Function sends a WM_DESTROY Message to the Window.
				DestroyWindow(hwnd);
			}

			break;
		}
		case WM_DESTROY:
			// This Message is sent when a Window is being destroyed.
			// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-destroy

			// Stop All Timers and Threads via a call to Shutdown function.
			g_applicationManagerPtr->Shutdown();

			// Places a WM_QUIT message into the queue. Which is used to terminate the Application.
			// The WM_QUIT message is read in the main Message Loop and a return value of 0 is returned from the WM_QUIT Message. 
			// The specified exit code is passed as the parameter of the WM_QUIT Message.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postquitmessage
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			// This Message is sent to a window after its size has changed.
			// URI: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
			if (wParam != SIZE_MINIMIZED)
			{
				g_applicationManagerPtr->ResizeControls(hwnd);
			}
			break;
		case WM_DROPFILES:
		{	
			// This Message is sent when the user Drops a file on the Window of an Application that has registered itself as a recipient of dropped files.
			// URI: https://learn.microsoft.com/en-us/windows/win32/shell/wm-dropfiles
			
			// The addition of more Input Data (i.e. Files and Folders) is not allowed while the Application Manager is in the Stopping State.
			if (!g_applicationManagerPtr->IsStoppingState())
			{
				// A handle to an internal structure describing the dropped files.
				HDROP hDrop = (HDROP)wParam;
				g_applicationManagerPtr->OnDrop(hDrop);
			}
			else
			{
				MessageBox(g_hwndMainApplicationWindow, L"Files and Folders cannot be added during the Stop Operation. Please try again later when the Application is ready.", L"Unsupported Action", MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
			}
			break;
		}
		case WM_COMMAND:
		{
			// The Message is sent when the user selects a Command Item from a Menu, 
			// OR when an Accelerator Keystroke is translated, 
			// OR or when a Control sends a notification Message.
			// URI: https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command

			WORD highWordMessageSource = HIWORD(wParam);
			WORD lowWordMessageSourceID = LOWORD(wParam);

			const unsigned short MENU_SOURCE_ID = 0U;
			const unsigned short ACCELERATOR_SOURCE_ID = 1U;
			
			// Handle to the Control Window. (NOTE: LPARAM is 0 if the Message Source is either a Menu or an Accelerator).
			HWND hwndControlWindow = reinterpret_cast<HWND>(lParam);
			
			if (highWordMessageSource == MENU_SOURCE_ID && hwndControlWindow == NULL)
			{
				// Handle the Menu Message Source.
				switch (lowWordMessageSourceID)
				{
					case ID_FILE_OPENFILE:
					{
						// Use the Common File Dialog to open Files.
						// URI: https://learn.microsoft.com/en-us/windows/win32/api/commdlg/nf-commdlg-getopenfilenamew
						// URI: https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamew
						g_applicationManagerPtr->OpenFileDialogBox();
						break;
					}
					case ID_FILE_OPENFOLDER:
					{	
						// Open Folder with the Shell Folder Dialog Box.
						g_applicationManagerPtr->OpenSelectFolderDialogBox();
						break;
					}
					case ID_FILE_EXIT:
						// Post Message to close the Main Application Window.
						PostMessage(hwnd, WM_CLOSE, 0, 0);
						break;
					case ID_HELP_HELP:
						// URI: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew
						ShellExecute(NULL, L"open", L"https://github.com/R3xK4xLL", NULL, NULL, SW_SHOWNORMAL);
						break;
					case ID_HELP_ABOUT:
						// Creates a Modal Dialog Box from a Dialog Box template resource.
						// The created Dialog Box does NOT return control, until the specified Callback Function terminates the Modal Dialog Box by calling the EndDialog() function.
						// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dialogboxw
						DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hwnd, AboutDialogProc);
						break;
				}
				break;
			}
			else if (highWordMessageSource == ACCELERATOR_SOURCE_ID && hwndControlWindow == NULL)
			{
				// Handle the Accelerator Message Source.
				// Accelerators currently not supported, so for now, call the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
				break;
			}
			else if (hwndControlWindow == g_applicationManagerPtr->GetButtonWindow())
			{
				// Handle the Control Message Source.
				switch (highWordMessageSource)
				{
					case BN_CLICKED:
						// The Stop Button Control was clicked.
						g_applicationManagerPtr->SetStopButtonPressedFlag(true);
						
						// Set the Flags indicating that the user has made a request to enter the Stopping State.
						g_applicationManagerPtr->EnableStoppingState();
						
						// Clear the List of Filenames that are pending for Thread-based processing.
						g_applicationManagerPtr->ClearListOfFilenames();

						HWND hwndButtonWindow = g_applicationManagerPtr->GetButtonWindow();
						EnableWindow(hwndButtonWindow, false); // Disable the Stop Button.

						PostMessage(g_hwndMainApplicationWindow, MSG_STATUS_BAR_STATIC_UPDATE, STATUS_BAR_PART_1, reinterpret_cast<LPARAM>(L"Status: Stopping"));

						wstring message(L"Message: Current processing is being Stopped. Please wait...");
						const size_t messageBufferSize = message.size() + 1;
						g_applicationManagerPtr->PostDynamicStatusBarMessage(message.c_str(), messageBufferSize);
						break;
				}
				break;
			}
			else
			{
				// Otherwise, call the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_SYSCOMMAND:
			if (wParam == ID_ABOUT)
			{
				// Creates a Modal Dialog Box from a Dialog Box template resource.
				// The created Dialog Box does NOT return control, until the specified Callback Function terminates the Modal Dialog Box by calling the EndDialog() function.
				// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dialogboxw
				DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hwnd, AboutDialogProc);
			}
			else
			{
				// Otherwise, call the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;
		case WM_COPYDATA:
		{
			// An Application sends the WM_COPYDATA Message to pass data to another Application.
			// URI: https://learn.microsoft.com/en-us/windows/win32/dataxchg/wm-copydata
			
			PCOPYDATASTRUCT pCopyDataStruct = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
			if (pCopyDataStruct->dwData == ID_ABOUT)
			{
				wchar_t* filenamePtr = static_cast<wchar_t*>(pCopyDataStruct->lpData);
				g_applicationManagerPtr->AddSupportedTypeSync(filenamePtr, true);
			}
			messageResult = true;
			break;
		}
		case WM_TIMER:
			for (unsigned long cpuID = 0; cpuID < g_applicationManagerPtr->GetMaxCPUCount(); ++cpuID)
			{	
				// Message Result is the current position of the Progress Window.
				// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-getpos
				
				HWND hwndProgressWindow = g_applicationManagerPtr->GetProgressWindow(cpuID);
				LRESULT messageResult = SendMessage(hwndProgressWindow, PBM_GETPOS, 0, 0);

				uint64_t currentDecoderProgressPercentage = g_decoderProgressTracker[cpuID];
				if (currentDecoderProgressPercentage != messageResult)
				{
					// Sets the current position for the progress bar and redraws the progress bar to reflect the new position.
					// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setpos
					SendMessage(hwndProgressWindow, PBM_SETPOS, (int)currentDecoderProgressPercentage, 0);
				}
			}
			break;
		case WM_LBUTTONDOWN:
		{
			// Handle Left Button Mouse Clicks.
			bool enableMouseButtonClickHandling = false;
			if (enableMouseButtonClickHandling)
			{
				// Handle the click here.
			}
			else
			{
				// Do not process the Message. Use the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_MBUTTONDOWN:
		{
			// Handle Middle Button Mouse Clicks.
			bool enableMouseButtonClickHandling = false;
			if (enableMouseButtonClickHandling)
			{
				// Handle the click here.
			}
			else
			{
				// Do not process the Message. Use the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_RBUTTONDOWN:
		{
			// Handle Right Button Mouse Clicks.
			bool enableMouseButtonClickHandling = false;
			if (enableMouseButtonClickHandling)
			{
				// Handle the click here.
			}
			else
			{
				// Do not process the Message. Use the default Window Procedure.
				messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;
		}
		case MSG_EDIT_WINDOW_UPDATE_TEXT:
			// This Message is sent when a request to update the Edit Window text is made.
			g_applicationManagerPtr->SetEditWindowText((wchar_t*)wParam, (wchar_t*)lParam);
			break;
		case MSG_TITLE_BAR_UPDATE:
			// This Message is sent when a request to update the Window Title Bar Percentage text is made.
			g_applicationManagerPtr->UpdateWindowTitleTextPercent(g_hwndMainApplicationWindow);
			break;
		case MSG_TASK_PROGRESS_UPDATE:
			// This Message is sent when a Thread has finished processing a File, to update the Task Progress Window.
			g_applicationManagerPtr->UpdateTaskProgressWindowPercent();
			break;
		case MSG_STATUS_BAR_STATIC_UPDATE:
		{
			// This Message is sent when a request to update the Status Bar was made with String Literals, which use compile-time read-only Memory.
			wchar_t* newStatusBarTextPtr = (wchar_t*)lParam;
			int statusBarPartID = static_cast<int>(wParam);
			g_applicationManagerPtr->SetStaticStatusBarText(statusBarPartID, newStatusBarTextPtr);
			break;
		}
		case MSG_STATUS_BAR_DYNAMIC_UPDATE:
		{
			// This Message is sent when a request to update the Status Bar was made with Strings that use Dynamically Allocated Memory.
			wchar_t* newStatusBarTextPtr = reinterpret_cast<wchar_t*>(lParam);
			int statusBarPartID = static_cast<int>(wParam);
			g_applicationManagerPtr->SetDynamicStatusBarText(statusBarPartID, newStatusBarTextPtr);
			break;
		}
		case MSG_THREAD_FILE_PROCESSED:
			// This Message is sent when a Thread has finished processing a File.
			g_applicationManagerPtr->IncrementFilesProcessed();
			break;
		case MSG_THREAD_FINISHED:
		{
			// This Message is sent when a Thread has finished processing.
			unsigned long index = static_cast<unsigned long>(wParam);

			// Sets an Individual Finished Event Object to the 'signaled state' (Available).
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
			SetEvent(g_applicationManagerPtr->GetFinishedEvent(index));
			
			// This Function call Blocks until all Finished Event Object Handles have been set to the 'signaled state' (Available).
			if (WaitForMultipleObjects(g_applicationManagerPtr->GetMaxCPUCount(), g_applicationManagerPtr->GetFinishedEventArray(), true, 0) == WAIT_OBJECT_0)
			{
				// Checks whether any background tasks are still running before generating the Final Report.
				// Also, checks whether the Stopping State has been set OR that all Background Tasks have finished. If so, then the Final Report will be generated.
				if (!g_applicationManagerPtr->IsRunningState())
				{
					// After all Threads have finished Processing, generate the Final Report and display it in the Edit Window.
					g_applicationManagerPtr->GenerateFinalReport();

					HWND hwndButtonWindow = g_applicationManagerPtr->GetButtonWindow();
					EnableWindow(hwndButtonWindow, false); // Disable the Stop Button.

					SetWindowText(g_hwndMainApplicationWindow, WINDOW_TITLE); // Reset the Window Title Bar text to default.

					PostMessage(g_hwndMainApplicationWindow, MSG_STATUS_BAR_STATIC_UPDATE, STATUS_BAR_PART_1, reinterpret_cast<LPARAM>(L"Status: Ready")); // Reset the Status Bar Message.

					// Reset the Task Progress Window only if the Stop Button was pressed, otherwise leave it displaying the completed percentage.
					if (g_applicationManagerPtr->StopButtonPressedFlagEnabled())
					{
						HWND hwndTaskProgressWindow = g_applicationManagerPtr->GetTaskProgressWindow();
						SendMessage(hwndTaskProgressWindow, PBM_SETPOS, 0, 0); // Reset the Task Progress Window.
						g_applicationManagerPtr->SetStopButtonPressedFlag(false); // Reset the Button pressed Flag.

						// Reset the Stopping State. 
						g_applicationManagerPtr->SetStoppingStateFlagEnabled(false);
					}
				}

			}
			break;
		}
		case MSG_CMDLINE:
			if (reinterpret_cast<wchar_t*>(wParam) != nullptr)
			{
				// Add the filename passed as a parameter in the Message.
				g_applicationManagerPtr->AddSupportedTypeSync(reinterpret_cast<wchar_t*>(wParam), true);
				delete[](wchar_t*)wParam; // Delete the filename.
			}
			break;
		default:
			// Call the default window procedure to provide default processing for any window messages that this Application does not process.
			// The DefWindowProc() function performs the default action for the message, which varies by message type. 
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-defwindowproca
			messageResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return messageResult;
}

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT messageResult = 0;

	// A typical window procedure is simply a large switch statement that switches on the message code. 
	// Add cases for each message that you want to handle.
	switch (uMsg)
	{
		case WM_KEYDOWN:
		{	
			// Check if the Control Key is pressed.
			if (GetKeyState(VK_CONTROL) & 0x8000) {

				// Check if the 'A' key is pressed.
				if (wParam == 'A') {
					// Select ALL of the text in the Edit Control by sending the EM_SETSEL message to the Edit Control.
					SendMessage(hwnd, EM_SETSEL, 0, -1);
				}

				// Check if the 'C' OR 'X' key is pressed.
				if ((wParam == 'C') || (wParam == 'X')) {
					// The selected text (possibly All of the text if 'A' key was pressed too) in the Edit Control is copied to the Clipboard.
					// Sending the WM_COPY Message to the Edit Control, copies the current selection to the clipboard in CF_TEXT format.
					SendMessage(hwnd, WM_COPY, NULL, NULL);
				}
			}
			break;
		}
		case WM_CONTEXTMENU:
		{
			bool enableRightClickContextMenu = true;
			if (enableRightClickContextMenu)
			{
				// Enable the right-click Context Menu, by simply passing along the Message.
				WNDPROC wndprocEditProcedure = g_applicationManagerPtr->GetEditProcedure();
				messageResult = CallWindowProc(wndprocEditProcedure, hwnd, uMsg, wParam, lParam);
			}
			else
			{
				// Disable the right-click Context Menu.
				messageResult = 0;
			}
			break;
		}
		default:
		{
			// Use the CallWindowProc() function for window subclassing. Usually, all windows with the same class share one window procedure. 
			// A subclass is a window or set of windows with the same class whose messages are intercepted and processed by another 
			// window procedure (or procedures) before being passed to the window procedure of the class.
			// 
			// The SetWindowLong() function creates the subclass by changing the window procedure associated with a particular window, 
			// causing the system to call the new window procedure instead of the previous one.
			// 
			// An application must pass any messages not processed by the new window procedure to the previous window procedure by calling CallWindowProc. 
			// This allows the application to create a chain of window procedures.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callwindowprocw
			WNDPROC wndprocEditProcedure = g_applicationManagerPtr->GetEditProcedure();
			messageResult = CallWindowProc(wndprocEditProcedure, hwnd, uMsg, wParam, lParam);
			break;
		}
	}

	return messageResult;
}

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// This Message is sent when the Dialog Box Procedure performs initialization.
			// This is the first Message the Dialog Box Procedure receives.
			
			// Center the Dialog Box Window within the Main Application Window.
			
			// Calculate the center position of Main Application Window.
			RECT rectStructMainApplicationWindow;
			GetWindowRect(g_hwndMainApplicationWindow, &rectStructMainApplicationWindow);
			int centerX = (rectStructMainApplicationWindow.left + rectStructMainApplicationWindow.right) / 2;
			int centerY = (rectStructMainApplicationWindow.top + rectStructMainApplicationWindow.bottom) / 2;
			
			// Calculate the width and height of the Dialog Box Window.
			RECT rectStructDialogBoxWindow;
			GetWindowRect(hDlg, &rectStructDialogBoxWindow);
			int dialogWidth = rectStructDialogBoxWindow.right - rectStructDialogBoxWindow.left; // Pixels
			int dialogHeight = rectStructDialogBoxWindow.bottom - rectStructDialogBoxWindow.top; // Pixels

			// Calculate the Client Cooordinates for centering the Dialog Box Window.
			int leftWindowPosition = centerX - (dialogWidth / 2); // Client Coordinate
			int topWindowPosition = centerY - (dialogHeight / 2); // Client Coordinate

			// Set the Dialog Box Window to the center position.
			SetWindowPos(hDlg, NULL, leftWindowPosition, topWindowPosition, dialogWidth, dialogHeight, SWP_NOZORDER | SWP_NOSIZE);

			return TRUE;
		}
		case WM_COMMAND:
		{
			// This is the Message the Button Control sends when either the Button Control is clicked with the mouse 
			// OR when the Spacebar is pressed while the Button Control has the input focus.
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					// Fall-Through to EndDialog() function below.
				}
				case IDCANCEL:
				{
					EndDialog(hDlg, 0); // Destroy the Dialog Box.
					return TRUE;
				}
				break;
			}
		}
		case WM_NOTIFY:
		{
			if (LOWORD(wParam) == IDC_SYSLINK_ABOUT_DIALOG_GITHUB)
			{
				// A notification Message was sent from the SysLink Control.
				
				// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-nmhdr
				LPNMHDR notificationMessageStructPtr = reinterpret_cast<LPNMHDR>(lParam);
				switch (notificationMessageStructPtr->code)
				{
					case NM_CLICK:
						// Sent when the user clicks the SysLink Control with the left mouse button.
						// Fall-Through to the next case below.
					case NM_RETURN:
						// Sent when the SysLink Control has the input focus and the user has pressed the ENTER key.
						// Handle the Hyperlink click (Open the URI).
						// URI: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew
						ShellExecute(NULL, L"open", L"https://github.com/R3xK4xLL", NULL, NULL, SW_SHOWNORMAL);
						break;
				}
			}
			break;
		}
	}

	// For all other Messages, the Dialog Box Procedure did NOT process the Message.
	// The Dialog Manager performs the default dialog operation in response to returning FALSE.
	return FALSE;
}

UINT_PTR CALLBACK Lpofnhookproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// The Dialog Window Hook Procedure returns FALSE to pass a message to the default Dialog Box procedure or TRUE to discard the message.
	UINT_PTR messageResult = FALSE;

	// A typical Hook Procedure is simply a large switch statement that switches on the message code.
	// Add cases for each message that you want to handle.
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Message is sent after the parent Dialog Window finishes processing its own WM_INITDIALOG message.
			// Using Window Properties provides a good thread-safe solution and alternative to using a global variable.

			// A Pointer to the OPENFILENAME structure used to initialize the Dialog Box.
			LPOPENFILENAME lpOFN = reinterpret_cast<LPOPENFILENAME>(lParam);

			// Use a Window Property to store the OPENFILENAME structure for usage throughout this Dialog Window Hook Procedure.
			// NOTE: Using Window Properties provides a good thread-safe solution and alternative to using a global variable.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setpropw
			SetProp(GetParent(hwnd), L"OFN", (void*)lpOFN);
			
			messageResult = TRUE; // Return TRUE to direct the System to set the keyboard focus to the control specified by wParam.
			break;
		}
		case WM_NOTIFY:
		{
			// This Message is sent when actions are taken by the user in the Dialog Box.
			
			// The lParam parameter for each WM_NOTIFY message is a pointer (LPOFNOTIFY) to an OFNOTIFY structure.
			LPOFNOTIFY lpOFNotify = reinterpret_cast<LPOFNOTIFY>(lParam);
			switch (lpOFNotify->hdr.code)
			{
				case CDN_SELCHANGE:
				{
					// In CDN_SELCHANGE, check to see if the Buffer presently allocated is large enough to handle all the files selected. 
					// Reallocate the Buffer if neccessary and free the previous allocated Buffer to prevent memory leaks.

					// The OFN struct is stored in a Property of the Dialog Window.
					// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getpropw
					HANDLE handleProperty = GetProp(GetParent(hwnd), L"OFN");
					LPOPENFILENAME lpOFN = static_cast<LPOPENFILENAME>(handleProperty);

					// Retrieves the file name (not including the Path) of the currently selected file in an Explorer-style Dialog Box.
					// If the message succeeds, the return value is the size, in characters/bytes, of the file name string, including the terminating NULL character '\0'.
					int returnValue = CommDlg_OpenSave_GetSpec(GetParent(hwnd), 0, NULL);
					
					if (returnValue >= 0)
					{
						// Determine the required Buffer size for the currently selected file(s).
						DWORD requiredBufferSize = static_cast<DWORD>(returnValue);
						requiredBufferSize += MAX_PATH;
						
						if (lpOFN->nMaxFile < requiredBufferSize)
						{
							// The previously allocated memory is not large enough for the currently selected file(s). 
							
							if (lpOFN->lpstrFile != nullptr)
							{
								// Free the previously allocated memory.
								delete[] lpOFN->lpstrFile;
								lpOFN->lpstrFile = nullptr;
							}

							// Get the maximum value that can be stored in a DWORD.
							// Surround the max() function call with extra parenthesis like so '(std::numeric_limits<T>::max)()'.
							// This is a workaround for the WinAPI and the minwindef.h MACRO definitions for 'max'/ 'min' causing a name conflict with this Standard Library function.
							// This solution allows for those Windows Macros definitions to coexist without having to disable them via a #define NOMINMAX Macro, before including the minwindef.h file.
							constexpr DWORD MAX_DWORD = (std::numeric_limits<DWORD>::max)();

							// Double the required Buffer size to reduce the number of memory allocation and memory cleanup operations.
							// (NOTE: The use of a static cast to an 'unsigned long long' is to prevent a multiplication overflow.
							// This approach will allocate a Buffer size up to the limit of the maximum value that can be stored in a DWORD (4,294,967,295 Characters / '16-bit Unicode Characters').
							// A maximum Buffer size of 8,589,934,590 bytes of storage is supported. (e.g. 4,294,967,295 * 2 bytes 'wchar_'t' = 8,589,934,590 bytes of storage)
							requiredBufferSize = (static_cast<unsigned long long>(requiredBufferSize) * 2 < MAX_DWORD) ? requiredBufferSize * 2 : MAX_DWORD;
							
							// Create a new dynamically allocated Buffer using the required Buffer size.
							wchar_t* szFileNamePtr = new wchar_t[requiredBufferSize] {};

							// Adjust the size of the Buffer used in the OPENFILENAME structure.
							lpOFN->lpstrFile = szFileNamePtr; // Buffer to store the selected File Path.
							lpOFN->nMaxFile = requiredBufferSize; // The size in characters/bytes of the Buffer pointed to by lpstrFile.
						}
					}
					else
					{
						// An error occurred. For now, do nothing.
					}

					messageResult = FALSE; // The return value is ignored by the Caller.
					break;
				}
			}

			break;
		}
		case WM_DESTROY:
		{
			// Free the Window Property associated with the OPENFILENAME structure.
			// URI: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-removepropw
			RemoveProp(GetParent(hwnd), L"OFN");
			messageResult = FALSE;
			break;
		}
		default:
			messageResult = FALSE;
	}

	return messageResult;
}

int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	int messageResult = 0;

	switch (uMsg)
	{
		case BFFM_INITIALIZED:
			// Sets the initial folder/location using the path specified in LPARAM lpData.
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);

			// Sets the text that is displayed on the Dialog Box's OK button.
			SendMessage(hwnd, BFFM_SETOKTEXT, NULL, reinterpret_cast<LPARAM>(L"Open"));

			// Center the Dialog Box Window within the Main Application Window.

			// Calculate the center position of Main Application Window.
			RECT rectStructMainApplicationWindow;
			GetWindowRect(g_hwndMainApplicationWindow, &rectStructMainApplicationWindow);
			int centerX = (rectStructMainApplicationWindow.left + rectStructMainApplicationWindow.right) / 2;
			int centerY = (rectStructMainApplicationWindow.top + rectStructMainApplicationWindow.bottom) / 2;

			// Calculate the width and height of the Dialog Box Window.
			RECT rectStructDialogBoxWindow;
			GetWindowRect(hwnd, &rectStructDialogBoxWindow);
			int dialogWidth = rectStructDialogBoxWindow.right - rectStructDialogBoxWindow.left; // Pixels
			int dialogHeight = rectStructDialogBoxWindow.bottom - rectStructDialogBoxWindow.top; // Pixels

			// Calculate the Client Cooordinates for centering the Dialog Box Window.
			int leftWindowPosition = centerX - (dialogWidth / 4) - (dialogHeight / 4); // Client Coordinate
			int topWindowPosition = centerY - (dialogHeight / 4) - (dialogHeight / 4); // Client Coordinate

			// Set the Dialog Box Window to the center position.
			SetWindowPos(hwnd, NULL, leftWindowPosition, topWindowPosition, dialogWidth, dialogHeight, SWP_NOZORDER | SWP_NOSIZE);

			break;
	}
	
	return messageResult;
}

DWORD WINAPI DecoderThreadProc(LPVOID lpParameter)
{
	// Each Thread receives a unique copy of the local variables of this function.
	// Any static or global variables are shared by all Threads in the Process.
	
	// A Converted Generic Pointer Data Type.
	// (On a 32-bit Windows OS LPVOID is 32-bits wide; On a 64-bit Windows OS LPVOID is 64-bits wide)
	uint64_t progressTrackerIndex = reinterpret_cast<uint64_t>(lpParameter);
	
	g_decoderProgressTracker[progressTrackerIndex] = 0ULL;

	wstring filename{};
	wchar_t* filenameMessagePtr = nullptr;
	wchar_t* errorMessagePtr = nullptr;

	// Stores the total number of units from the audio file that have been decoded.
	uint64_t totalAudioUnitsRead = 0ULL;;

	int64_t currentAudioUnitSizeRead = 0LL;
	uint64_t decodedAudioDataTotal = 0ULL;
	
	const int waitTimeOutInterval = 0;
	const int numberOfEventHandleObjects = 2;
	
	// The index value used to determine whether the main Thread Loop is terminated.
	static const DWORD THREAD_TERMINATION_INDEX_VALUE = 0;

	// The index value used to determine whether the main Thread Loop can start/continue Looping.
	static const DWORD THREAD_CONTINUATION_INDEX_VALUE = 1;

	// An array of Object Handles for use with the synchronization Wait Function.
	HANDLE handleEventObjectsThreadState[numberOfEventHandleObjects] = { 0 };
	handleEventObjectsThreadState[THREAD_TERMINATION_INDEX_VALUE] = g_applicationManagerPtr->GetTerminateEvent(); // Event Object (Thread End)
	handleEventObjectsThreadState[THREAD_CONTINUATION_INDEX_VALUE] = g_applicationManagerPtr->GetPendingEvent(); // Event Object (Thread Start/Continue)
	
	// When an Event Object is in the 'signaled state' (Available), the wait function returns and the thread may continue executing. 
	// If an Event Object is in the 'not-signaled state' (Owned), the Thread blocks until the Event Object is put into the 'signaled state'.

	// Waits until one or all of the specified objects are in the 'signaled state' (Available) or the time-out interval elapses. (synchapi.h)
	// Since the argument 'bWaitAll = false', the function returns when the state of any one of the objects is set to signaled (Available). 
	// Therefore, the return value indicates the Handle Event Object Array Index, whose state caused the function to return.
	// If all of the Event Objects are set to the 'signaled state', the lowest index value in the Handle array is returned. 
	// In the current implementation, the Terminate Event Object Index is returned when all of the Event Objects are signaled.
	// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
	// 
	// This Function call blocks until at least one of the Event Objects is set to the 'signaled state' (Available).
	DWORD waitEventMessageHandleIndex = WaitForMultipleObjects(numberOfEventHandleObjects, handleEventObjectsThreadState, false, INFINITE);
	
	// Purpose: The main Thread Loop.
	// The main Thread Loop runs each time a Pending Event Object is set to the 'signaled state' (Available).
	// The main Thread Loop terminates when Terminate Event Object is set to the 'signaled state' (Available).
	while (waitEventMessageHandleIndex != THREAD_TERMINATION_INDEX_VALUE && waitEventMessageHandleIndex == THREAD_CONTINUATION_INDEX_VALUE)
	{
		// Sets the specified Finished Event Object to the 'non-signaled state' (Owned).
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent
		ResetEvent(g_applicationManagerPtr->GetFinishedEvent(static_cast<unsigned long>(progressTrackerIndex)));
		
		// Purpose: A Thread Loop to Process each available File.
		// The Thread Loop terminates when the Terminate Event Object is set to the 'signaled state' (Available).
		// OR
		// The Thread Loop terminates when no more Files are available.
		// 
		// Waits until the specified object (Terminate Event Object) is in the 'signaled state' (Available) or the time-out interval elapses.
		// The time-out interval is set to 0, therefore the function does not enter a Wait State if the object is 'not-signaled' (Owned); it always returns immediately.
		// Therefore, this function call does NOT Block.
		// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
		while ((WaitForSingleObject(g_applicationManagerPtr->GetTerminateEvent(), waitTimeOutInterval) != WAIT_OBJECT_0) && (g_applicationManagerPtr->GetNextAvailableFilename(filename, g_applicationManagerPtr->GetPendingEvent())))
		{
			const size_t bufferSize = filename.size() + 1;
			filenameMessagePtr = new wchar_t[bufferSize]{}; // Create on the Heap. This will be cleaned up later by the recipient of the MSG_EDIT_WINDOW_UPDATE_TEXT Message. 
			wcscpy_s(filenameMessagePtr, bufferSize, filename.c_str());
			
			// Request ownership of the Critical Section.
			EnterCriticalSection(&g_criticalSection);

			// Serialize Thread access to reading Files from the Disk.
			// By placing the OpenDecoder() operation within the Critical Section, it ensures that only 1-Thread at a time can read a File from the Disk.
			// This optimization significantly improves Disk I/O performance and overall program speed when processing Files on the Disk.
			// Open a new Decoder using the file.
			unique_ptr<PureAbstractBaseDecoder> decoderSmartPtr = g_applicationManagerPtr->OpenDecoder(filename.c_str());

			// Release ownership of the Critical Section.
			LeaveCriticalSection(&g_criticalSection);

			if (decoderSmartPtr != nullptr)
			{
				// A Decoder was opened for the File.
				// Configure and update the Progress Bar.
				decodedAudioDataTotal = decoderSmartPtr->GetDecodedAudioDataTotal();
				if (decodedAudioDataTotal == 0ULL)
				{
					decodedAudioDataTotal = 0xFFFFFF; // Default Audio Data Total = 16,777,215 (2^24)
				}

				HWND hwndProgressBarWindow = g_applicationManagerPtr->GetProgressWindow(static_cast<unsigned long>(progressTrackerIndex));
				int progressBarLowMinimum = 0;
				int progressBarHighMaximum = (int)(decodedAudioDataTotal >> 8LL);

				// URI: https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setrange32
				PostMessage(hwndProgressBarWindow, PBM_SETRANGE32, progressBarLowMinimum, progressBarHighMaximum);

				totalAudioUnitsRead = 0ULL;
				currentAudioUnitSizeRead = decoderSmartPtr->Read(); // Initialize the loop control variable.
				
				// Purpose: A Thread Loop to Read File Data.
				// The Thread Loop terminates when the Terminate Event Object is set to the 'signaled state' (Available).
				// OR
				// The Thread Loop terminates when the EOF value is encountered.
				// 
				// Waits until the specified object (Terminate Event Object) is in the 'signaled state' (Available) or the time-out interval elapses.
				// The time-out interval is set to 0, therefore the function does not enter a Wait State, if the Event Object is 'not-signaled' (Owned); it always returns immediately.
				// Therefore, this function call does NOT Block.
				// URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
				while ((WaitForSingleObject(g_applicationManagerPtr->GetTerminateEvent(), waitTimeOutInterval) != WAIT_OBJECT_0) && (currentAudioUnitSizeRead > 0LL))
				{
					// Algorithm: Update the Progress Completed percentage.
					// 
					// General Explanation: 
					// By bit-shifting the totalAudioUnitsRead variable right by 8 positions, the code divides the totalAudioUnitsRead decoded by 256 (i.e. 2^8).
					// Which is equivalent to calculating the percentage of the task that has been completed.
					// By dividing totalAudioUnitsRead by 256, the value stored in progressTracker[progressTrackerIndex] represents the percentage 
					// of the audio file that has been decoded in increments of 0.39%.
					// 
					// Bit-shifting Explanation: Bit-shifting a value to the right by 8 positions is equivalent to dividing it by 256 (2^8) (i.e. A power of 2). 
					// Since 100% is equivalent to the total amount, dividing the current total amount by 256 will give us the percentage of the final total amount in increments of 0.39%.
					// 
					// This technique uses division by a power of 2. 
					// This technique is often used in embedded systems and other performance-critical applications where efficiency is a top priority. 
					// By using bit-shifting instead of division, the calculation can be performed much faster, since bit-shifting is a simple bitwise operation 
					// that can be executed very quickly by the processor.
					totalAudioUnitsRead += currentAudioUnitSizeRead;

					// Caution: Global Shared State is being updated through this Array (Critical Section). 
					// The Array index being assumed unique per Thread, is the only safety mechanism.
					// An Explicit Lock is ommitted for maximum performance.
					g_decoderProgressTracker[progressTrackerIndex] = totalAudioUnitsRead >> 8ULL;
					
					currentAudioUnitSizeRead = decoderSmartPtr->Read(); // Update the loop control variable.
				}
				
				if (currentAudioUnitSizeRead < 0LL)
				{
					// A decoding read error occurred. (i.e. Audio Unit Size == -1)
					
					// Create an Error Message (on the Heap). 
					// This Error Message will be cleaned up later by the recipient of the MSG_EDIT_WINDOW_UPDATE_TEXT Message.
					
					const wchar_t* lastErrorPtr = decoderSmartPtr->GetLastErrorMessage();
					const size_t bufferSize = wcslen(lastErrorPtr) + 1;
					errorMessagePtr = new wchar_t[bufferSize]{};
					wcscpy_s(errorMessagePtr, bufferSize, lastErrorPtr); // Copy to the Heap.
				}
				else 
				{
					// No error occurred.
					errorMessagePtr = nullptr;
				}
			}
			else
			{
				// A Decoder was NOT opened for the File. 
				// Create an Error Message (on the Heap).
				// This Error Message will be cleaned up later by the recipient of the MSG_EDIT_WINDOW_UPDATE_TEXT Message.

				const size_t bufferSize = wcslen(STR_FILE_ERROR) + 1;
				errorMessagePtr = new wchar_t[bufferSize]{};
				wcscpy_s(errorMessagePtr, bufferSize, STR_FILE_ERROR); // Copy to the Heap.
			}
			
			// Send a Message to update the Files Processed Count.
			PostMessage(g_hwndMainApplicationWindow, MSG_THREAD_FILE_PROCESSED, 0, 0);

			// Send a Message to update the Edit Window text.
			PostMessage(g_hwndMainApplicationWindow, MSG_EDIT_WINDOW_UPDATE_TEXT, reinterpret_cast<WPARAM>(filenameMessagePtr), reinterpret_cast<LPARAM>(errorMessagePtr));

			// Send a Message to update the Window Title Bar Percentage text.
			PostMessage(g_hwndMainApplicationWindow, MSG_TITLE_BAR_UPDATE, 0, 0);
			
			// Send a Message to update the Task Progress Window.
			PostMessage(g_hwndMainApplicationWindow, MSG_TASK_PROGRESS_UPDATE, 0, 0);
		}

		g_decoderProgressTracker[progressTrackerIndex] = 0ULL;

		#if defined(_WIN64)
		// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.
		// Code for 64-bit OS.
		WPARAM wParam = progressTrackerIndex; // On a 64-bit Windows OS, WPARAM is a 64-bit wide unsigned integer.

		#elif defined(_WIN32)
		// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.
		// Code for 32-bit OS.
		WPARAM wParam = static_cast<WPARAM>(progressTrackerIndex); // On a 32-bit Windows OS, WPARAM is a 32-bit wide unsigned integer.
		#endif

		// Post a Message Asynchronously that sets the Finished Event Object to the 'signaled state' (Available).
		PostMessage(g_hwndMainApplicationWindow, MSG_THREAD_FINISHED, wParam, 0);
		
		// This Function call blocks until at least one of the Event Objects is set to the 'signaled state' (Available).
		waitEventMessageHandleIndex = WaitForMultipleObjects(numberOfEventHandleObjects, handleEventObjectsThreadState, false, INFINITE);
	}

	// Return Thread Exit Code.
	return 0;
}

#pragma endregion WinAPI_Functions_Region
