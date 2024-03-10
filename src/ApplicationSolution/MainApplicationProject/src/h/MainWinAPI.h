#ifndef MAIN_WIN_API_H
#define MAIN_WIN_API_H

// Preprocessor directives

#pragma region WINDOW_TITLE_ADJUSTMENT_FOR_COMPILATION_TARGET
#if defined(_WIN64)
// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

#define WINDOW_TITLE				    TEXT("WinAudioDecodeR [x64] [64-bit]")

#elif defined(_WIN32)
// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

#define WINDOW_TITLE				    TEXT("WinAudioDecodeR [x86] [32-bit]")

#endif
#pragma endregion WINDOW_TITLE_ADJUSTMENT_FOR_COMPILATION_TARGET

#define ABOUT_WINDOW_TITLE              TEXT("WinAudioDecodeR")
#define STR_ABOUTMENU				    TEXT("About WinAudioDecodeR...")

#define STR_START_EDIT_WINDOW		    TEXT("Drop files/folders here")
#define STR_FILE_ERROR				    TEXT("UNABLE_TO_OPEN_DECODER")
#define STR_BUTTON_TEXT				    TEXT("Stop")
#define STR_RESULT					    TEXT("%d %s scanned in %.2f seconds")
#define STR_FILE					    TEXT("file")
#define STR_FILES					    TEXT("files")
#define STR_ERROR					    TEXT("failed")
#define STR_PASS					    TEXT("passed")
#define STR_OK						    TEXT("OK")
#define STR_MESSAGE_BOX_CLOSE		    TEXT("Are you sure you want to close the Window?")

#define WINDOW_PADDING				    12
#define BUTTON_WIDTH				    80
#define BUTTON_HEIGHT				    24

#define MSG_EDIT_WINDOW_UPDATE_TEXT     WM_USER + 1
#define MSG_TITLE_BAR_UPDATE            WM_USER + 2
#define MSG_TASK_PROGRESS_UPDATE        WM_USER + 3
#define MSG_STATUS_BAR_STATIC_UPDATE	WM_USER + 4
#define MSG_STATUS_BAR_DYNAMIC_UPDATE	WM_USER + 5
#define MSG_THREAD_FILE_PROCESSED       WM_USER + 6
#define MSG_THREAD_FINISHED				WM_USER + 7
#define MSG_CMDLINE					    WM_USER + 8

#define ID_ABOUT					    1974
#define TIMER						    20
#define TIMER_ID                        42
#define SHELL_MUSIC					    237

#define STATUS_BAR_PART_1               0
#define STATUS_BAR_PART_2               1
#define STATUS_BAR_PART_3               2 


#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h> // WinAPI
#include <CommCtrl.h> // Interface for the Windows Common Controls (WinAPI)
#include <shlwapi.h> // The Shell Lightweight API. URI: https://learn.microsoft.com/en-us/windows/win32/shell/shlwapi
#include <shlobj.h>
#include "resource.h"
#include "WinAPIUtils.h"

// Declare and initialize Global Variables for use with WinAPI Program.

#pragma region MAX_CPU_ADJUSTMENT_FOR_COMPILATION_TARGET
#if defined(_WIN64)
// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

constexpr unsigned long MAX_CPU = 64UL;

#elif defined(_WIN32)
// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

constexpr unsigned long MAX_CPU = 32UL;

#endif
#pragma endregion MAX_CPU_ADJUSTMENT_FOR_COMPILATION_TARGET

// The maximum default size for C-style strings allowed.
constexpr unsigned long MAX_TEXT_SIZE = 256UL;

// Declare WinAPI Function Prototypes

/// <summary>
/// Purpose: Every Windows program includes an entry-point function named either WinMain() or wWinMain().
/// See <a href="https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point">link</a> for more information.
/// </summary>
/// <param name="hInstance">
/// Is the handle to an instance or handle to a module. The operating system uses 
///	this value to identify the executable or EXE when it's loaded in memory. Certain Windows functions 
///	need the instance handle, for example to load icons or bitmaps.
/// </param>
/// <param name="hPrevInstance">Has no meaning. It was used in 16-bit Windows, but is now always zero (NULL) for Win32/WinAPI programs.</param>
/// <param name="lpCmdLine">Contains the command-line arguments as a single ANSI string, NOT including the Program Name.</param>
/// <param name="nShowCmd">Is a flag that indicates whether the main application window is minimized, maximized, or shown normally.</param>
/// <returns>The function returns an int value. The operating system doesn't use the return value, but you can use 
/// the value to pass a status code to another program.
/// </returns>
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);

/// <summary>
/// Purpose: The WinAPI DispatchMessage function calls the Window Procedure of the window 
/// that is the target of the message. CALLBACK is the calling convention for the function.
/// 
/// Additional data for the message is contained in the lParam and wParam parameters. 
/// Both parameters are integer values the size of a pointer width (32 bits or 64 bits).
/// Usually the data is either a numeric value or a pointer to a structure. Some messages do not have any data.
/// 
/// See <a href="https://learn.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure">link</a> for more information.
/// </summary>
/// <param name="hwnd">Is a handle to the window that is the target of the Message.</param>
/// <param name="uMsg">Is the message code; for example, the WM_SIZE message indicates the window was resized.</param>
/// <param name="wParam">Contains additional data that pertains to the message. The exact meaning depends on the message code.</param>
/// <param name="lParam">Contains additional data that pertains to the message. The exact meaning depends on the message code.</param>
/// <returns> An integer value (LRESULT) that your program returns to Windows. It contains your program's response to a particular message. 
/// The meaning of this value depends on the message code.</returns>
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Purpose: This is a user-defined Window Procedure Function that handles Messages sent to an Edit Control. 
/// </summary>
/// <param name="hwnd">Is a handle to the window that is the target of the Message.</param>
/// <param name="uMsg">Is the message code; for example, the WM_KEYDOWN message indicates a nonsystem key was pressed.</param>
/// <param name="wParam">Contains additional data that pertains to the message. The exact meaning depends on the message code.</param>
/// <param name="lParam">Contains additional data that pertains to the message. The exact meaning depends on the message code.</param>
/// <returns></returns>
LRESULT CALLBACK EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Purpose:  This is a user-defined Dialog Box Procedure Function that handles Messages sent to an About Dialog Box. 
/// </summary>
/// <param name="hDlg">A Handle to the Dialog Box.</param>
/// <param name="uMsg">The Message.</param>
/// <param name="wParam">Contains the ID of the Control that sent the Message.</param>
/// <param name="lParam"></param>
/// <returns>Returns TRUE if it processes the Message. Otherwise, returns FALSE if it does NOT process the Message.</returns>
INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Purpose: This is a user-defined Dialog Window Hook Procedure used with Explorer-style Open and Save As Dialog Boxes.
/// This Hook Procedure allows for the dynamic allocation of memory whenever the capacity of the selected filename Buffer is exceeded.
/// This allows the Dialog Box to support the selection of a large number of files and the filename Buffer will be adjusted as necessary
/// during runtime to accomodate the selected files. This process is seemless to the end-user and happens in the background.
/// 
/// NOTE: If you provide a Hook Procedure for an Explorer-style Dialog Box, the default Dialog Box Procedure 
/// creates a child Dialog Box when the default Dialog Procedure is processing its WM_INITDIALOG message. 
/// This Hook Procedure acts as the Dialog Procedure for that child Dialog Box.
/// </summary>
/// <param name="hwnd"></param>
/// <param name="uMsg"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns></returns>
UINT_PTR CALLBACK Lpofnhookproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Purpose: An Application-defined callback function used to send messages to, and process messages from, a Browse Dialog Box displayed in response to a call to SHBrowseForFolder().
/// </summary>
/// <param name="hwnd"></param>
/// <param name="uMsg"></param>
/// <param name="lParam"></param>
/// <param name="lpData"></param>
/// <returns></returns>
int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

/// <summary>
/// Purpose: The main Thread Procedure for Decoder-based processing of Files. A Thread Procedure is an Application-defined function that serves as the starting address for a Thread. 
/// 
/// See <a href="https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms686736(v=vs.85)">link</a> for more information.
/// </summary>
/// <param name="lpParameter">The Thread data passed to the function.
/// </param>
/// <returns>The return value indicates the success or failure of this function.</returns>
DWORD WINAPI DecoderThreadProc(LPVOID lpParameter);

#endif // MAIN_WIN_API_H