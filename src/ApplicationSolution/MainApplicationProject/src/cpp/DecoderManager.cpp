#include "DecoderManager.h"

using namespace std;

/// <summary>
/// Purpose: Default no-arg Constructor.
/// </summary>
DecoderManager::DecoderManager()
{
    this->PopulateFileExtensionToDecoderNameMap(FlacDecoder::FILE_EXTENSION_TYPES, FlacDecoder::DECODER_NAME);
    this->PopulateFileExtensionToDecoderNameMap(MP3Decoder::FILE_EXTENSION_TYPES, MP3Decoder::DECODER_NAME);
    this->PopulateFileExtensionToDecoderNameMap(WavPackDecoder::FILE_EXTENSION_TYPES, WavPackDecoder::DECODER_NAME);
    this->PopulateFileExtensionToDecoderNameMap(OggVorbisDecoder::FILE_EXTENSION_TYPES, OggVorbisDecoder::DECODER_NAME);
}

#pragma region RULE_OF_FIVE_REGION

// URI: https://en.wikipedia.org/wiki/Rule_of_three_%28C%2B%2B_programming%29

/// <summary>
/// Purpose: The Destructor.
/// </summary>
DecoderManager::~DecoderManager()
{

}

/// <summary>
/// Purpose: The Copy Constructor. A Copy Constructor creates a new Object that is a copy of an existing Object.
/// The copy is created by copying the Member Variables of an existing Object 'other'. This can be time consuming, 
/// especially if large amounts of data are involved during the operation.
/// </summary>
/// <param name="other"></param>


/// <summary>
/// Purpose: The Copy Assignment Operator (Overloaded).
/// </summary>
/// <param name="other"></param>
/// <returns></returns>


/// <summary>
/// Purpose: The Move Constructor. A Move Constructor moves the resources from an existing Object 'other' to another Object 'this'.
/// This operation is typically much faster than copying the Member Variables. 
/// The 'noexcept' keyword is used to indicate that the Move Constructor does not throw any exceptions.
/// </summary>
/// <param name="other">
/// An rvalue reference to another Object of this type. 
/// An rvalue is a temporary object that is created by the Compiler when an expression is evaluated. 
/// An rvalue reference is declared using two Ampersands.
/// </param>


/// <summary>
/// Purpose: Move Assignment Operator (Overloaded). The 'noexcept' keyword is used to indicate that theMove Assignment Operator does not throw any exceptions.
/// </summary>
/// <param name="other">
/// An rvalue reference to another Object of this type. 
/// An rvalue is a temporary object that is created by the Compiler when an expression is evaluated. 
/// An rvalue reference is declared using two Ampersands.
/// </param>
/// <returns></returns>

#pragma endregion RULE_OF_FIVE_REGION

#pragma region Public_Member_Functions_Region

std::unique_ptr<PureAbstractBaseDecoder> DecoderManager::OpenDecoderSmartPointer(const wchar_t* filenamePtr)
{
    PureAbstractBaseDecoder* decoderFilePtr = this->OpenDecoder(filenamePtr);
    unique_ptr<PureAbstractBaseDecoder> decoderFileSmartPtr(decoderFilePtr); // Convert Raw Pointer to a Smart Pointer.
    return decoderFileSmartPtr;
}

bool DecoderManager::IsSupportedType(const wchar_t* filenamePtr)
{
    if (FindDecoderType(filenamePtr).compare(UNSUPPORTED_TYPE) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void DecoderManager::SetDecoderMemoryBufferEnabled(bool value)
{
    this->decoderMemoryBufferEnabled = value;
}

#pragma endregion Public_Member_Functions_Region

#pragma region Private_Member_Functions_Region

void DecoderManager::PopulateFileExtensionToDecoderNameMap(const wchar_t* supportedTypesPtr, const wchar_t* decoderName)
{
    // Iterate through the Array of null-terminated file extensions and populate the Map.
    while (*supportedTypesPtr != NULL)
    {
        wstring supportedType(supportedTypesPtr);

        this->m_fileExtensionToDecoderNameMap.insert(std::make_pair(supportedType, decoderName));

        // Increment the Pointer, by the size of the current Wide String + 1 (the +1 is to advance past the null-terminated characters '\0' within the string literal), 
        // until it reaches the end and returns the final null-terminated character '\0', which is zero.
        supportedTypesPtr += wcslen(supportedTypesPtr) + 1; 
    }
}

std::wstring DecoderManager::FindDecoderType(const wchar_t* filenamePtr) const
{
    // Returns a pointer to the last occurrence of wc in the C-style wide string filename. 
    // The null-terminating character '\0' is considered part of the string.

    wchar_t wc = L'.';
    const wchar_t* lastOccurence = wcsrchr(filenamePtr, wc);
    if (lastOccurence == NULL)
    {
        // The character was not found in the string.
        return UNSUPPORTED_TYPE;
    }
    else {
        // The character was found.
        // Increment the pointer to the next position in the string, just past the found character.
        lastOccurence++;
    }

    const wchar_t* lowerCaseKeyPtr = Utils::ToLowerCase(lastOccurence); // Creates a lowercase C-Style Wide String on the Heap.
    wstring lowerCaseWString(lowerCaseKeyPtr);

    wstring value{};
    auto it = this->m_fileExtensionToDecoderNameMap.find(lowerCaseWString);
    if (it != this->m_fileExtensionToDecoderNameMap.end()) {
        // Value found.
        value = it->second;
    }
    else
    {
        // Value not found.
        value = UNSUPPORTED_TYPE;
    }

    delete lowerCaseKeyPtr; // Clean up
    lowerCaseKeyPtr = nullptr;

    return value;

}

PureAbstractBaseDecoder* DecoderManager::OpenDecoder(const wchar_t* filenamePtr) const
{
    bool decoderIsOpen = false;
    
    wstring decoderType = this->FindDecoderType(filenamePtr);

    PureAbstractBaseDecoder* decoderFilePtr = nullptr;
    if (decoderType.compare(FlacDecoder::DECODER_NAME) == 0)
    {
        // Creates a new Decoder object on the Heap.
        decoderFilePtr = new FlacDecoder(filenamePtr, this->decoderMemoryBufferEnabled);
        decoderIsOpen = ((FlacDecoder*)decoderFilePtr)->DecoderIsOpen();
    }
    else if (decoderType.compare(MP3Decoder::DECODER_NAME) == 0)
    {
        // Creates a new Decoder object on the Heap.
        decoderFilePtr = new MP3Decoder(filenamePtr, this->decoderMemoryBufferEnabled);
        decoderIsOpen = ((MP3Decoder*)decoderFilePtr)->DecoderIsOpen();
    }
    else if (decoderType.compare(WavPackDecoder::DECODER_NAME) == 0)
    {
        // Creates a new Decoder object on the Heap.
        decoderFilePtr = new WavPackDecoder(filenamePtr, this->decoderMemoryBufferEnabled);
        decoderIsOpen = ((WavPackDecoder*)decoderFilePtr)->DecoderIsOpen();
    }
    else if (decoderType.compare(OggVorbisDecoder::DECODER_NAME) == 0)
    {
        // Creates a new Decoder object on the Heap.
        decoderFilePtr = new OggVorbisDecoder(filenamePtr, this->decoderMemoryBufferEnabled);
        decoderIsOpen = ((OggVorbisDecoder*)decoderFilePtr)->DecoderIsOpen();
    }

    if (decoderFilePtr != nullptr && !decoderIsOpen)
    {
        delete decoderFilePtr;
        return nullptr;
    }

    return decoderFilePtr;
}

#pragma endregion Private_Member_Functions_Region