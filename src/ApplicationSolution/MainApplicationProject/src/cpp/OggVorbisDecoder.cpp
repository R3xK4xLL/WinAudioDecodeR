#include "OggVorbisDecoder.h"

/// <summary>
/// Purpose: The maximum number of decoded Samples to produce.
/// </summary>
constexpr auto CHUNK_SIZE = 4096;

#pragma region OGG_VORBIS_CALLBACK_FUNCTIONS

/// <summary>
/// Purpose: Namespace contains Callback Functions used for reading and writing Ogg-Vorbis Streams.
/// </summary>
namespace OggVorbisCallbackFunction
{
    static size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource)
    {
        StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(datasource);
        return streamWrapperPtr->Read(ptr, size * nmemb);
    }

    static int seek_func(void* datasource, ogg_int64_t offset, int whence)
    {
        StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(datasource);
        return streamWrapperPtr->Seek(offset, whence);
    }

    static int close_func(void* datasource)
    {
        // Called after the ov_clear() function is called.
        StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(datasource);
        streamWrapperPtr->Close();
        return 1;
    }

    static long tell_func(void* datasource)
    {
        StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(datasource);
        return static_cast<long>(streamWrapperPtr->Tell());
    }
}

#pragma endregion OGG_VORBIS_CALLBACK_FUNCTIONS

OggVorbisDecoder::OggVorbisDecoder() : AbstractBaseDecoder(DECODER_NAME)
{
    this->SetDecoderIsOpenStatus(false);
}

/// <summary>
/// Purpose: With-args Constructor that opens the Decoder using the specified File. Uses Constructor Chaining (i.e. Constructor Delegation)
/// </summary>
/// <param name="filenamePtr"></param>
/// <param name="memoryBufferIsEnabled"></param>
OggVorbisDecoder::OggVorbisDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled) : OggVorbisDecoder()
{
    this->m_decoderMemoryBufferIsEnabled = memoryBufferIsEnabled;
    
    // Open the Ogg-Vorbis File.
    this->OpenFile(filenamePtr, memoryBufferIsEnabled);
}

OggVorbisDecoder::~OggVorbisDecoder()
{
    if (this->DecoderIsOpen())
    {
        // After a bitstream has been opened using ov_open_callbacks() and decoding is complete, the Application must call ov_clear() 
        // to clear the Decoder's buffers. ov_clear() will also close the file and close the StreamWrapper.
        // URI: https://xiph.org/vorbis/doc/vorbisfile/ov_clear.html
        ov_clear(&this->m_oggVorbisFileStruct);
    }
}

#pragma region Overridden_Base_Class_Functions_Region

long long OggVorbisDecoder::Read()
{
    // URI: https://xiph.org/vorbis/doc/vorbisfile/decoding.html

    // Get the Samples in the Native Float Format instead of in Integer Formats.
    // URI: https://www.xiph.org/vorbis/doc/vorbisfile/ov_read_float.html
    long long numberOfSampleBytesRead = ov_read_float(&this->m_oggVorbisFileStruct, &this->m_decodedOutputBuffer, CHUNK_SIZE, &this->m_bitstream);
    switch (numberOfSampleBytesRead)
    {
        case 0LL:
        {
            // Indicates EOF has been reached.
            if (!this->m_oggVorbisFileStruct.os.e_o_s)
            {
                wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"TRUNCATED");
                return -1LL; // An error occured.
            }
            break;
        }
        case OV_HOLE:
        {
            // Indicates that there was an interruption in the Data.
            // Possibilities include one of the following: garbage between pages, loss of sync followed by recapture, or a corrupt page.
            
            // Get the current Decoding offset in seconds.
            // URI: https://www.xiph.org/vorbis/doc/vorbisfile/ov_time_tell.html
            float decodingTimeOffset = (float)ov_time_tell(&this->m_oggVorbisFileStruct);

            swprintf(this->m_errorMessage, MAX_ERROR_SIZE, L"OGG-VORBIS_HOLE @ %dm %02ds", ((int)decodingTimeOffset) / 60, ((int)decodingTimeOffset) % 60);
            return -1LL; // An error occured.
        }
        case OV_EBADLINK:
        {
            // Indicates that an invalid Stream section was supplied to 'libvorbisfile' (OggVorbis_File struct), or the requested Link is corrupt.
            // 
            // NOTE: A Vorbis stream may consist of multiple sections (called Links) that encode differing numbers of channels or sample rates.
            // It is important to pay attention to the Link numbers returned by ov_read() and handle audio changes that may occur at Link boundaries.
            // Multi-section files do exist in the wild and are not merely a specification curiosity.

            // Get the current Decoding offset in seconds.
            // URI: https://www.xiph.org/vorbis/doc/vorbisfile/ov_time_tell.html
            float decodingTimeOffset = (float)ov_time_tell(&this->m_oggVorbisFileStruct);

            swprintf(this->m_errorMessage, MAX_ERROR_SIZE, L"OGG-VORBIS_EBADLINK @ %dm %02ds", ((int)decodingTimeOffset) / 60, ((int)decodingTimeOffset) % 60);
            return -1LL;  // An error occured.
        }
        case OV_EINVAL:
            // Indicates the initial File Headers couldn't be read or are corrupt, OR that the initial open() call for the Vorbis File failed.
            wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"UNREADABLE_OR_CORRUPT_HEADER");
            return -1LL; // An error occured.
    }

    // The number of Sample bytes that were successfully read.
    return numberOfSampleBytesRead;
}

unsigned long long OggVorbisDecoder::GetDecodedAudioDataTotal()
{
    return this->m_streamTotalSampleCount;
}

const wchar_t* OggVorbisDecoder::GetLastErrorMessage()
{
    return this->m_errorMessage;
}

const wchar_t* OggVorbisDecoder::GetSupportedTypes()
{
    return OggVorbisDecoder::FILE_EXTENSION_TYPES;
}

void OggVorbisDecoder::OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled)
{
    // Configure all of the Ogg-Vorbis Callback Functions.
    this->m_oggVorbisCallback.close_func = OggVorbisCallbackFunction::close_func;
    this->m_oggVorbisCallback.read_func = OggVorbisCallbackFunction::read_func;
    this->m_oggVorbisCallback.seek_func = OggVorbisCallbackFunction::seek_func;
    this->m_oggVorbisCallback.tell_func = OggVorbisCallbackFunction::tell_func;

    if (this->m_stream.Open(filenamePtr, memoryBufferIsEnabled))
    {
        // In Windows, the ov_open_callbacks() function should always be used instead of the ov_open() function.
        // URI: https://www.xiph.org/vorbis/doc/vorbisfile/ov_open_callbacks.html
        int openResult = ov_open_callbacks(&this->m_stream, &this->m_oggVorbisFileStruct, 0, 0, this->m_oggVorbisCallback);
        if (openResult == 0)
        {
            // Get ths Vorbis Info Struct which contains basic information about the audio in a Vorbis bitstream.
            // URI: https://xiph.org/vorbis/doc/libvorbis/vorbis_info.html
            // URI: https://xiph.org/vorbis/doc/vorbisfile/ov_info.html
            vorbis_info* vorbisInfoStructPtr = ov_info(&this->m_oggVorbisFileStruct, -1);

            // Set the Sampling rate of the bitstream.
            this->m_streamSampleRate = vorbisInfoStructPtr->rate;

            // Get the total PCM samples for the entire physical bitstream.
            // URI: https://www.xiph.org/vorbis/doc/vorbisfile/ov_pcm_total.html
            this->m_streamTotalSampleCount = ov_pcm_total(&this->m_oggVorbisFileStruct, -1);

            this->SetDecoderIsOpenStatus(true);
        }
        else
        {
            // An error occurred.

            switch (openResult)
            {
                case OV_EREAD:
                    // A read from media returned an error.
                    wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"READ_ERROR");
                    break;
                case OV_ENOTVORBIS:
                    // Bitstream does not contain any Vorbis data.
                    wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"NON_VORBIS_DATA_IN_BITSTREAM");
                    break;
                case OV_EVERSION:
                    // Vorbis version mismatch.
                    wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"VORBIS_VERSION_MISMATCH");
                    break;
                case OV_EBADHEADER:
                    // Invalid Vorbis bitstream header.
                    wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"INVALID_VORBIS_HEADER");
                    break;
                case OV_EFAULT:
                    // Internal logic fault; indicates a bug or heap/stack corruption.
                    wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"DECODER_FAULT_OCCURRED");
                    break;
            }

            this->SetDecoderIsOpenStatus(false);
            this->m_stream.Close();
        }
    }
}

#pragma endregion Overridden_Base_Class_Functions_Region

#pragma region Private_Member_Functions_Region

#pragma endregion Private_Member_Functions_Region