#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include "PureAbstractBaseDecoder.h"
#include "AbstractBaseDecoder.h"
#include "StreamWrapper.h"
#include <string>

constexpr auto MPEG1 = 3;
constexpr auto MPEG2 = 2;
constexpr auto MPEG2_5 = 0;
constexpr auto LAYER_1 = 3;
constexpr auto LAYER_II = 2;
constexpr auto LAYER_III = 1;

/// <summary>
/// Purpose: Used to place a limit on the total number of resynchronization attempts.
/// </summary>
constexpr auto MAX_RESYNCHRONIZATION_ATTEMPTS = 65536UL;

constexpr auto APE_TAG_FOOTER_BYTES = 32LL;
constexpr auto APE_TAG_FOOTER_ID = "APETAGEX";

/// <summary>
/// Purpose:  A mask used to indicate whether an APE Tag contains a Header.
/// Bit shifts the number 1, 31 places to left. (00000000 00000000 00000000 00000001) ==> (10000000 00000000 00000000 00000000)
/// </summary>
constexpr unsigned long APE_TAG_FLAG_CONTAINS_HEADER_MASK = (1 << 31);

/// <summary>
/// Purpose: A Derived Class implementing a MP3 Decoder. 
/// 
/// About MP3 Files:
/// 
/// There is no main Header within an MPEG audio file. Instead, an MPEG Audio File is built-up from a succession of smaller parts called Frames. 
/// Each Frame is a datablock with its own Header and Audio information. In the case of Layer I or Layer II, Frames are totally independent from each other.
/// However, in the case of Layer III, Frames are not always independent.
/// 
/// Frames may optionally have a CRC Checksum value. If CRC Protection is enabled, the CRC Checksum value is 16-bits wide and inserted immediately after the Frame Header.
/// 
/// The Audio Data within a Frame contains a fixed number of Samples.
/// 
/// MP3 Info Sources:
/// URI: http://www.mp3-tech.org/
/// URI: https://www.codeproject.com/Articles/8295/MPEG-Audio-Frame-Header
/// URI: http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
/// </summary>
class MP3Decoder : public AbstractBaseDecoder
{
	public:
		MP3Decoder();
		MP3Decoder(const wchar_t* filename, bool memoryBufferIsEnabled);
		virtual ~MP3Decoder();
		MP3Decoder(const MP3Decoder& other) = delete; // Delete Copy Constructor
		MP3Decoder& operator=(const MP3Decoder& other) = delete; // Delete Assignment Operator (Overloaded)
		MP3Decoder(MP3Decoder&& other) noexcept = delete; // Delete The Move Constructor
		MP3Decoder& operator=(MP3Decoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

		#pragma region Overridden_Base_Class_Functions_Region

		long long Read();
		unsigned long long GetDecodedAudioDataTotal();
		const wchar_t* GetLastErrorMessage();
		const wchar_t* GetSupportedTypes();

#		pragma endregion Overridden_Base_Class_Functions_Region

		static constexpr wchar_t* DECODER_NAME = L"MP3";
		static constexpr wchar_t FILE_EXTENSION_TYPES[] = L"mp3\0mp2\0m2a\0";

	private:
		StreamWrapper m_stream{};
		wchar_t m_errorMessage[MAX_ERROR_SIZE]{};

		/// <summary>
		/// Purpose: The Sample Rate for the Stream. A Sample Rate is typically constant across all the Frames within an MP3 File.
		/// </summary>
		unsigned long m_streamSampleRate{};

		/// <summary>
		/// Purpose: The current Frame Header being processed. Each MP3 Audio Frame begins with a Frame Header. The Frame Header is the first 4-bytes (32-bits) in an MP3 Audio Frame.
		/// Note: In MPEG Audio File processing, it is common to check two or more Frames in a row to confirm you are really dealing with an MPEG Audio File.
		/// </summary>
		unsigned long m_currentFrameHeader{};
		
		/// <summary>
		/// Purpose: The previous Frame Header that was processed. Each MP3 Audio Frame begins with a Frame Header. The Frame Header is the first 4-bytes (32-bits) in an MP3 Audio Frame.
		/// Note: In MPEG Audio File processing, it is common to check two or more Frames in a row to confirm you are really dealing with an MPEG Audio File.
		/// </summary>
		unsigned long m_previousFrameHeader{};

		/// <summary>
		/// Purpose: Used to store the File size in Bytes.
		/// </summary>
		long long m_fileSizeInBytes{};

		/// <summary>
		/// Purpose: The offset position within the Stream.
		/// </summary>
		long long m_offset{};

		/// <summary>
		/// Purpose: The Frame Sample Position. Incremented/Decremented by a Constant Frame Size (Frame Size = Number of Samples in an Audio Frame).
		/// Represents the Total Number of Frame Samples encountered and can be used to calcluate the duration in time (seconds) of the Frame Position within the MP3 bitstream.
		/// </summary>
		unsigned long m_frameSamplePosition{};

		/// <summary>
		/// Purpose: The version of the MPEG Audio Encoding. Used to indicate whether it is MPEG-1 or MPEG-2 or MPEG-2.5 Audio Encoding.
		/// {MPEG-1 = 3, MPEG-2 = 2, Reserved = 1, MPEG-2.5 = 0}
		/// </summary>
		unsigned long m_encodingVersion{};

		/// <summary>
		/// Purpose: The Layer Description is one of the following: Layer I, Layer II, Layer III, or Reserved.
		/// {Layer I = 3, 'Layer II = 2, Layer III = 1, Reserved = 0}
		/// </summary>
		unsigned long m_layerDescription{};

		/// <summary>
		/// Purpose: The compression rate (A higher bitrate = better quality, but larger file size). Bitrates are always displayed in kilobits (1000) per second.
		/// NOTE: The prefix kilo (abbreviated with the small 'k') doesn't mean 1024 bits, but 1000 bits per second.
		/// </summary>
		unsigned long m_bitrate{};

		/// <summary>
		/// Purpose: The number of Audio Samples per second.
		/// </summary>
		unsigned long m_sampleRate{};

		/// <summary>
		/// Purpose: Used to indicate whether Frame Padding is enabled (1) or disabled (0) for a given Frame.
		/// {Frame is Padded with one extra slot = 1 OR Frame is NOT Padded = 0}
		/// </summary>
		unsigned long m_framePadding{};

		/// <summary>
		/// Purpose: Used to indicate whether the Channel Mode is Single Channel Mono OR any one of the following: Stereo, Joint Stereo (Stereo), or Dual channel (Stereo).
		/// {Mono Mode = 1 OR Stereo Modes = 0}
		/// </summary>
		unsigned long m_singleChannelMono{};

		/// <summary>
		/// Purpose: Used for accessing the columns in the MP3 Bit Rate and MP3 Sample Rate fixed size Arrays.
		/// </summary>
		unsigned long m_columnIndex{};

		/// <summary>
		/// Purpose: Used to the store the ID3v2 Tag Header Length for an MP3 File.
		/// </summary>
		unsigned long m_tagHeaderLengthBytes{};

		/// <summary>
		/// Purpose: Used to store Tag Footer Lengths for an MP3 File. Common Footer Tags include the following: ID3v1 Tag Footer, APE Tag Footer, and embedded Lyric Tags.
		/// </summary>
		unsigned long m_tagFooterLengthBytes{};

		/// <summary>
		/// Purpose: The current Frame Length.
		/// </summary>
		unsigned long m_currentFrameLength{};

		/// <summary>
		/// Purpose: A counter to store the maximum number of times the resychronization effort will be allowed to be attempted, before being abandoned.
		/// </summary>
		unsigned long m_maxResynchronizationCount{};
		
		unsigned long m_CRCByteSize{};

		/// <summary>
		/// Purpose: The 16th Bit in the Frame Header is the Protection Bit. 
		/// The 2 possibles Protection Bit values are: 0 = Protected by CRC (16-bit CRC follows Frame Header) OR 1 = Not protected.
		/// Initially set with the status of the Protection Bit (Enabled (0) / Disabled (1)). If enabled, it will be later used to hold the embedded 16-bit CRC Checksum value for a Frame.
		/// </summary>
		unsigned short m_embeddedFrameCRC{};
		
		/// <summary>
		/// Purpose: A fixed-size array used to contain the CRC Bytes for a specific Audio Frame.
		/// </summary>
		unsigned char m_bufferCRCBytes[40]{};

		/// <summary>
		/// Purpose: Calculates and return the length of the 'ID3v2 Tag Header' in an MP3 file that uses a ID3v2 tag for Metadata.
		/// 
		/// This function reads the first 10 bytes from a Stream, checks if they form an ID3v2 tag, and then calculates the 'ID3v2 Tag Header' length 
		/// based on specific byte values. If successful, it returns the 'ID3v2 Tag Header' length; otherwise, it sets an error message indicating that a bad ID3v3 tag has been encountered.
		/// 
		/// </summary>
		/// <returns>The Header length in bytes.</returns>
		unsigned long GetID3v2TagHeaderLength();
		
		/// <summary>
		/// Purpose: Checks for the following Footer Tag Types: ID3v1 Tag Footer, APE Tag Footer, and (LYRICS3v1 or LYRICS3v2) Tags. Calculates the total Tag Footer length in Bytes.
		/// If an error occurs, the function sets an error message.
		/// URI: https://en.wikipedia.org/wiki/ID3#ID3v1
		/// URI: https://en.wikipedia.org/wiki/APE_tag
		/// </summary>
		/// <returns>The Tag Footer length in bytes.</returns>
		unsigned long GetTagFooterLength();

		/// <summary>
		/// Purpose: This function designed to calculate and return the length of the Lyrics in an MP3 file, when the lyrics are stored in either a LYRICS3v1 Tag OR LYRICS3v2 Tag.
		/// A Lyrics3 Tag resides near the end of the File, after the end of Audio Data, but before the start of an ID3v1 Tag.
		/// If an error occurs, the function sets an error message.
		/// URI: https://en.wikipedia.org/wiki/ID3#Lyrics
		/// URI: https://id3lib.sourceforge.net/id3/lyrics3200.html
		/// </summary>
		/// <returns>The Lyrics length in bytes.</returns>
		unsigned long GetLyricsTagLength();

		/// <summary>
		/// Purpose: MP3 Frame Length is length of a Frame when compressed. It is calculated in slots. 
		/// One slot is 4-bytes long for Layer I, and 1-byte long for Layer II and Layer III. Frame Length may change from Frame to Frame due to Padding or Bitrate Switching.
		/// 
		/// In contrast, Frame Size is the number of samples contained in a Frame and it is Constant and always 384 samples for Layer I and 1152 samples for Layer II and Layer III.
		/// </summary>
		/// <returns>The Frame Length in bytes.</returns>
		unsigned long GetFrameLength();
		
		/// <summary>
		/// Purpose: This function is used for attempting to resynchronize the reading of MP3 Frames within a Stream. 
		/// This function attempts to repeatedly read in fresh bytes from the Stream and check if they eventually form a valid Frame Header.
		/// 
		/// About Resynchronization:
		/// 
		/// Resynchronization in MP3 decoding is necessary because MP3 is a bitstream format. This means that if the decoding process gets desynchronized due 
		/// to an error or inconsistency in the data, it can’t simply start decoding at the next byte. Instead, it needs to find the next valid Frame Header in the bitstream.
		/// 
		/// </summary>
		/// <returns>TRUE if resynchronization was successful. FALSE if resynchronization was failed.</returns>
		bool ResynchronizeBitstream();

		/// <summary>
		/// Purpose: This function is used to verify the integrity of an MP3 file using a Cyclic Redundancy Check (CRC). 
		/// MPEG Audio Frames may have an optional CRC Protection Bit enabled. 
		/// If the CRC Protection Bit is enabled, then an embedded 16-bit CRC Checksum value is placed in the Frame, immediately following MPEG Audio Frame Header.
		/// After the optionally embedded 16-bit CRC Checksum value, the Audio Data begins.
		/// </summary>
		/// <returns>
		/// Returns TRUE if the CRC Protection Bit is enabled AND CRC Check passes successfully OR the CRC Protection Bit is disabled and therefore the file does NOT contain the 16-bit CRC Data. 
		/// Otherwise, returns FALSE indicating that there is a CRC error in the file, which could be due to data corruption.
		/// </returns>
		bool CheckCRCProtection();

		/// <summary>
		/// Purpose: A Cyclic Redundancy Check (CRC) is a type of hash function used to produce a checksum. 
		/// A checksum is a small, fixed-size bit string that checks the integrity of data. 
		/// CRCs are used to detect errors in data transmission or storage.
		/// This function performs a CRC-16 calculation.
		/// </summary>
		/// <param name="bufferPtr">The Buffer data to check with the CRC-16 Algorithm.</param>
		/// <param name="bufferLength">The length of the Buffer Data.</param>
		/// <returns></returns>
		unsigned short CRC16(unsigned char* bufferDataPtr, size_t bufferLength);

		void OpenFile(const wchar_t* filenamePtr, bool memoryBufferIsEnabled);
};

#endif // MP3_DECODER_H