#include "ExecutionManager.h"

// ****************************************************************************
//                          Execution Manager
// ****************************************************************************

MainApplication::ExecutionManager::ExecutionManager(const LPCRITICAL_SECTION lpCriticalSection)
{
    this->criticalSectionPtr = lpCriticalSection;
    
    // Create the Thread.
    this->CreateFolderProcessingThread();
}

MainApplication::ExecutionManager::~ExecutionManager()
{
    this->StopFolderProcessingThread();
}

void MainApplication::ExecutionManager::AddFolderToQueue(const wchar_t* folderNamePtr, bool setPendingEventFlag)
{
    if (folderNamePtr != nullptr)
    {
        // Request ownership of the Critical Section.
        EnterCriticalSection(this->criticalSectionPtr);

        // Access the Shared Resource.
        this->folderQueue.push(folderNamePtr); // Copies the C-style String to a new wstring object in the Queue.
        
        // Release ownership of the Critical Section.
        LeaveCriticalSection(this->criticalSectionPtr);

        if (setPendingEventFlag == true)
        {
            // Sets the specified Pending Event Object to the 'signaled state' (Available).
            SetEvent(this->handlePendingEvent);
        }
    }
}

void MainApplication::ExecutionManager::SetBoundCallback(std::function<void(const wchar_t*)> boundCallback)
{
    this->boundCallback = boundCallback;
}

void MainApplication::ExecutionManager::SetPendingEvent()
{
    // This Function call Blocks until the Finished Event Object Handle has been set to the 'signaled state' (Available).
    if (WaitForSingleObject(this->handleFinishedEvent, INFINITE) == WAIT_OBJECT_0)
    {
        SetEvent(this->handlePendingEvent); // Sets the specified Pending Event Object to the 'signaled state' (Available).
    }
}

void MainApplication::ExecutionManager::ClearFolderQueue()
{
    // Request ownership of the Critical Section.
    EnterCriticalSection(this->criticalSectionPtr);

    // Access the Shared Resource.
    while (!this->folderQueue.empty())
    {
        this->folderQueue.pop(); // (Modify Shared Data)
    }

    // Release ownership of the Critical Section.
    LeaveCriticalSection(this->criticalSectionPtr);
}

bool MainApplication::ExecutionManager::IsFolderQueueEmpty()
{
    // Request ownership of the Critical Section.
    EnterCriticalSection(this->criticalSectionPtr);

    // Access the Shared Resource.
    bool isQueueEmpty = this->folderQueue.empty(); // (Read Shared Data)

    // Release ownership of the Critical Section.
    LeaveCriticalSection(this->criticalSectionPtr);

    return isQueueEmpty;
}

void MainApplication::ExecutionManager::AddToProcessedFileList(const wchar_t* filenamePtr)
{
    this->processedFileList.push_back(filenamePtr);
}

void MainApplication::ExecutionManager::ClearProcessedFileList()
{
    this->processedFileList.clear();
}

std::list<std::wstring>& MainApplication::ExecutionManager::GetProcessedFileListRef()
{
    return this->processedFileList;
}

bool MainApplication::ExecutionManager::IsRunningStateFlagEnabled() const
{
    return this->runningStateFlagEnabled;
}

void MainApplication::ExecutionManager::SetRunningStateFlagEnabled(bool value)
{
    this->runningStateFlagEnabled = value;
}

bool MainApplication::ExecutionManager::IsStoppingStateFlagEnabled() const
{
    return this->stoppingStateFlagEnabled;
}

void MainApplication::ExecutionManager::SetStoppingStateFlagEnabled(bool value)
{
    this->stoppingStateFlagEnabled = value;
}

void MainApplication::ExecutionManager::ExecuteOperation(std::function<void(const wchar_t*)> callback, const wchar_t* value)
{
    if (value != nullptr)
    {
        // Call the Callback function.
        callback(value);
    }
}

void MainApplication::ExecutionManager::CreateFolderProcessingThread()
{
    // Get a pointer to this instance.
    ExecutionManager* myInstance = this;

    // Create Event Synchronization Objects. 
    // 
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

    this->handleTerminateEvent = CreateEvent(NULL, bManualReset, false, NULL); // The initial state of the Event Object (set to 'non-signaled') (Owned). 
    this->handlePendingEvent = CreateEvent(NULL, bManualReset, false, NULL); // The initial state of the Event Object (set to 'non-signaled') (Owned). 
    this->handleFinishedEvent = CreateEvent(NULL, bManualReset, true, NULL); // The initial state of the Event Object (set to 'signaled') (Available). 

    LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL; // Use the default security descriptor. All access privileges are granted to the Thread.
    SIZE_T dwStackSize = NULL; // Set the Thread Stack size to use the Default Stack Size, that is used by the Process.
    LPTHREAD_START_ROUTINE lpStartAddress = MainApplication::ExecutionManager::FoldersThreadProc;
    LPVOID lpParameter = myInstance; // Thread Argument
    DWORD dwCreationFlags = 0; // Set the Thread to run immediately after creation.

    // Create the Thread.
    this->handleFolderThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, &this->folderThreadId);

    if (this->handleFolderThread == NULL)
    {
        // Thread creation failed.

        // Get the last Error Code, which was set by the call to the CreateThread() function.
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        // URI: https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes#system-error-codes
        DWORD dwErrorCode = GetLastError();

        // Construct an Error Message the will be displayed to the User.
        LPWSTR error_message = nullptr;
        DWORD dwResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, dwErrorCode, LANG_SYSTEM_DEFAULT, (LPWSTR)&error_message, 0, nullptr);

        // Prompt the User with an Error Message Box.
        MessageBox(NULL, error_message, WINDOW_TITLE, MB_ICONERROR | MB_OK);

        // NOTE: The ExitProcess() function is the preferred method of ending a process. This function provides a clean Process shutdown. 
        // This includes calling the entry-point function of all attached Dynamic-link libraries (DLLs) with DLL_PROCESS_DETACH, indicating that the Process is detaching from the DLL. 
        // After all attached DLLs have executed any process termination code, the ExitProcess() function terminates the current Process, including the calling Thread.
        // In contrast, if a Process terminates by calling the TerminateProcess() function, the DLLs that the Process is attached to are not notified of the Process termination.

        // Terminate the Program.
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess
        ExitProcess(0);
    }
}

void MainApplication::ExecutionManager::StopFolderProcessingThread()
{
    // Sets the specified event object to the signaled state. (synchapi.h)
    // URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
    SetEvent(this->handleTerminateEvent); // Sets Terminate Event Object to the signaled state for all Threads. This call initiates Thread shutdown.
    
    if (this->handleFolderThread != nullptr)
    {
        // Close the Thread Handle.
        CloseHandle(this->handleFolderThread);
        this->handleFolderThread = nullptr;
    }
}

bool MainApplication::ExecutionManager::GetNextFolder(std::wstring& folderNameRef, HANDLE pendingEvent)
{
    bool wasRemovedFromQueue = true;

    // Request ownership of the Critical Section.
    EnterCriticalSection(this->criticalSectionPtr);

    // Access the Shared Resource.
    if (this->folderQueue.empty())
    {
        wasRemovedFromQueue = false;
    }
    else
    {
        // Get the current Folder.
        folderNameRef = this->folderQueue.front(); // Set the folder name (Copy). (Read Shared Data)
        
        // Remove the current folder from the Queue.
        this->folderQueue.pop(); // Remove from folder from the Queue. (Modify Shared Data)
    }

    // Release ownership of the Critical Section.
    LeaveCriticalSection(this->criticalSectionPtr);

    if (!wasRemovedFromQueue)
    {
        // Sets the specified Event Object to the 'non-signaled state' (Owned).
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent
        ResetEvent(handlePendingEvent);
    }

    return wasRemovedFromQueue;
}

DWORD WINAPI MainApplication::ExecutionManager::FoldersThreadProc(LPVOID lpParameter)
{
    // Pointer allows acccess to instance members.
    ExecutionManager* instancePtr = reinterpret_cast<ExecutionManager*>(lpParameter);
    
    std::wstring folderName{};

    const int waitTimeOutInterval = 0;
    const int numberOfEventHandleObjects = 2;

    // The index value used to determine whether the main Thread Loop is terminated.
    static const DWORD THREAD_TERMINATION_INDEX_VALUE = 0;

    // The index value used to determine whether the main Thread Loop can start/continue Looping.
    static const DWORD THREAD_CONTINUATION_INDEX_VALUE = 1;

    // An array of Object Handles for use with the synchronization Wait Function.
    HANDLE handleEventObjectsThreadState[numberOfEventHandleObjects] = { 0 };
    handleEventObjectsThreadState[THREAD_TERMINATION_INDEX_VALUE] = instancePtr->handleTerminateEvent; // Event Object (Thread End)
    handleEventObjectsThreadState[THREAD_CONTINUATION_INDEX_VALUE] = instancePtr->handlePendingEvent; // Event Object (Thread Start/Continue)

    // This Function call blocks until at least one of the Event Objects is set to the 'signaled state' (Available).
    DWORD waitEventMessageHandleIndex = WaitForMultipleObjects(numberOfEventHandleObjects, handleEventObjectsThreadState, false, INFINITE);

    if (waitEventMessageHandleIndex == WAIT_FAILED)
    {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_INVALID_HANDLE)
        {
            // The handle is invalid.
            int breakpoint = 0;
        }
    }

    // Purpose: The main Thread Loop.
    // The main Thread Loop runs each time a Pending Event Object is set to the 'signaled state' (Available).
    // The main Thread Loop terminates when Terminate Event Object is set to the 'signaled state' (Available).
    while (waitEventMessageHandleIndex != THREAD_TERMINATION_INDEX_VALUE && waitEventMessageHandleIndex == THREAD_CONTINUATION_INDEX_VALUE)
    {

        // Sets the specified Finished Event Object to the 'non-signaled state' (Owned).
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent
        ResetEvent(instancePtr->handleFinishedEvent);
        
        // Purpose: A Thread Loop to Process each available Folder in the Queue.
        // The Thread Loop terminates when the Terminate Event Object is set to the 'signaled state' (Available).
        // OR
        // The Thread Loop terminates when no more queued Folders available.
        // 
        // Waits until the specified object (Terminate Event Object) is in the 'signaled state' (Available) or the time-out interval elapses.
        // The time-out interval is set to 0, therefore the function does not enter a Wait State if the object is 'not-signaled' (Owned); it always returns immediately.
        // Therefore, this function call does NOT Block.
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
        while (WaitForSingleObject(instancePtr->handleTerminateEvent, waitTimeOutInterval) && instancePtr->GetNextFolder(folderName, instancePtr->handlePendingEvent))
        {
            // Request ownership of the Critical Section.
            EnterCriticalSection(instancePtr->criticalSectionPtr);
            
            instancePtr->SetRunningStateFlagEnabled(true);  // (Modify Shared Data)
            
            // Release ownership of the Critical Section.
            LeaveCriticalSection(instancePtr->criticalSectionPtr);

            // Process current folder. (Execute the Callback Function)
            instancePtr->ExecuteOperation(instancePtr->boundCallback, folderName.c_str());

            // Request ownership of the Critical Section.
            EnterCriticalSection(instancePtr->criticalSectionPtr);

            instancePtr->SetRunningStateFlagEnabled(false);  // (Modify Shared Data)

            // Release ownership of the Critical Section.
            LeaveCriticalSection(instancePtr->criticalSectionPtr);
        }

        // Sets an Individual Finished Event Object to the 'signaled state' (Available).
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
        SetEvent(instancePtr->handleFinishedEvent);

        // This Function call blocks until at least one of the Event Objects is set to the 'signaled state' (Available).
        waitEventMessageHandleIndex = WaitForMultipleObjects(numberOfEventHandleObjects, handleEventObjectsThreadState, false, INFINITE);
    }
    
    return 0;
}
