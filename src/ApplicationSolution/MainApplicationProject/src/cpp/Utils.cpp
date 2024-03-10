#include "Utils.h"

// ****************************************************************************
//							General Purpose Utilities
// ****************************************************************************

const wchar_t* Utils::ToLowerCase(const wchar_t* source)
{
	size_t length = wcslen(source);
	wchar_t* lowerCasePtr = new wchar_t[length + 1] {}; // Add space for the null-terminating character '\0' at the end, by adding 1.

	// Converts each character to lowercase, one character at a time.
	for (size_t i = 0; i < length; ++i) {
		wchar_t originalCharacter = source[i];
		lowerCasePtr[i] = std::tolower(originalCharacter);
	}

	return lowerCasePtr;
}
