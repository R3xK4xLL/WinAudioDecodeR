#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctype.h>

/// <summary>
/// Purpose: A Utility Namespace.
/// </summary>
namespace Utils
{
    /// <summary>
    /// Purpose: A Utility Function to convert a C-style wide string to a new lowercase C-style wide string.
    /// </summary>
    /// <param name="source">The C-style wide string to convert to lowercase.</param>
    /// <returns>A new C-style wide string that is lowercase. The caller must properly manage/dispose of Pointer to Heap Memory.</returns>
    const wchar_t* ToLowerCase(const wchar_t* source);
    
}

#endif // UTILS_H