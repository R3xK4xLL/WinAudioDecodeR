#include "FlacDecoder.h"

// ****************************************************************************
//							FLAC Decoder
// ****************************************************************************

/// <summary>
/// Purpose: No-arg Constructor.
/// </summary>
FlacDecoder::FlacDecoder() : AbstractBaseDecoder(DECODER_NAME)
{
	this->SetDecoderIsOpenStatus(false);
}

/// <summary>
/// Purpose: With-args Constructor that opens the Decoder using the specified File. Uses Constructor Chaining (i.e. Constructor Delegation)
/// </summary>
/// <param name="filenamePtr"></param>
/// <param name="memoryBufferIsEnabled"></param>
FlacDecoder::FlacDecoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled) : FlacDecoder()
{
	this->m_decoderMemoryBufferIsEnabled = memoryBufferIsEnabled;

	// Open the FLAC File.
	this->OpenFile(filenamePtr, memoryBufferIsEnabled);
}

/// <summary>
/// Purpose: Destructor
/// </summary>
FlacDecoder::~FlacDecoder()
{
	if (this->DecoderIsOpen())
	{
		// Calling the finish() function flushes the decoding buffer, releases resources, resets the decoder settings to their defaults, 
		// and returns the decoder state to FLAC__STREAM_DECODER_UNINITIALIZED.
		// In addition, the function resets all settings to the constructor defaults, including the callbacks.
		this->FLAC::Decoder::Stream::finish();
		this->m_stream.Close(); // Close the File Stream.
	}
}

#pragma region Overridden_Base_Class_Functions_Region

long long FlacDecoder::Read()
{
	this->m_lastDecodedFrameSampleSize = 0LL;

	// FLAC::Decoder::Stream::process_single() - Tells the decoder to process at
	// most one metadata block or audio frame and return, calling either the
	// metadata callback or write callback, respectively, once. If the decoder
	// loses sync it will return with only the error callback being called.

	// Decode one MetaData Block or Audio Frame.
	bool decodeOperationWasSuccessful = this->FLAC::Decoder::Stream::process_single();

	if (!decodeOperationWasSuccessful || this->m_errorCallbackFlagEnabled)
	{
		// Decoding error occurred.

		this->m_lastDecodedFrameSampleSize = -1LL; // Flag that an error occurred.

		if (!this->m_errorCallbackFlagEnabled)
		{
			switch (this->FLAC::Decoder::Stream::get_state())
			{
				case FLAC__STREAM_DECODER_END_OF_STREAM:
					this->Truncated(this->m_errorMessage);
					break;
				case FLAC__STREAM_DECODER_SEEK_ERROR:
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"SEEK_ERROR");
					break;
				case FLAC__STREAM_DECODER_ABORTED:
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"DECODER_ABORTED");
					break;
				case FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR:
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"MEMORY_ALLOCATION_ERROR");
					break;
				case FLAC__STREAM_DECODER_OGG_ERROR:
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"OGG_LAYER_ERROR");
					break;
				default:
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"DECODER_ERROR");
			}
		}
		else
		{
			// Decoder has Lost Sync. The Error Callback Function has already been called.
			if (this->m_errorMessage == L"" || this->m_errorMessage == nullptr)
			{
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"DECODER_LOST_SYNC");
			}
		}
	}
	else
	{
		// Decoding was successful.

		if (this->FLAC::Decoder::Stream::get_state() == FLAC__STREAM_DECODER_END_OF_STREAM)
		{
			if (!this->FLAC::Decoder::Stream::finish())
			{
				// If MD5 Checking is enabled, the FLAC::Decoder::Stream::finish() function will report when the decoded MD5 signature 
				// does not match the one stored in the STREAMINFO block.
				// 
				// The FLAC::Decoder::Stream::finish() function returns FALSE when ALL of the following conditions are met: 
				// 1. The MD5 checking is on.
				// 2. A STREAMINFO block was available AND The MD5 signature in the STREAMINFO block was non-zero.
				// 3. The MD5 signature does not match the one computed by the Decoder.
				
				this->m_lastDecodedFrameSampleSize = -1LL; // Flag that an error occurred.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"MD5_MISMATCH");
			}

			// Check that the Decoded data matches the expected size.
			if (this->m_totalDecodedFrameSampleCount != this->m_streamTotalSampleCount)
			{
				this->m_lastDecodedFrameSampleSize = -1LL; // Flag that an error occurred.
				if (this->m_totalDecodedFrameSampleCount < this->m_streamTotalSampleCount)
				{
					// Less Decoded data than expected was encountered.
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"MISSING_SAMPLES");
				}
				else
				{
					// More Decoded data than expected was encountered.
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"EXTRA_SAMPLES");
				}
			}
		}
	}
	return this->m_lastDecodedFrameSampleSize;
}

unsigned long long FlacDecoder::GetDecodedAudioDataTotal()
{
	return this->m_streamTotalSampleCount;
}

const wchar_t* FlacDecoder::GetLastErrorMessage()
{
	return this->m_errorMessage;
}

const wchar_t* FlacDecoder::GetSupportedTypes()
{
	return FlacDecoder::FILE_EXTENSION_TYPES;
}

#pragma endregion Overridden_Base_Class_Functions_Region

#pragma region Overridden_Protected_FLAC_Decoder_Stream_Functions_Region

FLAC__StreamDecoderReadStatus FlacDecoder::read_callback(FLAC__byte buffer[], size_t* bytes)
{
	// Check the value of the size of the buffer.
	if (*bytes > 0)
	{
		// Check for EOF before Reading from Stream.
		if (this->m_stream.EndOfFile())
		{
			// The read was attempted while at the end of the stream.
			*bytes = 0;
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}

		// Read the Stream and update the value stored at the pointer location.
		size_t readDataSize = this->m_stream.Read(buffer, *bytes);

		if (readDataSize == 0)
		{
			// The read was attempted while at the end of the stream.
			*bytes = 0;
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}
		else
		{
			// Update the value stored at the pointer location.
			*bytes = readDataSize;

			// The read was OK and decoding can continue.
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
	{
		// An unrecoverable error occurred. The Decoder will return from the process call.
		*bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
}

FLAC__StreamDecoderWriteStatus FlacDecoder::write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[])
{
	// Sets the Frame Size (i.e. The number of samples per Subframe).
	// All Subframes within a Frame will contain the same number of samples.
	// URI: https://xiph.org/flac/api/structFLAC____Frame.html
	// URI: https://xiph.org/flac/format.html#frame
	// 
	// URI: https://xiph.org/flac/api/structFLAC____FrameHeader.html
	// URI: https://xiph.org/flac/format.html#frame_header

	FLAC__FrameHeader frameHeader = frame->header;

	// Get the number of decoded samples per Decoded Subframe.
	this->m_lastDecodedFrameSampleSize = frameHeader.blocksize;

	// Add the number of decoded Subframe Samples to the total Decoded Frame Samples.
	this->m_totalDecodedFrameSampleCount += this->m_lastDecodedFrameSampleSize;

	// The write was OK and Decoding can continue.
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacDecoder::error_callback(FLAC__StreamDecoderErrorStatus status)
{
	this->m_errorCallbackFlagEnabled = true; // Set the flag indicating that the Error Callback Function has been called.

	std::wostringstream woss;

	unsigned long long totalDecodedStreamTimeSeconds = (this->m_streamSampleRate > 0) ? this->m_totalDecodedFrameSampleCount / this->m_streamSampleRate : 0;
	unsigned long long minutes = totalDecodedStreamTimeSeconds / 60;
	unsigned long long seconds = totalDecodedStreamTimeSeconds % 60;

	switch (status)
	{
		case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
		{
			// An error in the stream caused the decoder to lose synchronization.
			if (this->m_streamTotalSampleCount > 0ULL && this->m_streamSampleRate > 0UL)
			{
				// Get the total time in seconds for the entire duration of Stream.
				unsigned long long totalStreamTimeSeconds = this->m_streamTotalSampleCount / this->m_streamSampleRate;

				// Check if the reported Decoded Stream Time matches the Total Stream Time.
				if (totalStreamTimeSeconds == totalDecodedStreamTimeSeconds)
				{
					// An error caused the decoder to lose synchronization at the end of the Stream.

					// Check the FLAC file for the existence of a non-standard ID3v1 Tag embedded at the end of the File.
					if (this->hasID3v1Tag())
					{
						// An ID3v1 Tag was found at the end of the Stream.
						woss << L"<LOST_SYNC @ " << minutes << L"m " << seconds << L"s>" << L" <ID3v1_TAG_FOUND>";
					}
					else
					{
						// An IDv3Tag was NOT found at the end of the Stream.
						// A different error caused the decoder to lose synchronization.
						woss << L"LOST_SYNC @ " << minutes << L"m " << seconds << L"s";
					}
				}
				else
				{
					// An error caused the decoder to lose synchronization before the end of the Stream.
					woss << L"LOST_SYNC @ " << minutes << L"m " << seconds << L"s";
				}
			}
			else
			{
				// An unknown error caused the decoder to lose synchronization.
				woss << L"LOST_SYNC @ " << minutes << L"m " << seconds << L"s";
			}

			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, woss.str().c_str());
			break;
		}
		case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
		{
			// The decoder encountered a corrupted frame header.
			woss << L"BAD_HEADER @ " << minutes << L"m " << seconds << L"s";
			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, woss.str().c_str());
			break;
		}
		case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
		{
			// The frame's data did not match the CRC in the footer.
			woss << L"FRAME_CRC_MISMATCH @ " << minutes << L"m " << seconds << L"s";
			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, woss.str().c_str());
			break;
		}
		case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
		{
			// The decoder encountered reserved fields in use in the stream.
			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"UNPARSEABLE_STREAM");
			break;
		}
		case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_METADATA:
		{
			// The decoder encountered a corrupted metadata block.
			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_METADATA");
			break;
		}
	}
}

FLAC__StreamDecoderSeekStatus FlacDecoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if (this->m_stream.Seek(absolute_byte_offset, SEEK_SET))
	{
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}
	else
	{
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}
}

FLAC__StreamDecoderTellStatus FlacDecoder::tell_callback(FLAC__uint64* absolute_byte_offset)
{
	*absolute_byte_offset = this->m_stream.Tell();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus FlacDecoder::length_callback(FLAC__uint64* stream_length)
{
	*stream_length = this->m_stream.Length();
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool FlacDecoder::eof_callback(void)
{
	return this->m_stream.EndOfFile();
}

void FlacDecoder::metadata_callback(const FLAC__StreamMetadata* metadata)
{
	// URI: https://xiph.org/flac/format.html#metadata_block_streaminfo
	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		// The Sample Rate in Hz for the Stream.
		// The Valid Range: 1Hz - 655350Hz.
		// A returned value of zero means the number of total samples is unknown.
		this->m_streamSampleRate = metadata->data.stream_info.sample_rate;
		
		// Total Samples in the Stream.
		// For example, one second of 44.1Khz audio will have 44100 samples regardless of the number of channels.
		// A returned value of zero means the number of total samples is unknown.
		this->m_streamTotalSampleCount = metadata->data.stream_info.total_samples;
	}
}

#pragma endregion Overridden_Protected_FLAC_Decoder_Stream_Functions_Region

#pragma region Private_Member_Functions_Region

void FlacDecoder::OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled)
{
	if (this->m_stream.Open(filenamePtr, memoryBufferIsEnabled))
	{
		// The decoder will compute the MD5 signature of the unencoded audio data while decoding and 
		// compare it to the signature from the STREAMINFO block, if it exists, during FLAC::Decoder::Stream::finish().

		//	The stream decoder provides MD5 signature checking. If this is
		//	turned on before initialization, FLAC::Decoder::Stream::finish() will
		//	report when the decoded MD5 signature does not match the one stored
		//	in the STREAMINFO block. 
		//	
		//	MD5 checking is automatically turned off
		//	(until the next FLAC__stream_decoder_reset()) if there is no signature
		//	in the STREAMINFO block or when a seek is attempted.

		this->FLAC::Decoder::Stream::set_md5_checking(true);

		// The FLAC::Decoder::Stream::init() function initializes the Decoder to decode native FLAC streams. 
		// I/O is performed via callbacks to the Client, instead of decoding from a plain native FLAC file via filename or open FILE*.

		FLAC__StreamDecoderInitStatus decoderInitStatus = this->FLAC::Decoder::Stream::init();

		if (decoderInitStatus == FLAC__STREAM_DECODER_INIT_STATUS_OK)
		{
			// Initialization was successful, now check the MetaData.

			// FLAC::Decoder::Stream::process_until_end_of_metadata() - Tells the decoder
			// to process the stream from the current location and stop upon reaching
			// the first Audio Frame. The Client will get one metadata_callback(), write_callback(), or error_callback()
			// per Metadata Block, Audio Frame, or Sync Error, respectively.

			// Returns FALSE if any fatal read, write, or memory allocation error occurred (meaning decoding must stop).

			bool decodingMetaDataWasSuccessful = this->FLAC::Decoder::Stream::process_until_end_of_metadata();

			if (decodingMetaDataWasSuccessful)
			{
				this->m_totalDecodedFrameSampleCount = 0ULL;
				this->SetDecoderIsOpenStatus(true);
			}
			else
			{
				// A problem occurred decoding the MetaData.
				this->SetDecoderIsOpenStatus(false);
				this->FLAC::Decoder::Stream::finish();
				this->m_stream.Close(); // Close the File Stream.
			}
		}
		else
		{
			// An initialization problem occurred.
			this->SetDecoderIsOpenStatus(false);
			this->m_stream.Close(); // Close the File Stream.
		}
	}
}

void FlacDecoder::Truncated(wchar_t errorMessage[]) const
{
	std::wostringstream woss;

	unsigned long long totalDecodedStreamTimeSeconds = (this->m_streamSampleRate > 0) ? this->m_totalDecodedFrameSampleCount / this->m_streamSampleRate : 0;
	unsigned long long minutes = totalDecodedStreamTimeSeconds / 60;
	unsigned long long seconds = totalDecodedStreamTimeSeconds % 60;

	woss << L"TRUNCATED @" << minutes << L"m " << seconds << L"s";
	wcscpy_s(errorMessage, MAX_ERROR_SIZE, woss.str().c_str());
}

bool FlacDecoder::hasID3v1Tag()
{
	// Initialize the offset.
	long long offset = 0LL;

	// Check for ID3v1 Tag.
	// An ID3v1 Tag is stored in a fixed 128-byte segment at the end of the Stream.
	// Start seeking from the end of the Stream, backwards 128 bytes.
	if (this->m_stream.Seek(-128LL, SEEK_END) == 0)
	{
		// Seek operation was successful.
		// Check for ID3v1 Tag, starting at the -128 bytes position within the Stream.
		char id3[4]{};
		this->m_stream.Read(id3, 3);
		if (strcmp(id3, "TAG") == 0)
		{
			// An ID3v1 Tag was found.
			return true;
		}
	}

	return false;
}

#pragma endregion Private_Member_Functions_Region