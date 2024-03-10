# Error Message Guide

GENERAL
-------

- Error Messages:
	- UNABLE_TO_OPEN_DECODER
		- An unspecified problem occurred, causing initialization of the Decoder to fail.

FLAC
----

- Error Messages:
	- TRUNCATED @ \<time>
		- The Decoder indicated that an error occurred before reaching the end of the FLAC File Stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga3adb6891c5871a87cd5bbae6c770ba2da28ce845052d9d1a780f4107e97f4c853) for details.
	
	- SEEK_ERROR
		- The Decoder indicated that an error occurred before reaching the end of the FLAC File Stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga3adb6891c5871a87cd5bbae6c770ba2daf2c6efcabdfe889081c2260e6681db49) for details.
	
	- DECODER_ABORTED
		- The Decoder indicated that an error occurred before reaching the end of the FLAC File Stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga3adb6891c5871a87cd5bbae6c770ba2dadb52ab4785bd2eb84a95e8aa82311cd5) for details.
	
	- MEMORY_ALLOCATION_ERROR
		- The Decoder indicated that an error occurred before reaching the end of the FLAC File Stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga3adb6891c5871a87cd5bbae6c770ba2da0d08c527252420813e6a6d6d3e19324a) for details.
	
	- OGG_LAYER_ERROR
		- The Decoder indicated that an error occurred before reaching the end of the FLAC File Stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga3adb6891c5871a87cd5bbae6c770ba2da3bc0343f47153c5779baf7f37f6e95cf) for details.
	
	- DECODER_ERROR
		- The Decoder indicated that an unspecified error occurred, and this happened before reaching the end of the FLAC File Stream.
	
	- DECODER_LOST_SYNC
		- The Decoder lost sync due to an unspecified error, and this happened before reaching the end of the FLAC File Stream.
	
	- MD5_MISMATCH
		- The Decoder successfully reached the end of the FLAC File Stream. However, the MD5 signature stored within the FLAC file does NOT match the one computed by the Decoder.
	
	- EXTRA_SAMPLES
		- The Decoder successfully reached the end of the FLAC File Stream. However, more Decoded data than expected was encountered. 
	
	- MISSING_SAMPLES
		- The Decoder successfully reached the end of the FLAC File Stream. However, less Decoded data than expected was encountered. 
	
	- LOST_SYNC @ \<time>
		- Several scenarios result in this error message:
			- An error caused the Decoder to lose synchronization before the end of the Stream.
			- OR
			- An unknown error caused the Decoder to lose synchronization.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga130e70bd9a73d3c2416247a3e5132ecfa3ceec2a553dc142ad487ae88eb6f7222) for details.
	
	- <LOST_SYNC @ \<time>> <ID3v1_TAG_FOUND>
		- The existence of a non-standard ID3v1 Tag was found embedded at the end of the file, resulting in a loss of synchronization.
		- ID3v1 Tags are not part of the FLAC standard for supported Tags. Therefore, the file is technically non-compliant with the FLAC standard.
	
	- BAD_HEADER @ \<time>
		- The Decoder encountered a corrupted frame header.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga130e70bd9a73d3c2416247a3e5132ecfae393a9b91a6b2f23398675b5b57e1e86) for details.
	
	- FRAME_CRC_MISMATCH @ \<time>
		- The Decoder encounterd a frame thats data did not match the CRC in the footer.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga130e70bd9a73d3c2416247a3e5132ecfa208fe77a04e6ff684e50f0eae1214e26) for details.
	
	- UNPARSEABLE_STREAM
		- The decoder encountered reserved fields in use in the stream.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga130e70bd9a73d3c2416247a3e5132ecfa8b6864ad65edd8fea039838b6d3e5575) for details.
	
	- BAD_METADATA
		- The decoder encountered a corrupted metadata block.
		- [See FLAC API Documentation](https://xiph.org/flac/api/group__flac__stream__decoder.html#gga130e70bd9a73d3c2416247a3e5132ecfa67ee497c6fe564b50d7a7964ef5cd30a) for details.

MP3
---

- Error Messages:

	- TRUNCATED
		- The number of Decoded bytes were more than the expected Stream length in bytes.
	
	- LOST_SYNC @ \<time>
		- Synchronization was lost, and an attempt to Resynchronize the bitstream was successful.
			- The Frame Sample Position within the Stream, was NOT at the start, indicating the error occurred either within or at the end of the Stream.
	
	- LOST_SYNC @ END_OF_FILE
		- Several scenarios result in this error message:
			- Failure to read the last 4-bytes of the File.
			- OR
			- Synchronization was lost, and an attempt to Resynchronize the bitstream failed.
				- The Frame Sample Position within the Stream, was NOT at the start, indicating the error occurred either within or at the end of the Stream.
	
	- UNRECOGNIZED_FORMAT
		- Synchronization was lost, and an attempt to Resynchronize the bitstream failed.
			- The Frame Sample Position within the Stream, was at the start, indicating the error occurred at the beginning of the Stream.
	
	- BAD_ID3v2_TAG
		- Synchronization was lost, and an attempt to Resynchronize the bitstream was successful.
			- The Tag Header bytes were found at the start of the Stream.
	
	- BAD_STARTING_SYNC
		- Synchronization was lost, and an attempt to Resynchronize the bitstream was successful.
			- Tag Header bytes do NOT exist at the start of the Stream.
	
	- BAD_ID3v2_TAG
		- The bytes of the 'ID3v2 Tag Header' did NOT match the expected length, indicating that a problem was encountered within the 'ID3v2 Tag Header' bytes.
	
	- BAD_APE_TAG
		- The 'APE Tag Footer' bytes are NOT within the expected size range. Therefore, a bad APE Tag was encountered.
	
	- BAD_LYRICS3v1_TAG
		- A "LYRICSEND" string was found, but a corresponding "LYRICSBEGIN" string was NOT found.
			- NOTE: A valid LYRICS3v1 Tag block starts with the string "LYRICSBEGIN" and ends with the string "LYRICSEND".
	
	- BAD_LYRICS3v2_TAG
		- A "LYRICS200" string was found, but a corresponding "LYRICSBEGIN" string was NOT found.
			- NOTE: A valid LYRICS3v2 Tag block starts with the string "LYRICSBEGIN" and ends with the string "LYRICS200".
	
	- CRC_ERROR @ \<time>
		- The calculated CRC-16 Checksum value from the raw data does NOT match the 16-bit CRC Checksum value read from the file. The data is possibly corrupted.
		- NOTE: MPEG Audio Frames may have an optional CRC Protection Bit enabled.
			- If the CRC Protection Bit is enabled, then an embedded 16-bit CRC Checksum value is placed in the Frame, immediately following MPEG Audio Frame Header.
			- After the optionally embedded 16-bit CRC Checksum value, the Audio Data begins.

WavPack
-------

- Error Messages:
	- MD5_MISMATCH
		- The computed MD5 Hash Digest using all of the unpacked WavPack data, does NOT match the MD5 checksum value stored within the WavPack Metadata.
	
	- \<number> BAD_BLOCKS
		- This error can be caused by a number of different internal WavPack errors. The most likely causes are either CRC errors or Missing Blocks.
	
	- \<number> SAMPLE_COUNT_UNKNOWN_ERROR
		- A decoded sample count mismatch occurred.
			- The total number of expected samples was determined to be unknown.
	
	- \<number> MISSING_SAMPLES
		- A sample count mismatch was found.
			- The total unpacked sample count was less than total number of expected samples.
	
	- \<number> EXTRA_SAMPLES
		- A sample count mismatch was found.
			- The total unpacked sample count exceeded the total number of expected samples.

Ogg-Vorbis
----------

- Error Messages:
	
	- TRUNCATED
		- Indicates that during decoding, the End-of-File (EOF) was reached before before the last packet was buffered, within the logical bitstream.
	
	- OGG-VORBIS_HOLE @ \<time>
		- Indicates that there was an interruption in the Data.
		- Possibilities include one of the following: garbage between pages, loss-of-sync followed by recapture, or a corrupt page.

	- OGG-VORBIS_EBADLINK @ \<time>
		- Indicates that an invalid Stream section was supplied to the Decoder, or the requested Link is corrupt.
			- NOTE: A Vorbis stream may consist of multiple sections (called Links) that encode differing numbers of channels or sample rates.

	- UNREADABLE_OR_CORRUPT_HEADER
		- Indicates the initial File Headers couldn't be read or are corrupt.
		- OR 
		- Opening the Vorbis File failed.
