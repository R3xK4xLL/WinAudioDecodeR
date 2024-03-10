#ifndef EXECUTION_MANAGER_H
#define EXECUTION_MANAGER_H

#include "MainWinAPI.h"
#include <queue>
#include <functional>
#include <string>

/// <summary>
/// Purpose: This namespace contains the Main Application elements, other than the WinAPI elements. 
/// The existence of this namespace helps to reduce unneccessary global variables.
/// </summary>
namespace MainApplication {

    /// <summary>
    /// Purpose: An Execution Manager that supports Asynchronous Thread-based background processing.
    /// </summary>
    class ExecutionManager
    {
        public:
            /// <summary>
            /// Purpose: The with-args Constructor for the Execution Manager.
            /// </summary>
            /// <param name="lpCriticalSection">A pointer to the Critical Section Object used by the current Process.</param>
            ExecutionManager(const LPCRITICAL_SECTION lpCriticalSection);
            virtual ~ExecutionManager();
            ExecutionManager(const ExecutionManager& other) = delete; // Delete Copy Constructor
            ExecutionManager& operator=(const ExecutionManager& other) = delete; // Delete Assignment Operator (Overloaded)
            ExecutionManager(ExecutionManager&& other) noexcept = delete; // Delete The Move Constructor
            ExecutionManager& operator=(ExecutionManager&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

            /// <summary>
            /// Purpose: Adds the specified folder name to the Queue and initiates background Asynchronous Thread-based processing.
            /// Uses the Critical Section to perform this operation.
            /// </summary>
            /// <param name="folderNamePtr"></param>
            /// <param name="setPendingEventFlag">A flag used for setting the Pending Event, to initiate/continue Thread-based processing with the Execution Manager.</param>
            void AddFolderToQueue(const wchar_t* folderNamePtr, bool setPendingEventFlag);
            
            /// <summary>
            /// Purpose: Sets a bound Callback Function, that can be used by the Execution Manager during Thread-based processing.
            /// </summary>
            /// <param name="boundCallback"></param>
            void SetBoundCallback(std::function<void(const wchar_t*)> boundCallback);

            /// <summary>
            /// Purpose: The purpose of this function is to set the Pending Event, which indicates that there are Files/Folders pending for Thread-based Processing.
            /// </summary>
            void SetPendingEvent();

            /// <summary>
            /// Purpose: Clears the Queue of folder names used by the Execution Manager for background Asynchronous Thread-based processing. 
            /// Uses the Critical Section to perform this operation.
            /// </summary>
            void ClearFolderQueue();

            /// <summary>
            /// Purpose: Checks whether Queue of folder names is empty. Uses the Critical Section to perform this operation.
            /// </summary>
            /// <returns></returns>
            bool IsFolderQueueEmpty();

            /// <summary>
            /// Purpose: Adds file name to the list of processed file names.
            /// </summary>
            /// <param name="filenamePtr"></param>
            void AddToProcessedFileList(const wchar_t* filenamePtr);

            /// <summary>
            /// Purpose: Clears the list of processed file names.
            /// </summary>
            void ClearProcessedFileList();

            /// <summary>
            /// Purpose: Gets a Reference to the File List.
            /// </summary>
            /// <returns>A Reference to the File List.</returns>
            std::list<std::wstring>& GetProcessedFileListRef();
            
            #pragma region Getter_Setter_Functions_Region

            /// <summary>
            /// Purpose: Returns whether the Execution Manager is in a Running State (TRUE) OR in a Waiting State (FALSE).
            /// </summary>
            /// <returns></returns>
            bool IsRunningStateFlagEnabled() const;
            void SetRunningStateFlagEnabled(bool value);
            bool IsStoppingStateFlagEnabled() const;
            void SetStoppingStateFlagEnabled(bool value);

            #pragma endregion Getter_Setter_Functions_Region

        private:
            LPCRITICAL_SECTION criticalSectionPtr{};

            /// <summary>
            /// Purpose: Supports the queuing of Folder names for processing.
            /// URI: https://cplusplus.com/reference/queue/queue/
            /// </summary>
            std::queue<std::wstring> folderQueue{};

            /// <summary>
            /// Purpose: A List used to store the results of Folder processing.
            /// </summary>
            std::list<std::wstring> processedFileList{};

            DWORD folderThreadId{};
            HANDLE handleFolderThread{};

            HANDLE handleTerminateEvent{};
            HANDLE handlePendingEvent{};
            HANDLE handleFinishedEvent{};

            /// <summary>
            /// Purpose: A Flag used to indicated if the Execution Manager is in a Running State (TRUE) or a Waiting State (FALSE).
            /// </summary>
            bool runningStateFlagEnabled{ false };

            /// <summary>
            /// Purpose: A Flag to indicate whether Thread-based Processing was force stopped by the User.
            /// </summary>
            bool stoppingStateFlagEnabled{ false };

            /// <summary>
            /// Purpose: A bound Callback Function.
            /// </summary>
            std::function<void(const wchar_t*)> boundCallback{};

            /// <summary>
            /// Purpose: A function that accepts a callback function.
            /// </summary>
            /// <param name="callback"></param>
            /// <param name="value"></param>
            void ExecuteOperation(std::function<void(const wchar_t*)> callback, const wchar_t* value);

            /// <summary>
            /// Purpose: Creates the Folder Processing Thread and Event Synchronization Objects.
            /// </summary>
            void CreateFolderProcessingThread();

            /// <summary>
            /// Purpose: Used to terminate the Folder Processing Thread.
            /// </summary>
            void StopFolderProcessingThread();

            /// <summary>
            /// Purpose: Gets and removes the next available Folder Name in the Queue. 
            /// Uses the Critical Section to perform this operation.
            /// </summary>
            /// <param name="folderNamePtrRef">Set the Reference to the next available Folder Name in the Queue.</param>
            /// <param name="pendingEvent"></param>
            /// <returns>Returns TRUE if a Folder Name was removed from the Queue. Returns FALSE otherwise.</returns>
            bool GetNextFolder(std::wstring& folderNamePtrRef, HANDLE pendingEvent);

            /// <summary>
            /// Purpose: A Thread Procedure used for processing Folders within a Queue.
            /// 
            /// See <a href="https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms686736(v=vs.85)">link</a> for more information.
            /// </summary>
            /// <param name="lpParameter"></param>
            /// <returns></returns>
            static DWORD WINAPI FoldersThreadProc(LPVOID lpParameter);
    };

}

#endif // EXECUTION_MANAGER_H