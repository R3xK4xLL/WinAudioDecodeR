#ifndef FLAC_DECODER_H
#define FLAC_DECODER_H

#include "PureAbstractBaseDecoder.h"
#include "AbstractBaseDecoder.h"
#include "StreamWrapper.h"
#include <stdio.h>
#include <sstream>
#include <FLAC++/all.h>

/// FLAC Nomenclature:
/// 
/// URI: https://xiph.org/flac/format.html#definitions
/// 
/// Blocks and Subblocks are referring to the raw unencoded audio data that is the input to the FLAC encoder. (Blocks/Subblocks ---> FLAC Encoder)
/// 
/// Block: One or more Audio Samples that span several channels. FLAC specifies a minimum Block Size of 16 and a maximum Block Size of 65535.
/// 
/// Subblock: One or more audio samples within a channel. So a block contains one subblock for each channel, and all subblocks contain the same number of samples.
/// 
/// Blocksize: The number of samples in any of a block's Subblocks. For example, a one second block sampled at 44.1KHz has a blocksize of 44100, regardless of the number of channels.
/// 
/// 
/// Frames and Subframes are referring to the FLAC-encoded data. (FLAC Decoder ---> Frames/Subframes)
/// 
/// Frame: A Frame Header plus one or more Subframes.
/// 
/// Subframe: A Subframe Header plus one or more encoded samples from a given channel. All Subframes within a Frame will contain the same number of samples.
///
///

/// <summary>
/// Purpose: A Derived Class implementing a FLAC Decoder. This Class also uses Multiple Inheritance.
/// FLAC API Documentation: https://xiph.org/flac/api/modules.html
/// FLAC Decoder API Documentation: https://xiph.org/flac/api/group__flac__stream__decoder.html
/// URI: https://en.wikipedia.org/wiki/FLAC
/// URI: https://wiki.hydrogenaud.io/index.php?title=Flac
/// </summary>
class FlacDecoder : public AbstractBaseDecoder, public FLAC::Decoder::Stream
{
	public:
		FlacDecoder();
		FlacDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
		virtual ~FlacDecoder();
		FlacDecoder(const FlacDecoder& other) = delete; // Delete Copy Constructor
		FlacDecoder& operator=(const FlacDecoder& other) = delete; // Delete Assignment Operator (Overloaded)
		FlacDecoder(FlacDecoder&& other) noexcept = delete; // Delete The Move Constructor
		FlacDecoder& operator=(FlacDecoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)
		
		#pragma region Overridden_Base_Class_Functions_Region

		/// <summary>
		/// Purpose: Reads/Decodes one MetaData Block or Audio Frame at a time.
		/// </summary>
		/// <returns>The size of Audio Frame that was read/decoded OR -1 indicating an error occcured.</returns>
		long long Read();

		unsigned long long GetDecodedAudioDataTotal();
		const wchar_t* GetLastErrorMessage();
		const wchar_t* GetSupportedTypes();

		#pragma endregion Overridden_Base_Class_Functions_Region

		#pragma region Protected_FLAC_Decoder_Stream_Functions_Region
		
		// The Mandatory Override Functions are read_callback(), write_callback(), and error_callback(). All other functions are optionally overidden.

		/// <summary>
		/// Purpose: A callback function that is called when the Decoder needs more input data. The address of the
		/// buffer to be filled is supplied, along with the number of bytes the buffer can hold. The callback function may choose 
		/// to supply less data and modify the byte count, but must be careful not to overflow the buffer. (Mandatory Override Function)
		/// URI: https://xiph.org/flac/api/group__flac__stream__decoder.html#ga25d4321dc2f122d35ddc9061f44beae7
		/// </summary>
		/// <param name="buffer">A pointer to a location for the callback function to store data to be decoded.</param>
		/// <param name="bytes">A pointer to the size of the buffer. 
		/// On entry to the callback, it contains the maximum number of bytes that may be stored in a buffer. 
		/// The callback function must set it to the actual number of bytes stored (0 in case of error or end-of-stream) before returning.
		/// </param>
		/// <returns>A status code chosen from FLAC__StreamDecoderReadStatus.</returns>
		virtual FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t* bytes);
		
		/// <summary>
		/// Purpose: A callback function that is called when the Decoder has decoded a single Audio Frame.
		/// The decoder will pass the frame Metadata as well as an array of pointers (one for each channel) pointing to the Decoded Audio. (Mandatory Override Function)
		/// URI: https://xiph.org/flac/api/group__flac__stream__decoder.html#ga61e48dc2c0d2f6c5519290ff046874a4
		/// </summary>
		/// <param name="frame">The description of the decoded Audio Frame.</param>
		/// <param name="buffer">
		/// An array of pointers to decoded channels of data. 
		/// Each pointer will point to an array of signed samples of length 'frame->header.blocksize'.
		/// </param>
		/// <returns></returns>
		virtual FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[]);

		/// <summary>
		/// Purpose: A callback function that is called whenever an error occurs during decoding. (Mandatory Override Function)
		/// </summary>
		/// <param name="status">The error encountered by the Decoder.</param>
		virtual void error_callback(FLAC__StreamDecoderErrorStatus status);

		// Seek Function (Optionally Overridden Functions)
		// The seek_callback(), tell_callback(), length_callback(), and eof_callback() functions 
		// are overridden to enable seeking to work.

		virtual FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
		virtual FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64* absolute_byte_offset);
		virtual FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64* stream_length);
		virtual bool eof_callback();
		
		virtual void metadata_callback(const FLAC__StreamMetadata* metadata);

		#pragma endregion Protected_FLAC_Decoder_Stream_Functions_Region

		static constexpr wchar_t* DECODER_NAME = L"FLAC";
		static constexpr wchar_t FILE_EXTENSION_TYPES[] = L"flac\0fla\0";

	private:
		/// <summary>
		/// Purpose: Total Samples in the Stream.
		/// For example, one second of 44.1Khz audio will have 44100 samples regardless of the number of channels. 
		/// A zero means the number of total samples is unknown.
		/// </summary>
		unsigned long long m_streamTotalSampleCount{};

		unsigned long long m_totalDecodedFrameSampleCount{};

		/// <summary>
		/// Purpose: The Sample Rate in Hz for the Stream. 
		/// The Valid Range: 1Hz - 655350Hz. 
		/// A value of zero means the number of total samples is unknown.
		/// </summary>
		unsigned long m_streamSampleRate{};

		bool m_errorCallbackFlagEnabled{ false };
		wchar_t m_errorMessage[MAX_ERROR_SIZE]{};

		StreamWrapper m_stream{};
		long long m_lastDecodedFrameSampleSize{};

		void OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
		void Truncated(wchar_t errorMessage[]) const;

		/// <summary>
		/// Purpose: Checks whether an ID3v1 Tag is embedded at the end of the FLAC file.
		/// </summary>
		/// <returns>TRUE if an ID3v1 Tag is found. FALSE if ID3v1 Tag is NOT found or a Stream error occured. </returns>
		bool hasID3v1Tag();
};


#endif // FLAC_DECODER_H