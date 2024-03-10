#ifndef WAV_PACK_DECODER_H
#define WAV_PACK_DECODER_H

#include "PureAbstractBaseDecoder.h"
#include "AbstractBaseDecoder.h"
#include "StreamWrapper.h"
#include "md5.h"
#include <wavpack/wavpack.h>
#include <string>
#include <codecvt>

/// <summary>
/// Purpose: Used for specifying the maximum Buffer Size used for Reading Unpacked Samples.
/// </summary>
constexpr auto MAX_UNPACKED_SAMPLE_BUFFER_SIZE = 4096L;

constexpr auto MAX_WAVPACK_ERROR = 80;

/// <summary>
/// Purpose: A Derived Class implementing a WavPack Decoder. 
/// 
/// URI: https://www.wavpack.com/
/// URI: https://en.wikipedia.org/wiki/WavPack
/// URI: https://wiki.hydrogenaud.io/index.php?title=WavPack
/// 
/// The basic procedure for reading a WavPack file is this:
/// 
/// 1. Open the file with WavpackOpenFileInput(), WavpackOpenFileInputEx, or WavpackOpenFileInputEx64().
/// 2. Determine important characteristics for decoding using these (and other) functions:
///		- WavpackGetNumSamples()
///		- WavpackGetBitsPerSample()
///		- WavpackGetBytesPerSample()
///		- WavpackGetSampleRate()
/// 3. Read decoded samples with WavpackUnpackSamples().
/// 4. Optionally seek with WavpackSeekSample().
/// 5. Close file with WavpackCloseFile().
/// 
/// </summary>
class WavPackDecoder : public AbstractBaseDecoder
{
	public:
		WavPackDecoder();
		WavPackDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
		virtual ~WavPackDecoder();
		WavPackDecoder(const WavPackDecoder& other) = delete; // Delete Copy Constructor
		WavPackDecoder& operator=(const WavPackDecoder& other) = delete; // Delete Assignment Operator (Overloaded)
		WavPackDecoder(WavPackDecoder&& other) noexcept = delete; // Delete The Move Constructor
		WavPackDecoder& operator=(WavPackDecoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

		#pragma region Overridden_Base_Class_Functions_Region

		long long Read();
		unsigned long long GetDecodedAudioDataTotal();
		const wchar_t* GetLastErrorMessage();
		const wchar_t* GetSupportedTypes();

		#pragma endregion Overridden_Base_Class_Functions_Region

		static constexpr wchar_t* DECODER_NAME = L"WAV_PACK";
		static constexpr wchar_t FILE_EXTENSION_TYPES[] = L"wv\0";

	private:
		/// <summary>
		/// Purpose: Used for the regular WavPack data Stream.
		/// </summary>
		StreamWrapper m_streamRegularData{};

		/// <summary>
		/// Purpose: Used for the correction WavPack data Stream.
		/// </summary>
		StreamWrapper m_streamCorrectionData{};

		/// <summary>
		/// Purpose: A Data Structure used within the WavPack Library to encapsulate information and State.
		/// </summary>
		WavpackContext* m_wavPackContextPtr{ nullptr };

		#pragma region WAVPACK_STREAM_READER_ADJUSTMENT_FOR_COMPILATION_TARGET
		#if defined(_WIN64)
		// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

		WavpackStreamReader64 m_wavPackStreamReader64{};

		#elif defined(_WIN32)
		// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

		WavpackStreamReader m_wavPackStreamReader{};

		#endif
		#pragma endregion WAVPACK_STREAM_READER_ADJUSTMENT_FOR_COMPILATION_TARGET

		wchar_t m_errorMessage[MAX_ERROR_SIZE]{};

		long long m_streamTotalSampleCount{};
		unsigned long long m_totalUnpackedSampleCount{};
		int m_bytesPerSample{};
		int m_numberOfChannels{};

		/// <summary>
		/// Purpose: The maximum required Memory in bytes, for storing all of the Unpacked Samples into a Buffer.
		/// Uses the formula: (4 * Number of Complete Samples * Number of Channels).
		/// </summary>
		unsigned long long maximumRequiredSampleBufferSize{};

		/// <summary>
		/// Purpose: Uses the formula: (MAX_UNPACKED_SAMPLE_BUFFER_SIZE / Number of Channels) to limit the number of requested Unpacked Audio Samples.
		/// </summary>
		unsigned long m_requestedCompleteSamples{};
		
		/// <summary>
		/// Purpose: Used for enabling DSD Support and associated processing within the Decoder.
		/// </summary>
		bool m_DSDAudioFlagEnabled{ false };

		bool m_md5ModeIsEnabled{ false };
		md5_state_t m_stateStructMD5Algorithm{};

		/// <summary>
		/// Purpose: A Buffer used for storing Unpacked Samples.
		/// </summary>
		int32_t m_unpackedSampleBuffer[MAX_UNPACKED_SAMPLE_BUFFER_SIZE]{};

		/// <summary>
		/// Purpose: Prepares the Sample Buffer Data and updates Input Message Data that will be used for a final MD5 calculation.
		/// </summary>
		/// <param name="sampleCount">The number of Samples to process for the Input Message Data.</param>
		void UpdateMD5(long long sampleCount);

		/// <summary>
		/// Purpose: A wrapper function for getting Error Messages from WavPack as the std:wstring type.
		/// </summary>
		/// <returns></returns>
		std::wstring GetLastWavPackErrorMessage();

		void OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);

		/// <summary>
		/// Purpose: Used to explicitly close any open Streams and WavPack Files.
		/// </summary>
		void CloseFiles();
};

#endif // WAV_PACK_DECODER_H