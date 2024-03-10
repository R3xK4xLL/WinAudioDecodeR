#include "MP3Decoder.h"

// ****************************************************************************
//							MP3 Decoder
// ****************************************************************************

/// <summary>
/// Purpose: A namespace containing supporting Data for the MP3 Decoder.
/// </summary>
namespace MP3DecoderSupport
{
	/// <summary>
	/// Purpose: Contains all of the commonly supported MP3 Bitrates (Excludes the Free and Bad Rows).
	/// Columns: [0] {MPEG-1 Layer-I}, [1] {MPEG-1 Layer-II}, [2] {MPEG-1 Layer-III}, [3] {MPEG-2 Layer-I}, [4] {MPEG-2 Layer-II & Layer-III}
	/// </summary>
	static const unsigned long MP3_BITRATES[16][5] =
	{
		{0UL, 0UL, 0UL, 0UL, 0UL},	// Free (Free Format)
		{32UL, 32UL, 32UL, 32UL, 8UL},
		{64UL, 48UL, 40UL, 48UL, 16UL},
		{96UL, 56UL, 48UL, 56UL, 24UL},
		{128UL, 64UL, 56UL, 64UL, 32UL},
		{160UL, 80UL, 64UL, 80UL, 40UL},
		{192UL, 96UL, 80UL, 96UL, 48UL},
		{224UL, 112UL, 96UL, 112UL, 56UL},
		{256UL, 128UL, 112UL, 128UL, 64UL},
		{288UL, 160UL, 128UL, 144UL, 80UL},
		{320UL, 192UL, 160UL, 160UL, 96UL},
		{352UL, 224UL, 192UL, 176UL, 112UL},
		{384UL, 256UL, 224UL, 192UL, 128UL},
		{416UL, 320UL, 256UL, 224UL, 144UL},
		{448UL, 384UL, 320UL, 256UL, 160UL},
		{0UL, 0UL, 0UL, 0UL, 0UL}	// Bad (Not an allowed value)
	};

	/// <summary>
	/// Purpose: Contains all of the commonly supported MP3 Sample Rates (values are in Hz).
	/// Columns: [0] {MPEG-1}, [1] {MPEG-2}, [2] {MPEG-2.5}
	/// </summary>
	static const unsigned long MP3_SAMPLE_RATES[4][3] =
	{
		{44100UL, 22050UL, 11025UL},
		{48000UL, 24000UL, 12000UL},
		{32000UL, 16000UL, 8000UL},
		{0UL, 0UL, 0UL}	// Reserved
	};
	
	/// <summary>
	/// Purpose: Used for verifying CRC Data in Audio Frames that have the CRC Protection Bit enabled. 
	/// Contains the known CRC Byte Sizes for the various MPEG-1/MPEG-2/MPEG-2.5 Layer III file types.
	/// 
	/// Row 0: Either Stereo or Joint Stereo (Stereo) or Dual channel (Stereo) Mode.
	/// Row 1: Mono Channel Mode.
	/// 
	/// Column 0: MPEG-1
	/// Column 1: MPEG-2/MPEG-2.5
	/// 
	/// URI: http://www.codeproject.com/audio/MPEGAudioInfo.asp#CRC
	/// </summary>
	static const unsigned long MP3_CRC_BYTE_SIZE[2][2] =
	{
		{32UL, 17UL}, 
		{17UL, 9UL}
	};

	typedef struct
	{
		char ID[8];              // The ID of the APE Tag Footer. Should equal 'APETAGEX'.
		unsigned long Version;   // Equals CURRENT_APE_TAG_VERSION.
		unsigned long Size;      // The complete size of the Tag, including this Footer (Note: excludes Header).
		unsigned long Fields;    // The number of Fields in the Tag.
		unsigned long Flags;     // The APE Tag flags.
		char Reserved[8];        // Reserved for later use (must be zero).
	} APE_TAG_FOOTER;
}

MP3Decoder::MP3Decoder() : AbstractBaseDecoder(DECODER_NAME)
{
	this->SetDecoderIsOpenStatus(false);
}

/// <summary>
/// Purpose: With-args Constructor that opens the Decoder using the specified File. Uses Constructor Chaining (i.e. Constructor Delegation)
/// </summary>
/// <param name="filenamePtr"></param>
/// <param name="memoryBufferIsEnabled"></param>
MP3Decoder::MP3Decoder(const wchar_t* filenamePtr, bool memoryBufferIsEnabled) : MP3Decoder()
{
	this->m_decoderMemoryBufferIsEnabled = memoryBufferIsEnabled;

	// Open the MP3 File.
	this->OpenFile(filenamePtr, memoryBufferIsEnabled);
}

MP3Decoder::~MP3Decoder()
{
	if (this->DecoderIsOpen())
	{
		this->m_stream.Close();
	}
}

#pragma region Overridden_Base_Class_Functions_Region

long long MP3Decoder::Read()
{

	// Read() 1-byte of data from the Stream at a time into the Buffer, starting from the 4th byte, in reverse order. (4-bytes total)
	// Check for the 'unsigned long' wrap-around condition with the std::numeric_limits<T>::max() function.
	// NOTE: The Loop stops when the loop variable wraps around to the maximum value of an 'unsigned long' due to underflow (which happens when you subtract 1 from 0 for an 'unsigned long').
	// For clarity, this for-loop is logically equivalent to the more traditional decrement-style for-loop: 'for (int loopVar = 3; loopVar >=0; --loopVar)'.
	for (this->m_currentFrameLength = 3UL; this->m_currentFrameLength != std::numeric_limits<unsigned long>::max(); --this->m_currentFrameLength)
	{
		void* readBufferPtr = reinterpret_cast<unsigned char*>(&this->m_currentFrameHeader) + this->m_currentFrameLength;
		size_t numberOfBytesRead = this->m_stream.Read(readBufferPtr, 1);
		if (numberOfBytesRead == 0)
		{
			// If the Read() operation fails before reading 4-bytes, set an error message and return.
			wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"LOST_SYNC @ END_OF_FILE");
			return -1L; // An error occurred.
		}
	}

	// After reading 4-bytes (32-bits) in reverse order within the Stream, the beginning of the current Frame Header is set and ready for processing.

	// Get the current Frame Length.
	this->m_currentFrameLength = this->GetFrameLength();
	
	if (this->m_currentFrameLength != 0UL)
	{
		// Use the Bitmask to extract the Bitrate, Sampling Rate, and Channel Mode, from both of the Frame Headers.
		// Compare if those fields are identical OR the last Frame Header read is zero.
		if ( ((this->m_currentFrameHeader & 0xFFFE0C00) == (this->m_previousFrameHeader & 0xFFFE0C00)) || (this->m_previousFrameHeader == 0UL) )
		{
			if (this->m_previousFrameHeader == 0UL)
			{
				this->m_previousFrameHeader = this->m_currentFrameHeader; // Update the previous Frame Header that is stored.
				this->m_streamSampleRate = this->m_sampleRate; // Update the reported Sample Rate for entire the Stream.
			}

			// Update the offset position within the Stream.
			this->m_offset += this->m_currentFrameLength;
			
			if (this->m_offset < this->m_fileSizeInBytes)
			{
				// The offset is within the File Size.
				if (this->CheckCRCProtection())
				{
					// CRC Check passed, seek to the offset position within the Stream.
					if (this->m_stream.Seek(this->m_offset, SEEK_SET) != 0)
					{
						return -1LL; // A seek error occurred.
					}

					// Return the number of Decoded Audio Units read.
					return this->m_currentFrameLength;
				}
				else
				{
					return -1LL; // An error occurred.
				}
			}
			else if (this->m_offset == this->m_fileSizeInBytes)
			{
				// The offset is equal to the File Size.
				if (this->CheckCRCProtection())
				{
					return 0LL; // EOF
				}
				else
				{
					return -1LL; // An error occurred
				}
			}
			else if (this->m_offset > this->m_fileSizeInBytes)
			{
				// The offset exceeds the File Size.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"TRUNCATED");
			}
		}
	}

	// Check if an error previously occurred.
	size_t errorMessageSize = wcslen(this->m_errorMessage);
	if (errorMessageSize == 0)
	{
		// Check the current Frame Sample Position within the Stream.
		if (this->m_frameSamplePosition > 0UL)
		{
			// The Frame Sample Position is NOT at the start of the Stream.
			// Attempt resynchronization of the bitstream.
			if (this->ResynchronizeBitstream())
			{
				// Resynchronization of the bitstream was successful.
				float timeDuration = static_cast<float>(this->m_frameSamplePosition) / this->m_streamSampleRate;
				swprintf_s(this->m_errorMessage, MAX_ERROR_SIZE, L"LOST_SYNC @ %dm %02ds", ((int)timeDuration) / 60, ((int)timeDuration) % 60);
			}
			else
			{
				// Resynchronization of the bitstream failed.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"LOST_SYNC @ END_OF_FILE");
			}
		}
		else
		{
			// The Frame Sample Position is at the start of the Stream.
			// Attempt resynchronization of the bitstream.
			if (!this->ResynchronizeBitstream())
			{
				// Resynchronization of the bitstream failed.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"UNRECOGNIZED_FORMAT");
			}
			else
			{
				// Resynchronization of the bitstream was successful.
				
				// Check if there are Tag Header bytes at the start of the Stream.
				if (this->m_tagHeaderLengthBytes > 0UL)
				{
					// Tag Header bytes exist at the start of the Stream.
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_ID3v2_TAG");
				}
				else
				{
					// Tag Header bytes do NOT exist at the start of the Stream.
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_STARTING_SYNC");
				}
			}
		}
	}

	return -1LL; // An error occurred.
}

unsigned long long MP3Decoder::GetDecodedAudioDataTotal()
{
	return static_cast<unsigned long long>(this->m_fileSizeInBytes);
}

const wchar_t* MP3Decoder::GetLastErrorMessage()
{
	return this->m_errorMessage;
}

const wchar_t* MP3Decoder::GetSupportedTypes()
{
	return MP3Decoder::FILE_EXTENSION_TYPES;
}

#pragma endregion Overridden_Base_Class_Functions_Region

#pragma region Private_Member_Functions_Region

unsigned long MP3Decoder::GetID3v2TagHeaderLength()
{
	// Read the 'ID3v2 Tag Header' bytes.
	unsigned long headerLength = 0UL;
	char buffer[10]{};
	if (this->m_stream.isOpen() && this->m_stream.Read(buffer, 10) == 10)
	{
		// The 10 bytes of the 'ID3v2 Tag Header' were successfully read into the buffer.
		// Check if the first three bytes in the buffer form the string “ID3” (indicating an ID3v2 tag). [byte 0 - byte 2]
		if ((buffer[0] == 'I') && (buffer[1] == 'D') && (buffer[2] == '3'))
		{
			// These conditions ensure that certain bytes within the Header, are within specific ranges. [byte 3 - byte 9]
			if ((buffer[3] < 0xff) && (buffer[4] < 0xff) && (buffer[6] < 0x80) && (buffer[7] < 0x80) && (buffer[8] < 0x80) && (buffer[9] < 0x80))
			{
				//  Combine the values of 'byte 6', 'byte 7', 'byte 8', and 'byte 9' to calculate the header length. [byte 6 - byte 9]
				headerLength = buffer[6];
				headerLength <<= 7UL;
				headerLength += buffer[7];
				headerLength <<= 7UL;
				headerLength += buffer[8];
				headerLength <<= 7UL;
				headerLength += buffer[9];
				
				// Check 'byte 3' and 'byte 5' (uses a Bitmask for 'byte 5' to examine a specific bit).
				// NOTE: 'byte 5' is the ID3v2 TagHeaderFlags bitfield. Bits 3-7 in the bitfield are reserved for future use (they are usually set to 0).
				// 
				// (If 'byte 3' is equal to the value of 4) 
				// AND 
				// (Perform a Bitwise AND (&) operation to check if the 5th bit contained within 'byte 5' is specifically set to 1, Bitmask 0x10 = 0001 0000 in binary), 
				// TRUE add 20 the 'ID3v2 Tag Header' length; FALSE, add 10 to the 'ID3v2 Tag Header' length.
				headerLength += ((buffer[3] == 4) && (buffer[5] & 0x10)) ? 20UL : 10UL;
			}

			if (headerLength == 0UL)
			{
				// If 'ID3v2 Tag Header' Length remains 0 after the checks, create an Error message.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_ID3v2_TAG");
			}
		}
	}

	// The calculated 'ID3v2 Tag Header' Length in bytes.
	return headerLength;
}

unsigned long MP3Decoder::GetTagFooterLength()
{
	// Initialize the offset.
	// The offset is used to keep track of the total length in bytes, of all the Tags found in the Footer.
	this->m_offset = 0LL;

	// Check for ID3v1 Tag.
	// An ID3v1 Tag is stored in a fixed 128-byte segment at the end of the Stream.
	// Start seeking from the end of the Stream, backwards 128 bytes.
	if (this->m_stream.Seek(-128LL, SEEK_END) == 0)
	{
		// Seek operation was successful.
		
		// Check for ID3v1 Tag, starting at the -128 bytes position within the Stream.
		char id3[4]{};
		this->m_stream.Read(id3, 3);
		if (strcmp(id3,"TAG") == 0)
		{
			// An ID3v1 Tag was found.
			// Reduce the offset by 128 bytes (offset == -128 bytes).
			this->m_offset -= 128LL;

			// Check for LYRICS3 tag.
			// Temporarily reduce the offset by 9 bytes (offset == -137 bytes), then Seek backwards 9 bytes.
			if (this->m_stream.Seek(this->m_offset - 9LL, SEEK_END) == 0)
			{
				// Reduce the offset by the length of the Lyrics bytes.
				this->m_offset -= this->GetLyricsTagLength();
			}
		}
	}

	// At this point, if an ID3v1 Tag was previously found the offset is either -128 bytes or -137 bytes.
	// Otherwise, if an ID3v1 Tag was NOT found, the offset is 0 bytes.

	// Check for an APE Tag.
	if (this->m_stream.Seek(this->m_offset - APE_TAG_FOOTER_BYTES, SEEK_END) == 0)
	{
		// An APE tag is stored in a variable-length segment at the end of the Stream, and its length in bytes, is stored in the 'APE Tag Footer' size.

		// A Struct used to store APE TAG Footer bytes from the Stream.
		MP3DecoderSupport::APE_TAG_FOOTER tagFooterStruct{};

		// Read the size of the 'APE Tag Footer Bytes' (32 bytes) into the 'APE Tag Footer Struct' (Struct size is 32 bytes). 
		// Afterwards, Confirm that 32 Bytes were read were successfully read the Stream.
		// AND Check that the 'APE Tag Footer ID' matches the expected value.
		if ( (this->m_stream.Read(&tagFooterStruct, APE_TAG_FOOTER_BYTES) == APE_TAG_FOOTER_BYTES) && (strncmp(tagFooterStruct.ID, APE_TAG_FOOTER_ID, 8) == 0) )
		{
			// The APE Tag Footer Bytes were read successfully AND an 'APE Tag Footer ID' match was found, therefore an APE Tag was found.
			
			// Check the boundary conditions for the expected complete size of the 'APE Tag Footer', excluding the Header.
			if (tagFooterStruct.Size < APE_TAG_FOOTER_BYTES || tagFooterStruct.Size > this->m_fileSizeInBytes)
			{
				// The 'APE Tag Footer' is NOT the expected size. Therefore, a bad APE Tag was encountered.
				wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_APE_TAG");
			}
			else
			{
				// The 'APE Tag Footer' size in bytes, is within the expected size range in bytes.
				// Reduce the offset by the size of bytes in the APE Tag.
				this->m_offset -= tagFooterStruct.Size;
				
				// Check if an APE Tag flag is set for containing a Header.
				if (tagFooterStruct.Flags & APE_TAG_FLAG_CONTAINS_HEADER_MASK)
				{
					// The APE Tag contains a Header.
					// Reduce the offset by 'APE Tag Footer Bytes' (32 bytes).
					this->m_offset -= APE_TAG_FOOTER_BYTES;
				}

				// Check for LYRICS tag.
				// Temporarily reduce the offset by 9 bytes, then Seek backwards 9 bytes.
				if (this->m_stream.Seek(this->m_offset - 9LL, SEEK_END) == 0)
				{
					// Reduce the offset by the length of the Lyrics bytes.
					this->m_offset -= this->GetLyricsTagLength();
				}
			}
		}
	}

	// At this point, the offset value represents the total length in bytes, of all the tags found in the Footer.
	// Subtract either 0 or a negative offset byte value, resulting in either 0 or a positive byte value for the Footer length.
	unsigned long footerLength = static_cast<unsigned long>( 0LL - this->m_offset );
	return footerLength;
}

unsigned long MP3Decoder::GetLyricsTagLength()
{
	char tagNameBuffer[12] = {0};

	// Read 9 characters from the Stream into the Buffer.
	if (this->m_stream.Read(tagNameBuffer, 9) == 9)
	{
		// Check for a LYRICS3v1 Tag. The Lyrics3v1 block ends with the string "LYRICSEND".
		if (strcmp(tagNameBuffer, "LYRICSEND") == 0)
		{
			const long long searchBufferSize = 5100LL;
			// Seek backwards -5100 Characters from the current position in the Stream.
			if (this->m_stream.Seek(-searchBufferSize, SEEK_CUR) == 0)
			{
				// Read 5100 characters into the search Buffer.
				char searchBuffer[searchBufferSize] = {0};
				if (this->m_stream.Read(searchBuffer, searchBufferSize) == searchBufferSize)
				{
					// Search for String.
					// URI: https://cplusplus.com/reference/cstring/strstr/

					// A Lyrics3v1 block begins with the word "LYRICSBEGIN".
					char* searchResultPositionPtr = strstr(searchBuffer, "LYRICSBEGIN");
					if (searchResultPositionPtr != nullptr)
					{
						// A match was found. 
						// The pointer is set to the first occurrence of "LYRICSBEGIN" within the Buffer.

						// Lyrics Offset = Start Position of the Lyrics - Start Position of the Buffer
						long long lyricsOffset = searchResultPositionPtr - searchBuffer;

						// Lyrics Length = Buffer Size - Buffer Offset
						long long lyricsLength = searchBufferSize - lyricsOffset;
						return static_cast<int>(lyricsLength);
					}
					else
					{
						// A match was NOT found.
						wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_LYRICS3v1_TAG");
					}
				}
			}
		}
		else
		{
			if ((strcmp(tagNameBuffer, "LYRICS200") == 0) // Check for a LYRICS3v2 Tag Name. The Lyrics3v2 block ends with the string "LYRICS200".
				&& (this->m_stream.Seek(-15, SEEK_CUR) == 0) // And then seek() -15 Characters backwards from the current position in the Stream.
				&& (this->m_stream.Read(tagNameBuffer, 6) == 6)) // And finally, read() 6 Characters from Stream into the Buffer.
			{
				// Convert the 6 read Characters into an integer representing the Lyrics length.
				// The Lyrics length includes the "LYRICSBEGIN" and "LYRICS200" strings.
				tagNameBuffer[6] = 0; // Null-terminate the buffer.
				int lyricsLength = atoi(tagNameBuffer);

				if ((lyricsLength != 0)
					&& (!this->m_stream.Seek(-(6 + static_cast<long long>(lyricsLength)), SEEK_CUR)) // Seek backwards -(6 characters + lyricsLength).
					&& (this->m_stream.Read(tagNameBuffer, 11) == 11) // And then read() 11 Characters from the Stream to get the Tag Name.
					&& (!strcmp(tagNameBuffer, "LYRICSBEGIN"))) // And finally, check for a LYRICS3v2 Tag Name. The Lyrics3v2 block begins with the word "LYRICSBEGIN".
				{
					return lyricsLength + 15;
				}
				else
				{
					wcscpy_s(this->m_errorMessage, MAX_ERROR_SIZE, L"BAD_LYRICS3v2_TAG");
				}
			}
		}
	}
	return 0UL;
}

unsigned long MP3Decoder::GetFrameLength()
{
	if (this->m_currentFrameHeader > 0xFFE00000)
	{
		// Read the Frame Header contents:

		// Bit-shift 19-bits to the right and set the value for the MPEG Audio Encoding Version. (BitMask 0x03 = 0000 0011)
		// 0x00 = 0000 0000 = MPEG Version 2.5
		// 0x01 = 0000 0001 = Reserved
		// 0x02 = 0000 0010 = MPEG Version 2 (ISO/IEC 13818-3)
		// 0x03 = 0000 0011 = MPEG Version 1 (ISO/IEC 11172-3)
		this->m_encodingVersion = (this->m_currentFrameHeader >> 19UL) & 0x03;
		
		// Bit-shift 19-bits to the right and set the value for the Layer Description. (BitMask 0x03 = 0000 0011)
		// 0x00 = 0000 0000 = Reserved
		// 0x01 = 0000 0001 = Layer III
		// 0x02 = 0000 0010 = Layer II
		// 0x03 = 0000 0011 = Layer I
		this->m_layerDescription = (this->m_currentFrameHeader >> 17UL) & 0x03;
		
		// Bit-shift 12-bits to the right and set the value for the Bitrate. (BitMask 0x0F = 0000 1111)
		this->m_bitrate = (this->m_currentFrameHeader >> 12UL) & 0x0F;

		// Bit-shift 10-bits to the right and set the value for the Sampling Rate Frequency. (BitMask 0x03 = 0000 0011)
		this->m_sampleRate = (this->m_currentFrameHeader >> 10UL) & 0x03;
		
		// Bit-shift 9-bits to the right, and sets the value for the Padding Bit. (BitMask 0x01 = 0000 0001)
		// 0x00 = 0000 0000 = Frame is NOT Padded.
		// 0x01 = 0000 0001 = Frame is Padded with one extra slot. For Layer I a slot is 32-bits long, for Layer II and Layer III a slot is 8-bits long.
		this->m_framePadding = (this->m_currentFrameHeader >> 9UL) & 0x01;
		
		// Bit-shift 6-bits to the right, and set the value for the Channel Mode (Explicitly checks for Single Channel Mono Mode with the Bitmask). (BitMask 0x03 = 0000 0011)
		// 0x00 = 0000 0000 = Stereo
		// 0x01 = 0000 0001 = Joint Stereo (Stereo)
		// 0x02 = 0000 0010 = Dual Channel (Stereo)
		// 0x03 = 0000 0011 = Single Channel (Mono)
		this->m_singleChannelMono = ((this->m_currentFrameHeader >> 6UL) & 0x03) == 0x03;
		
		// Bit-shift 16-bits to the right, and sets the value for the CRC Protection Bit. (BitMask 0x01 = 0000 0001)
		// 0x00 = 0000 0000 = Protected by CRC (16bit CRC Checksum value follows Frame Header).
		// 0x01 = 0000 0001 = Not Protected
		this->m_embeddedFrameCRC = ((this->m_currentFrameHeader >> 16UL) & 0x01);

		this->m_columnIndex = 5UL;

		// Adjust the Column Index used based upon the encoding version.
		switch (this->m_encodingVersion)
		{
			case MPEG1:
				switch (this->m_layerDescription)
				{
					case LAYER_1:
						this->m_columnIndex = 0UL;
						break;
					case LAYER_II:
						this->m_columnIndex = 1UL;
						break;
					case LAYER_III:
						this->m_columnIndex = 2UL;
						break;
				}
				break;
			case MPEG2:
				// Fall through.
			case MPEG2_5:
				switch (this->m_layerDescription)
				{
					case LAYER_1:
						this->m_columnIndex = 3UL;
						break;
					case LAYER_II:
						// Fall-through.
					case LAYER_III:
						this->m_columnIndex = 4UL;
						break;
				}
				break;
		}

		if (this->m_columnIndex < 5UL)
		{
			// Calculate the Bitrate in kilobits per second. (Bitrate Formula: Bits-per-second * Bit Rate)
			unsigned long bitsPerSecond = 1000UL;
			this->m_bitrate = bitsPerSecond * MP3DecoderSupport::MP3_BITRATES[this->m_bitrate][this->m_columnIndex];
		}
		else
		{
			return 0UL;
		}

		// Adjust the Column Index used based upon the encoding version.
		switch (this->m_encodingVersion)
		{
			case MPEG1:
				this->m_columnIndex = 0UL;
				break;
			case MPEG2:
				this->m_columnIndex = 1UL;
				break;
			case MPEG2_5:
				this->m_columnIndex = 2UL;
				break;
			default:
				return 0UL;
		}
		
		// Retrieve the Sampling Rate.
		this->m_sampleRate = MP3DecoderSupport::MP3_SAMPLE_RATES[this->m_sampleRate][this->m_columnIndex];

		if (this->m_sampleRate == 0UL)
		{
			// Reserved Sample Rate encountered.
			return 0UL;
		}

		// Use the Layer Description and the MPEG Audio Encoding Version to determine the Frame Length Formula used for calculating the Frame Length.
		switch (this->m_layerDescription)
		{
			case LAYER_1:
			{
				// Frame Length Formula (Layer I): FrameLengthInBytes = (12 * BitRate / SampleRate + Padding) * 4
				this->m_frameSamplePosition += 384UL;// Increment Frame Sample Position by the Frame Size Constant.
				unsigned long frameLength = (12UL * this->m_bitrate / this->m_sampleRate + this->m_framePadding) * 4UL;
				return frameLength;
			}
			case LAYER_II:
			{
				// Frame Length Formula (Layer II): FrameLengthInBytes = 144 * BitRate / SampleRate + Padding
				this->m_frameSamplePosition += 1152UL; // Increment Frame Sample Position by the Frame Size Constant.
				unsigned long frameLength = 144UL * this->m_bitrate / this->m_sampleRate + this->m_framePadding;
				return frameLength;
			}
			case LAYER_III:
				if (this->m_encodingVersion == MPEG1)
				{
					// MPEG-1
					// Frame Length Formula (Layer III): FrameLengthInBytes = 144 * BitRate / SampleRate + Padding
					this->m_frameSamplePosition += 1152UL; // Increment Frame Sample Position by the Frame Size Constant.
					unsigned long frameLength = 144UL * this->m_bitrate / this->m_sampleRate + this->m_framePadding;
					return frameLength;
				}
				else
				{
					// MPEG-2 or MPEG-2.5
					// Frame Length Formula (Layer III): FrameLengthInBytes = 72 * BitRate / SampleRate + Padding
					this->m_frameSamplePosition += 576UL; // Increment Frame Sample Position by the Frame Size Constant.
					unsigned long frameLength = 72UL * this->m_bitrate / this->m_sampleRate + this->m_framePadding;
					return frameLength;
				}
			default:
				return 0UL;
		}
	}
	else 
	{
		return 0UL;
	}
}

bool MP3Decoder::ResynchronizeBitstream()
{
	// Reset the Frame Header and Frame Length.
	this->m_currentFrameHeader = 0UL;
	this->m_currentFrameLength = 0UL;
	
	// Enter the Resynchronization Loop Algorithm. 
	// 1. Searches by reading in a fresh new byte from the Stream.
	// 2. Inserts the new byte into the Frame Header.
	// 3. Recalculates the new Frame Length for the Frame Header.
	// 4. Either Exits the loop when the conditions are satisfied OR determines that Resychronization when the Stream bytes are exhausted or there is Stream error.
	// NOTE: MAX_RESYNCHRONIZATION_ATTEMPTS is used to limit the maximum total number of resynchronization iterations.
	bool validFrameLengthWasFound = false;
	while (!validFrameLengthWasFound && --this->m_maxResynchronizationCount > 0UL)
	{
		// Bit-shifts the Frame Header 8-bits to the left. 
		// This effectively discards the oldest byte (on the left-side) and makes room for a new byte to be read in (on the right-side).
		this->m_currentFrameHeader <<= 8UL;

		// Read the next single byte into the Frame Header (on the right-side).
		if (this->m_stream.Read(reinterpret_cast<unsigned char*>(&this->m_currentFrameHeader), 1) != 0)
		{
			// Recalculate the Frame Length for the Frame Header using the newly inserted byte.
			this->m_currentFrameLength = this->GetFrameLength();

			if (this->m_currentFrameLength > 0UL)
			{
				validFrameLengthWasFound = true;
			}
		}
		else
		{
			// Resynchronization was NOT successful.
			return false; // (Base Case)
		}
	}

	// Check the Frame Length.
	if (this->m_currentFrameLength > 0UL)
	{
		// Check that a previous Frame Header exists.
		if (this->m_previousFrameHeader > 0UL)
		{
			// Compare the Previous Frame Header with the Frame Header (that has the new byte inserted).
			// Use the Bitmask to extract the Bitrate, Sampling Rate, and Channel Mode, from both of the Headers.
			// Compare if those fields are identical.
			if ( (this->m_currentFrameHeader & 0xFFFE0C00) == (this->m_previousFrameHeader & 0xFFFE0C00) )
			{
				// The Frame Headers match, therefore a valid new Frame Header was constructed from the data.
				// Resynchronization was successful.
				return true; // (Base Case)
			}
			else
			{
				// The Frame Headers do NOT match, therefore a valid new Frame Header was NOT constructed from the data.
				// Resynchronization was NOT successful.
				// Try another resychronization attempt.
				return this->ResynchronizeBitstream(); // Recursive Call. (Problem Reduction Step)
			}
		}
		else
		{
			// A previous Frame Header did NOT exist.
			// Update the previous Frame Header that is stored.
			this->m_previousFrameHeader = this->m_currentFrameHeader;

			// Resynchronization was NOT successful.
			// Try another resychronization attempt.
			return this->ResynchronizeBitstream(); // Recursive Call. (Problem Reduction Step)
		}
	}
	else
	{
		// Resynchronization was NOT successful.
		return false;  // (Base Case)
	}
}

bool MP3Decoder::CheckCRCProtection()
{
	// Check if CRC Protection is enabled for the Frame and that the layer is Layer III Encoding.
	if (this->m_embeddedFrameCRC == 0 && this->m_layerDescription == LAYER_III)
	{
		// Determine the number of CRC bytes for the combination of Channel Mode and Layer III Encoding Version.
		this->m_CRCByteSize = MP3DecoderSupport::MP3_CRC_BYTE_SIZE[this->m_singleChannelMono == 1 ? 1 : 0][this->m_encodingVersion == MPEG1 ? 0 : 1];
		
		// Seek backwards 2-bytes from the current position in the Stream.
		int seekSuccessful = this->m_stream.Seek(-2, SEEK_CUR);

		// Read 2-bytes from the Buffer.
		size_t initialbufferReadSize = this->m_stream.Read(this->m_bufferCRCBytes, 2);

		// Read 2-bytes (High + Low) to obtain the embedded CRC Checksum value for the Audio Frame.
		// Read 1-byte into the High Byte. (The High Byte must be read 1st)
		unsigned char* highBytePtr = reinterpret_cast<unsigned char*>(&this->m_embeddedFrameCRC) + 1; // Increment by 1-byte.
		size_t highByteReadSize = this->m_stream.Read(highBytePtr, 1);

		// Read 1-byte into the Low Byte. (The Low Byte must be read 2nd)
		unsigned char* lowBytePtr = reinterpret_cast<unsigned char*>(&this->m_embeddedFrameCRC);
		size_t lowByteReadSize = this->m_stream.Read(lowBytePtr, 1);

		// Read CRC bytes into the CRC Buffer starting at the third byte (skip over 2-bytes).
		size_t bufferCRCReadSize = this->m_stream.Read(this->m_bufferCRCBytes + 2, this->m_CRCByteSize);

		// Calculates the CRC of the data bytes using the CRC16() function and compares it to the CRC read from the file.
		unsigned short calculatedCRC = this->CRC16(this->m_bufferCRCBytes, static_cast<size_t>(this->m_CRCByteSize) + 2);

		if (seekSuccessful == 0 
			&& initialbufferReadSize == 2 
			&& highByteReadSize == 1
			&& lowByteReadSize == 1
			&& bufferCRCReadSize == this->m_CRCByteSize 
			&& calculatedCRC != this->m_embeddedFrameCRC)
		{
			float timeDuration = static_cast<float>(this->m_frameSamplePosition) / this->m_streamSampleRate;
			swprintf(this->m_errorMessage, MAX_ERROR_SIZE, L"CRC_ERROR @ %dm %02ds", (static_cast<int>(timeDuration)) / 60, (static_cast<int>(timeDuration)) % 60);
			return false;
		}
	}

	return true;
}

unsigned short MP3Decoder::CRC16(unsigned char* bufferDataPtr, size_t bufferLength)
{
	unsigned short crc = 0xFFFF; // Initial value.
	unsigned short polynomial = 0x8005; // A Polynomial commonly used with CRC-16 Algorithms.

	// Process each Byte of Data.
	for (size_t byteIndex = 0; byteIndex < bufferLength; ++byteIndex) {

		// For each Byte, XOR the CRC with the byte shifted left by 8-bits.
		crc ^= static_cast<unsigned short>(bufferDataPtr[byteIndex] << 8);
		
		// Process each Bit of data in the Byte. (Iterate 8 times, once for each individual bit).
		for (size_t bitIndex = 0; bitIndex < 8; ++bitIndex) {

			// Check the 15th bit of the CRC.
			if (crc & 0x8000)
			{
				// If the 15th bit is 1, then shift the CRC to the left by 1-bit and XOR it with the Polynomial.
				crc = (crc << 1) ^ polynomial;
			}
			else
			{
				// If the 15th bit is 0, then shift the CRC to the left by 1-bit.
				crc = crc << 1;
			}
		}
	}

	return crc;
}

void MP3Decoder::OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled)
{
	if (this->m_stream.Open(filenamePtr, memoryBufferIsEnabled))
	{
		// Set the File size using the Stream Length in Bytes.
		long long fileSizeInBytes = this->m_stream.Length();
		if (fileSizeInBytes >= 0LL)
		{
			this->m_fileSizeInBytes = fileSizeInBytes;

			// Check for the existence of any Header Tags or Footer Tags, and calculate both other their respective lengths in Bytes.
			this->m_tagHeaderLengthBytes = this->GetID3v2TagHeaderLength();
			this->m_tagFooterLengthBytes = this->GetTagFooterLength();

			// Reduce the File size bytes by the number of Tag Footer Bytes encountered.
			this->m_fileSizeInBytes -= this->m_tagFooterLengthBytes;

			// Seek from the beginning of the Stream.
			// Moves the Stream position to just past the length of an ID3v2 Tag Length.
			this->m_stream.Seek(this->m_tagHeaderLengthBytes, SEEK_SET);

			// Update the offset to reflect the current Stream position.
			this->m_offset = this->m_tagHeaderLengthBytes;

			this->m_frameSamplePosition = 0UL;
			this->m_previousFrameHeader = 0UL;
			this->m_maxResynchronizationCount = MAX_RESYNCHRONIZATION_ATTEMPTS;

			this->SetDecoderIsOpenStatus(true);
		}
		else
		{
			// A problem occurred.
			this->SetDecoderIsOpenStatus(false); // Set the Decoder Status as Closed.
			this->m_stream.Close(); // Close the File Stream.
		}
	}
}

#pragma endregion Private_Member_Functions_Region