#include "WavPackDecoder.h"

// ****************************************************************************
//							WavPack Decoder
// ****************************************************************************

#pragma region WAVPACK_CALLBACK_FUNCTIONS

#pragma region WAVPACK_CALLBACK_FUNCTION_ADJUSTMENT_FOR_COMPILATION_TARGET
#if defined(_WIN64)
	// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

	/// <summary>
	/// Purpose: Namespace contains Callback Functions used for reading and writing WavPack Streams. (64-bit)
	/// </summary>
	namespace WavPackCallbackFunction64
	{
		/// <summary>
		/// Purpose: Indicates whether seeking (random access) is supported.
		/// </summary>
		/// <param name="id"></param>
		/// <returns></returns>
		static int can_seek(void* id)
		{
			return 1;
		}

		/// <summary>
		/// Purpose: Determines the total length of the WavPack file.
		/// </summary>
		/// <param name="id"></param>
		/// <returns></returns>
		static int64_t get_length(void* id)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			int64_t length = streamWrapperPtr->Length();
			return streamWrapperPtr->Length();
		}

		/// <summary>
		/// Purpose: Retrieves the current position (offset) within the WavPack file.
		/// </summary>
		/// <param name="id"></param>
		/// <returns></returns>
		static int64_t get_pos(void* id)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			int64_t position = streamWrapperPtr->Tell();
			return position;
		}

		/// <summary>
		/// Purpose: Pushes back a byte into the stream (useful for lookahead).
		/// </summary>
		/// <param name="id"></param>
		/// <param name="c"></param>
		/// <returns></returns>
		static int push_back_byte(void* id, int c)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			if (streamWrapperPtr->Seek(-1, SEEK_CUR))
			{
				return EOF;
			}
			else
			{
				return c;
			}
		}

		/// <summary>
		/// Purpose: Reads a specified number of bytes from the WavPack file.
		/// </summary>
		/// <param name="id"></param>
		/// <param name="data"></param>
		/// <param name="bcount"></param>
		/// <returns></returns>
		static int32_t read_bytes(void* id, void* data, int32_t bcount)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			size_t numberOfBytesRead = streamWrapperPtr->Read(data, bcount);
			return static_cast<int32_t>(numberOfBytesRead);
		}

		/// <summary>
		/// Purpose: Sets the position to an absolute value within the file.
		/// </summary>
		/// <param name="id"></param>
		/// <param name="pos"></param>
		/// <returns></returns>
		static int set_pos_abs(void* id, int64_t pos)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			return streamWrapperPtr->Seek(pos, SEEK_SET);
		}

		/// <summary>
		/// Purpose:  Adjusts the position relative to the current position.
		/// </summary>
		/// <param name="id"></param>
		/// <param name="delta"></param>
		/// <param name="mode"></param>
		/// <returns></returns>
		static int set_pos_rel(void* id, int64_t delta, int mode)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			return streamWrapperPtr->Seek(delta, mode);
		}
	}

#elif defined(_WIN32)
	// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

	/// <summary>
	/// Purpose: Namespace contains Callback Functions used for reading and writing WavPack Streams (32-bit).
	/// </summary>
	namespace WavPackCallbackFunction32
	{
		static int can_seek(void* id)
		{
			return 1;
		}

		static uint32_t get_length(void* id)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			uint32_t length = static_cast<uint32_t>(streamWrapperPtr->Length());
			return (uint32_t)((StreamWrapper*)id)->Length();
		}

		static uint32_t get_pos(void* id)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			uint32_t position = static_cast<uint32_t>(streamWrapperPtr->Tell());
			return position;
		}

		// Only used once in the WavPack source, and this should be sufficient.
		static int push_back_byte(void* id, int c)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			if (streamWrapperPtr->Seek(-1, SEEK_CUR))
			{
				return EOF;
			}
			else
			{
				return c;
			}
		}

		static int32_t read_bytes(void* id, void* data, int32_t bcount)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			size_t numberOfBytesRead = streamWrapperPtr->Read(data, bcount);
			return static_cast<int32_t>(numberOfBytesRead);
		}

		static int set_pos_abs(void* id, uint32_t pos)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			return streamWrapperPtr->Seek(pos, SEEK_SET);
		}

		static int set_pos_rel(void* id, int32_t delta, int mode)
		{
			StreamWrapper* streamWrapperPtr = static_cast<StreamWrapper*>(id);
			return streamWrapperPtr->Seek(delta, mode);
		}
	}

#endif
#pragma endregion WAVPACK_CALLBACK_FUNCTION_ADJUSTMENT_FOR_COMPILATION_TARGET

#pragma endregion WAVPACK_CALLBACK_FUNCTIONS

WavPackDecoder::WavPackDecoder() : AbstractBaseDecoder(DECODER_NAME)
{
	this->SetDecoderIsOpenStatus(false);
}

/// <summary>
/// Purpose: With-args Constructor that opens the Decoder using the specified File. Uses Constructor Chaining (i.e. Constructor Delegation)
/// </summary>
/// <param name="filenamePtr"></param>
/// <param name="memoryBufferIsEnabled"></param>
WavPackDecoder::WavPackDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled) : WavPackDecoder()
{
	this->m_decoderMemoryBufferIsEnabled = memoryBufferIsEnabled;

	// Open the WavPack File.
	this->OpenFile(filenamePtr, memoryBufferIsEnabled);
}

WavPackDecoder::~WavPackDecoder()
{
	// Close all Streams and Files.
	this->CloseFiles();
}

#pragma region Overridden_Base_Class_Functions_Region

long long WavPackDecoder::Read()
{
	// The actual number of Samples unpacked should be equal to the number of Samples requested, unless the end-of-file is encountered OR an error occurs.
	// If all Samples have been unpacked then 0 will be returned.
	long long numberOfUnpackedSamplesPerChannel = WavpackUnpackSamples(this->m_wavPackContextPtr, this->m_unpackedSampleBuffer, this->m_requestedCompleteSamples);

	if (numberOfUnpackedSamplesPerChannel > 0LL)
	{
		if (this->m_md5ModeIsEnabled)
		{
			long long numberOfUnpackedSamplesForAllChannels = numberOfUnpackedSamplesPerChannel * this->m_numberOfChannels;
			this->UpdateMD5(numberOfUnpackedSamplesForAllChannels);
		}
		this->m_totalUnpackedSampleCount += numberOfUnpackedSamplesPerChannel;
	}
	else if (numberOfUnpackedSamplesPerChannel == 0LL)
	{
		// All Samples have been unpacked.
		
		// Check for the number of errors encountered so far. These are possibly CRC errors, but could also be Missing Blocks.
		int numberOfErrors = WavpackGetNumErrors(this->m_wavPackContextPtr);
		if (numberOfErrors > 0)
		{
			std::wstring errorMessage = this->GetLastWavPackErrorMessage();

			// Errors were encountered.
			swprintf(this->m_errorMessage, MAX_ERROR_SIZE, L"%d BAD_BLOC%s", numberOfErrors, numberOfErrors > 1 ? L"KS" : L"K");
			numberOfUnpackedSamplesPerChannel = -1LL;
		}
		else
		{
			// Errors were NOT encountered.

			std::wstring errorMessage = this->GetLastWavPackErrorMessage();
			
			if (this->m_totalUnpackedSampleCount == this->m_streamTotalSampleCount)
			{
				// Expected sample count match.

				// Perform an optional MD5 Checksum value match check.
				if (this->m_md5ModeIsEnabled)
				{
					// Look for an MD5 checksum stored in the WavPack Metadata.
					unsigned char md5ChecksumWavPack[16]{}; // MD5 is a 128-bit Hash value (32 Hexadecimal characters = 16 bytes).
					if (WavpackGetMD5Sum(this->m_wavPackContextPtr, md5ChecksumWavPack))
					{
						// An MD5 Checksum was found stored in the WavPack Metadata.
						
						// Compute the MD5 Hash Digest using all of the Input Message data.
						unsigned char computedMD5Checksum[16]{}; // MD5 Algorithm produces a 128-bit Hash (32 Hexadecimal characters = 16 bytes).
						md5_finish(&this->m_stateStructMD5Algorithm, computedMD5Checksum);
						
						// Compare the MD5 Checksum values stored at the two memory locations.
						// URI: https://cplusplus.com/reference/cstring/memcmp/
						if (memcmp(md5ChecksumWavPack, computedMD5Checksum, 16) != 0)
						{
							wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"MD5_MISMATCH");
							return -1LL;
						}
					}
					else
					{
						// MD5 checksum was NOT found stored in the WavPack Metadata.
					}
				}
			}
			else
			{
				// A sample count mismatch was found.

				std::wstring errorMessage;
				int sampleCountMismatch = 0;
				if (this->m_streamTotalSampleCount < 0LL)
				{
					// Unknown number of samples encountered.
					sampleCountMismatch = 0;
					errorMessage = L"SAMPLE_COUNT_UNKNOWN_ERROR";
				}
				else if (this->m_totalUnpackedSampleCount < static_cast<unsigned long long>(this->m_streamTotalSampleCount))
				{
					// Missing Samples.
					sampleCountMismatch = static_cast<int>(this->m_streamTotalSampleCount - this->m_totalUnpackedSampleCount);
					errorMessage = L"MISSING_SAMPL";
					sampleCountMismatch == 1 ? errorMessage += L"E" : errorMessage += L"ES";
				}
				else if (this->m_totalUnpackedSampleCount > static_cast<unsigned long long>(this->m_streamTotalSampleCount))
				{
					// Extra Samples.
					sampleCountMismatch = static_cast<int>(this->m_totalUnpackedSampleCount - this->m_streamTotalSampleCount);
					errorMessage = L"EXTRA_SAMPL";
					sampleCountMismatch == 1 ? errorMessage += L"E" : errorMessage += L"ES";
				}

				swprintf(this->m_errorMessage, MAX_ERROR_SIZE, L"%d %s", sampleCountMismatch, errorMessage.c_str());
				numberOfUnpackedSamplesPerChannel = -1LL;
			}
		}
	}

	return numberOfUnpackedSamplesPerChannel;
}

unsigned long long WavPackDecoder::GetDecodedAudioDataTotal()
{
	if (this->m_streamTotalSampleCount < 0LL)
	{
		// Unknown number of Samples encountered.
		return 0ULL;
	}
	else
	{
		return this->m_streamTotalSampleCount;
	}
}

const wchar_t* WavPackDecoder::GetLastErrorMessage()
{
	return this->m_errorMessage;
}

const wchar_t* WavPackDecoder::GetSupportedTypes()
{
	return WavPackDecoder::FILE_EXTENSION_TYPES;
}

#pragma endregion Overridden_Base_Class_Functions_Region

#pragma region Private_Member_Functions_Region

void WavPackDecoder::UpdateMD5(long long sampleCount)
{
	int32_t tempIntegerValue{};
	int32_t* srcIntBufferPtr = this->m_unpackedSampleBuffer; // Access the Buffer through an Integer Pointer.
	unsigned char* dstByteBufferPtr = (unsigned char*)this->m_unpackedSampleBuffer; // Access the Buffer through a Byte Pointer.
	long long sampleCountLoopVar = sampleCount;

	// Prepare the Sample Buffer for an MD5 calculation.
	switch (this->m_bytesPerSample)
	{
		case 1:
			// 1-byte per Sample.
			// This Loop converts each Sample byte and stores it back into the Buffer.
			
			// When entering the Loop, 1st use the Loop variable value for the conditional expression, 
			// then assign and decrement the Loop variable value afterwards, ready for the next Iteration.
			// Terminate the Loop when the Sample Count reaches zero.
			while (sampleCountLoopVar--)
			{
				// The Audio samples are signed Integer values, but for the MD5 calculation, those samples need to be treated as unsigned Integers.
				// Therefore, they must be converted to unsigned Integers. This is achieved by adding 128 to value.
				// Adding 128 to converts the range from [-128, 127] to [0, 255].

				if (this->m_DSDAudioFlagEnabled)
				{
					// DSD Audio was encountered. Conversion is NOT required.
					// Assign the current source byte to the current destination byte buffer memory location's value.
					// Dereference --> Assign RHS Value --> Increment Pointer = Dereference --> Add +0 to Dereferenced RHS Value --> Increment Pointer
					*dstByteBufferPtr++ = (*srcIntBufferPtr++) + 0;
				}
				else
				{
					// Non-DSD Audio was encountered. Conversion is required.
					// Assign the current source converted byte to the current destination byte buffer memory location's value.
					// Dereference --> Assign RHS Value --> Increment Pointer = Dereference --> Add +128 to Dereferenced RHS Value --> Increment Pointer
					*dstByteBufferPtr++ = (*srcIntBufferPtr++) + 128;
				}

				// After these operations, the pointer for source Buffer is incremented ahead by 1 memory location. 
				// The pointer for the destination Buffer is incremented ahead by 1 memory location.
			}
			break;
		case 2:
			// 2-bytes per Sample.
			// This Loop converts each 2-byte Sample into two separate bytes and stores them back into the Buffer.
			while (sampleCountLoopVar--)
			{
				// Copy the lower 8-bits of the temporary value to the destination value.
				// The lower 8-bits of the temporary value are extracted by casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = Dereference --> Assign Value to Temporary RHS Value --> Increment Pointer
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue = *srcIntBufferPtr++);

				// Copy the upper 8-bits of the temporary value to destination value.
				// The upper 8-bits of the temporary value are extracted by bit-shifting to the right 8-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 8);

				// After these operations, the pointer for the source Buffer is incremented ahead by 1 memory location. 
				// The pointer for the destination Buffer is incremented ahead by 2 memory locations.
			}
			break;
		case 3:
			// 3-bytes per Sample.
			// This Loop converts each 3-byte Sample into three separate bytes and stores them back into the Buffer.
			while (sampleCountLoopVar--)
			{
				// Copy the lower 8-bits of the temporary value to the destination value.
				// The lower 8-bits of the temporary value are extracted by casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = Dereference --> Assign Value to Temporary RHS Value --> Increment Pointer
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue = *srcIntBufferPtr++);

				// Copy the middle 8-bits of the temporary value to destination value.
				// The middle 8-bits of the temporary value are extracted by bit-shifting to the right 8-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 8);

				// Copy the upper 8-bits of the temporary value to destination value.
				// The upper 8-bits of the temporary value are extracted by bit-shifting to the right 16-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 16);

				// After these operations, the pointer for the source Buffer is incremented ahead by 1 memory location. 
				// The pointer for the destination Buffer is incremented ahead by 3 memory locations.
			}
			break;
		case 4:
			// 4-bytes per Sample.
			// This Loop converts each 4-byte Sample into four separate bytes and stores them back into the Buffer.
			while (sampleCountLoopVar--)
			{
				// Copy the lower 8-bits of the temporary value to the destination value.
				// The lower 8-bits of the temporary value are extracted by casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = Dereference --> Assign Value to Temporary RHS Value --> Increment Pointer
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue = *srcIntBufferPtr++);

				// Copy the middle 8-bits of the temporary value to destination value.
				// The middle 8-bits of the temporary value are extracted by bit-shifting to the right 8-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 8);

				// Copy the middle 8-bits of the temporary value to destination value.
				// The middle 8-bits of the temporary value are extracted by bit-shifting to the right 16-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 16);

				// Copy the upper 8-bits of the temporary value to destination value.
				// The upper 8-bits of the temporary value are extracted by bit-shifting to the right 24-bits, and then casting it to an 'unsigned char'.
				// Dereference --> Assign RHS Value --> Increment Pointer = RHS Value
				*dstByteBufferPtr++ = (unsigned char)(tempIntegerValue >> 24);

				// After these operations, the pointer for the source Buffer is incremented ahead by 1 memory location. 
				// The pointer for the destination Buffer is incremented ahead by 4 memory locations.
			}
			break;
		default:
			throw std::exception("Unsupported Bytes per Sample value encountered.");
			break;
	}

	// Sample Buffer values have been modified and are ready for an MD5 calculation.
	// Append a String to the Input Message.
	int numberOfBytes = static_cast<long>(sampleCount) * this->m_bytesPerSample;
	md5_append(&this->m_stateStructMD5Algorithm, (unsigned char*)this->m_unpackedSampleBuffer, numberOfBytes);
}

std::wstring WavPackDecoder::GetLastWavPackErrorMessage()
{
	std::wstring errorMessage = L"No errors messages reported by WavPack.";

	// Get a pointer to a string describing the last error generated by WavPack. 
	// This may provide useful information about why something is not working the way it should.
	char* errorMessagePtr = WavpackGetErrorMessage(this->m_wavPackContextPtr);
	size_t errorMessageSize = strlen(errorMessagePtr);
	if (errorMessageSize > 0)
	{
		std::string errorMessageWavPack;
		errorMessageWavPack += "The following error message was reported by WavPack: ";
		errorMessageWavPack += errorMessagePtr;

		// Convert string type.
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		errorMessage = converter.from_bytes(errorMessageWavPack);
	}

	return errorMessage;
}

void WavPackDecoder::OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled)
{
	// Open the WavPack file with regular data. (Required)
	this->m_streamRegularData.Open(filenamePtr, memoryBufferIsEnabled);

	// Attempt to locate a WavPack file (with correction data), that has the same filename.
	StreamWrapper* correctionDataPtr = nullptr;
	std::wstring correctionDataFilename = filenamePtr;
	correctionDataFilename += L'c'; // Append a 'c' to the current filename. (NOTE: .wvc is the correction file extension)

	// Open the WavPack file with correction data. (Optional)
	if (this->m_streamCorrectionData.Open(correctionDataFilename.c_str(), this->IsDecoderMemoryBufferIsEnabled()))
	{
		// Correction data file was opened successfully.
		correctionDataPtr = &this->m_streamCorrectionData;
	}

	#pragma region WAVPACK_STREAM_READER_ADJUSTMENT_FOR_COMPILATION_TARGET
	#if defined(_WIN64)
		// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

		// Configure all of the WavPack Reader Callback Functions. (64-bit Support)
		this->m_wavPackStreamReader64.can_seek = WavPackCallbackFunction64::can_seek;
		this->m_wavPackStreamReader64.get_length = WavPackCallbackFunction64::get_length;
		this->m_wavPackStreamReader64.get_pos = WavPackCallbackFunction64::get_pos;
		this->m_wavPackStreamReader64.push_back_byte = WavPackCallbackFunction64::push_back_byte;
		this->m_wavPackStreamReader64.read_bytes = WavPackCallbackFunction64::read_bytes;
		this->m_wavPackStreamReader64.write_bytes = NULL;
		this->m_wavPackStreamReader64.set_pos_abs = WavPackCallbackFunction64::set_pos_abs;
		this->m_wavPackStreamReader64.set_pos_rel = WavPackCallbackFunction64::set_pos_rel;
		this->m_wavPackStreamReader64.truncate_here = NULL;
		this->m_wavPackStreamReader64.close = NULL; // Callback set to NULL, so that the Application is explicitly allowed to handle the closing of files.

	#elif defined(_WIN32)
		// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

		// Configure all of the WavPack Reader Callback Functions. (32-bit Support)
		this->m_wavPackStreamReader.can_seek = WavPackCallbackFunction32::can_seek;
		this->m_wavPackStreamReader.get_length = WavPackCallbackFunction32::get_length;
		this->m_wavPackStreamReader.get_pos = WavPackCallbackFunction32::get_pos;
		this->m_wavPackStreamReader.push_back_byte = WavPackCallbackFunction32::push_back_byte;
		this->m_wavPackStreamReader.read_bytes = WavPackCallbackFunction32::read_bytes;
		this->m_wavPackStreamReader.write_bytes = NULL;
		this->m_wavPackStreamReader.set_pos_abs = WavPackCallbackFunction32::set_pos_abs;
		this->m_wavPackStreamReader.set_pos_rel = WavPackCallbackFunction32::set_pos_rel;

	#endif
	#pragma endregion WAVPACK_STREAM_READER_ADJUSTMENT_FOR_COMPILATION_TARGET

		// The basic procedure for reading a WavPack file is this:
		// 
		// 1. Open the file with WavpackOpenFileInput(), WavpackOpenFileInputEx, or WavpackOpenFileInputEx64().
		// 2. Determine important characteristics for decoding using these (and other) functions:
		//		- WavpackGetNumSamples() OR WavpackGetNumSamples64()
		//		- WavpackGetBitsPerSample()
		//		- WavpackGetBytesPerSample()
		//		- WavpackGetSampleRate()
		// 3. Read decoded samples with WavpackUnpackSamples().
		// 4. Optionally seek with WavpackSeekSample() OR WavpackSeekSample64().
		// 5. Close file with WavpackCloseFile().

		// Used for reporting errors.
		char error[MAX_WAVPACK_ERROR] = { 0 };

	#pragma region WAVPACK_OPEN_FILE_ADJUSTMENT_FOR_COMPILATION_TARGET
	#if defined(_WIN64)
		// _WIN64 is defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.

		// Open the WavPack File.
		// OPEN_WVC Flag: Attempts to open and read a corresponding "correction" file along with the standard WavPack file.
		// Supports opening Files larger than 2GB.
		this->m_wavPackContextPtr = WavpackOpenFileInputEx64(&this->m_wavPackStreamReader64, &this->m_streamRegularData, correctionDataPtr, error, OPEN_WVC | OPEN_DSD_NATIVE | OPEN_ALT_TYPES | OPEN_WRAPPER | OPEN_TAGS | OPEN_FILE_UTF8, 0);

	#elif defined(_WIN32)
		// _WIN32 is defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.

		// Open the WavPack File.
		// OPEN_WVC Flag: Attempts to open and read a corresponding "correction" file along with the standard WavPack file.
		// Limited to opening 2GB files or less.
		this->m_wavPackContextPtr = WavpackOpenFileInputEx(&this->m_wavPackStreamReader, &this->m_streamRegularData, correctionDataPtr, error, OPEN_WVC | OPEN_DSD_NATIVE | OPEN_ALT_TYPES | OPEN_WRAPPER | OPEN_TAGS | OPEN_FILE_UTF8, 0);

	#endif
	#pragma endregion WAVPACK_OPEN_FILE_ADJUSTMENT_FOR_COMPILATION_TARGET

	if (this->m_wavPackContextPtr != nullptr)
	{
		// WavPack File was opened successfully.
		this->m_totalUnpackedSampleCount = 0ULL;

		// Get total number of samples contained in the WavPack file. Returns -1 if the total number of samples is unknown.
		this->m_streamTotalSampleCount = WavpackGetNumSamples64(this->m_wavPackContextPtr);

		// Get the number of bytes used for each sample (1 to 4) in the specified WavPack file.
		this->m_bytesPerSample = WavpackGetBytesPerSample(this->m_wavPackContextPtr);

		// Get the number of channels in the specified WavPack file.
		this->m_numberOfChannels = WavpackGetNumChannels(this->m_wavPackContextPtr);

		this->maximumRequiredSampleBufferSize = 4 * this->m_streamTotalSampleCount * this->m_numberOfChannels;
		this->m_requestedCompleteSamples = MAX_UNPACKED_SAMPLE_BUFFER_SIZE / this->m_numberOfChannels;

		// Get the Qualify Mode to determine whether DSD Audio is present within the specified WavPack file. 
		int qualifyMode = WavpackGetQualifyMode(this->m_wavPackContextPtr);
		if (qualifyMode & QMODE_DSD_MSB_FIRST || qualifyMode & QMODE_DSD_LSB_FIRST)
		{
			// DSD Audio is present.
			this->m_DSDAudioFlagEnabled = true;
		}
		else
		{
			// Non-DSD Audio is present.
			this->m_DSDAudioFlagEnabled = false;
		}

		// Check if the File contains an MD5 Checksum value AND whether the File decoding is Lossless (e.g. either Pure or Hybrid).
		int mode = WavpackGetMode(this->m_wavPackContextPtr);
		if ((mode & MODE_MD5) && (mode & MODE_LOSSLESS))
		{
			this->m_md5ModeIsEnabled = true;

			// Initialize the MD5 Algorithm.
			md5_init(&this->m_stateStructMD5Algorithm);
		}
		else
		{
			// File does NOT contain an MD5 Checksum value.
			this->m_md5ModeIsEnabled = false;
		}

		// Set the Decoder Status as Open.
		this->SetDecoderIsOpenStatus(true);
	}
	else
	{
		// Opening WavPack File failed.

		// An error occcured.
		std::string errorMessageWavPack;
		errorMessageWavPack += "An error occurred opening the WavPack File. The following error was reported by WavPack: ";
		errorMessageWavPack += error;

		// Convert string type.
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring errorMessage = converter.from_bytes(errorMessageWavPack);

		wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, errorMessage.c_str());

		// Set the Decoder Status as Closed.
		this->SetDecoderIsOpenStatus(false);

		// Close Files.
		this->m_streamRegularData.Close();
		this->m_streamCorrectionData.Close();
	}
}

void WavPackDecoder::CloseFiles()
{
	if (this->DecoderIsOpen())
	{
		WavpackCloseFile(this->m_wavPackContextPtr);
	}
	this->m_streamRegularData.Close();
	this->m_streamCorrectionData.Close();
}

#pragma endregion Private_Member_Functions_Region