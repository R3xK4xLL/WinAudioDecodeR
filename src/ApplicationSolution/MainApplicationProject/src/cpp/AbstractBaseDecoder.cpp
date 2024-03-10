#include "AbstractBaseDecoder.h"

AbstractBaseDecoder::AbstractBaseDecoder(std::wstring decoderName)
{
    this->decoderName = decoderName;
}

AbstractBaseDecoder::~AbstractBaseDecoder()
{
    
}

#pragma region Overridden_Base_Class_Functions_Region

std::wstring AbstractBaseDecoder::GetName()
{
    return this->decoderName;
}

#pragma endregion Overridden_Base_Class_Functions_Region

#pragma region Public_Member_Functions_Region

bool AbstractBaseDecoder::DecoderIsOpen() const
{
    return this->m_decoderIsOpen;
}

#pragma endregion Public_Member_Functions_Region

#pragma region Protected_Member_Functions_Region

void AbstractBaseDecoder::SetDecoderIsOpenStatus(bool value)
{
    this->m_decoderIsOpen = value;
}

bool AbstractBaseDecoder::IsDecoderMemoryBufferIsEnabled() const
{
    return this->m_decoderMemoryBufferIsEnabled;
}

#pragma endregion Protected_Member_Functions_Region