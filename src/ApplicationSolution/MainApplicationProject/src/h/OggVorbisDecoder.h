#ifndef OGG_VORBIS_DECODER_H
#define OGG_VORBIS_DECODER_H

#include "PureAbstractBaseDecoder.h"
#include "AbstractBaseDecoder.h"
#include "StreamWrapper.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string>

/// <summary>
/// Purpose: A Derived Class implementing an Ogg-Vorbis Decoder. 
/// 
/// URI: https://www.xiph.org/ogg/
/// URI: https://en.wikipedia.org/wiki/Ogg
/// URI: https://en.wikipedia.org/wiki/Vorbis
/// URI: https://wiki.hydrogenaud.io/index.php?title=Ogg
/// URI: https://wiki.hydrogenaud.io/index.php?title=Vorbis
/// </summary>
class OggVorbisDecoder : public AbstractBaseDecoder
{
    public:
        OggVorbisDecoder();
        OggVorbisDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
        virtual ~OggVorbisDecoder();
        OggVorbisDecoder(const OggVorbisDecoder& other) = delete; // Delete Copy Constructor
        OggVorbisDecoder& operator=(const OggVorbisDecoder& other) = delete; // Delete Assignment Operator (Overloaded)
        OggVorbisDecoder(OggVorbisDecoder&& other) noexcept = delete; // Delete The Move Constructor
        OggVorbisDecoder& operator=(OggVorbisDecoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

        #pragma region Overridden_Base_Class_Functions_Region

        long long Read();
        unsigned long long GetDecodedAudioDataTotal();
        const wchar_t* GetLastErrorMessage();
        const wchar_t* GetSupportedTypes();

        #pragma endregion Overridden_Base_Class_Functions_Region

        static constexpr wchar_t* DECODER_NAME = L"OGG-VORBIS";
        static constexpr wchar_t FILE_EXTENSION_TYPES[] = L"ogg\0";

    private:
        StreamWrapper m_stream{};
        
        /// <summary>
        /// Purpose: An Ogg-Vorbis file Structure.
        /// URI: https://www.xiph.org/vorbis/doc/vorbisfile/OggVorbis_File.html
        /// </summary>
        OggVorbis_File m_oggVorbisFileStruct{};
        ov_callbacks m_oggVorbisCallback{};

        wchar_t m_errorMessage[MAX_ERROR_SIZE]{};

        long long m_streamTotalSampleCount{};
        int m_bitstream{};
        long m_streamSampleRate{};
        float** m_decodedOutputBuffer{};
        
        void OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
};

#endif // OGG_VORBIS_DECODER_H