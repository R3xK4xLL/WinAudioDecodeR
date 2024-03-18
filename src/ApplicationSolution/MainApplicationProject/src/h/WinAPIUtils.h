#ifndef WIN_API_UTILS_H
#define WIN_API_UTILS_H

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <exception>

/// <summary>
/// Purpose: A WinAPI Utility Namespace.
/// </summary>
namespace WinAPIUtils
{
    /// <summary>
    /// Purpose: Obtains the Disk Geometry for the System using the available Logical Drive(s). The caller is required to have Administrative Privileges.
    /// This function can throw an Exception.
    /// </summary>
    /// <returns>A Map containing the Logical Drive Letter (e.g. C:) associated with its Disk Geometry.</returns>
    std::map<std::wstring, DISK_GEOMETRY> GetLogicalDriveGeometry();

    /// <summary>
    /// Purpose: Used to determine at run-time which version of the OS Platform the program is running under.
    /// </summary>
    /// <returns>TRUE if 64-bit OS (_WIN64) {64-bit ARM or x64}, otherwise FALSE 32-bit OS (_WIN32) {32-bit ARM, 64-bit ARM, x86, or x64.}.</returns>
    bool is64BitWindows();

    /// <summary>
    /// Purpose: 
    /// </summary>
    /// <param name="filenamePtr"></param>
    /// <returns>TRUE if File Type and FALSE otherwise.</returns>
    bool IsFileType(const wchar_t* filenamePtr);

    /// <summary>
    /// Purpose: 
    /// </summary>
    /// <param name="filenamePtr"></param>
    /// <returns>TRUE if Folder/Directory Type and FALSE otherwise.</returns>
    bool IsFolderType(const wchar_t* filenamePtr);
}

#endif // WIN_API_UTILS_H