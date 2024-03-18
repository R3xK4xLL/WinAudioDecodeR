#include "WinAPIUtils.h"

std::map<std::wstring, DISK_GEOMETRY> WinAPIUtils::GetLogicalDriveGeometry()
{
    // URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getlogicaldrives
    DWORD logicalDrives = GetLogicalDrives();

    // Create and populate a List.
    std::list<std::pair<std::wstring, std::wstring>> logicalDriveList{};
    for (char driveLetter = 'A'; driveLetter <= 'Z'; ++driveLetter) {
        if (logicalDrives & (1 << (driveLetter - 'A'))) {
            
            std::wstring logicalDrivePath;
            logicalDrivePath += L"\\\\.\\";
            logicalDrivePath += driveLetter;
            logicalDrivePath += L":";

            std::wstring logicalDriveLetter;
            logicalDriveLetter += driveLetter;
            logicalDriveLetter += L":";

            logicalDriveList.push_back(std::make_pair(logicalDrivePath, logicalDriveLetter));
        }
    }

    // Loop over the List and create and populate a Map.
    std::map<std::wstring, DISK_GEOMETRY> driveToDiskGeometryMap;
    for (std::list<std::pair<std::wstring, std::wstring>>::iterator it = logicalDriveList.begin(); it != logicalDriveList.end(); ++it) {
        std::wstring logicalDrivePath = (*it).first;
        LPCWSTR logicalDrivePathPtr = logicalDrivePath.c_str();

        std::wstring logicalDriveLetter = (*it).second;

        // Initialize a Direct Access Storage Device (DASD) Handle that can be used with the DeviceIoControl function.
        HANDLE hDevice = INVALID_HANDLE_VALUE;

        // Using the CreateFile() function for direct access to the disk or to a volume is restricted.
        // 1. The caller must have Administrative Privileges.
        // 2. The dwCreationDisposition parameter must have the OPEN_EXISTING flag.
        // 3. The dwShareMode parameter must have the FILE_SHARE_WRITE flag.
        // 4. The dwDesiredAccess parameter can be zero, allowing the application to query device attributes without accessing a device.
        // URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
        hDevice = CreateFile(logicalDrivePathPtr, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (hDevice != INVALID_HANDLE_VALUE) {
            DISK_GEOMETRY diskGeometry{};

            DWORD dwIoControlCode = IOCTL_DISK_GET_DRIVE_GEOMETRY;
            LPVOID lpInBuffer = NULL;
            DWORD nInBufferSize = 0;
            LPVOID lpOutBuffer = &diskGeometry;
            DWORD nOutBufferSize = sizeof(diskGeometry);
            LPDWORD lpBytesReturned = 0;
            LPOVERLAPPED lpOverlapped = NULL;

            // Get the Logical Drive Geometry from the Device.
            // URI: https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol
            // URI: https://learn.microsoft.com/en-us/windows/win32/devio/calling-deviceiocontrol
            bool logicalDriveGeometryWasObtained = DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
            
            if (logicalDriveGeometryWasObtained)
            {
                // Logical Disk Geometry retrieved successfully.
                driveToDiskGeometryMap.insert(std::make_pair(logicalDriveLetter, diskGeometry));
            }
            else
            {
                // Error retrieving Logical Disk Geometry.
                throw std::exception("Error retrieving Logical Disk Geometry.");
            }
        }

        if (hDevice != 0)
        {
            CloseHandle(hDevice);
        }
    }

    // Return the Map by value. (Copy-constructor will be called).
    return driveToDiskGeometryMap;
}

bool WinAPIUtils::is64BitWindows()
{
    #if defined(_WIN64)
    // _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.
    // 64-bit programs only on run on 64-bit Windows.
    return true;

    #elif defined(_WIN32)
    // _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.
    // 32-bit Programs run on both 32-bit and 64-bit Windows.
    BOOL isWow64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64;

    #endif
}

bool WinAPIUtils::IsFileType(const wchar_t* filenamePtr)
{
    // URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw
    // URI: https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants

    // Check if the file specified by the path is a Directory.
    if (GetFileAttributes(filenamePtr) & FILE_ATTRIBUTE_DIRECTORY)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool WinAPIUtils::IsFolderType(const wchar_t* filenamePtr)
{
    // URI: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw
    // URI: https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants

    // Check if the file specified by the path is a Directory.
    if (GetFileAttributes(filenamePtr) & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;
    }
    else
    {
        return false;
    }
}