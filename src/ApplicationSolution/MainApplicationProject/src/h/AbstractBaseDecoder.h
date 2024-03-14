#ifndef ABSTRACT_BASE_DECODER_H
#define ABSTRACT_BASE_DECODER_H

#include "PureAbstractBaseDecoder.h"

/// <summary>
/// Purpose: An Abstract Base Class (ABC) for all Decoders. 
/// If a Derived Class does not override all of the Pure Virtual Functions of it's Base Class 
/// (e.g. either an Abstract Base Class (ABC) or Pure Abstract Base Class (PABC)), then the Derived Class is considered an Abstract Class.
/// </summary>
class AbstractBaseDecoder : public PureAbstractBaseDecoder
{
public:
    AbstractBaseDecoder(std::wstring decoderName);
    ~AbstractBaseDecoder();
    AbstractBaseDecoder(const AbstractBaseDecoder& other) = delete; // Delete Copy Constructor
    AbstractBaseDecoder& operator=(const AbstractBaseDecoder& other) = delete; // Delete Assignment Operator (Overloaded)
    AbstractBaseDecoder(AbstractBaseDecoder&& other) noexcept = delete; // Delete The Move Constructor
    AbstractBaseDecoder& operator=(AbstractBaseDecoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

    #pragma region Overridden_Base_Class_Functions_Region

    std::wstring GetName();

    #pragma endregion Overridden_Base_Class_Functions_Region

    bool DecoderIsOpen() const;

    /// <summary>
    /// Purpose: Gets the File Extensions types supported by the Decoder.
    /// </summary>
    /// <returns>Returns an array of null separated file extensions</returns>
    virtual const wchar_t* GetSupportedTypes() = 0;

protected:
    bool m_decoderMemoryBufferIsEnabled{ false }; // Use a memory buffer for decoder.
    void SetDecoderIsOpenStatus(bool value);
    bool IsDecoderMemoryBufferIsEnabled() const;

private:
    bool m_decoderIsOpen{ false };
    std::wstring decoderName;
};

#endif // ABSTRACT_BASE_DECODER_H