#ifndef DECODER_MANAGER_H
#define DECODER_MANAGER_H

#include <list>
#include <map>
#include <memory>
#include "Utils.h"
#include "PureAbstractBaseDecoder.h"
#include "MP3Decoder.h"
#include "OggVorbisDecoder.h"
#include "FlacDecoder.h"
#include "WavPackDecoder.h"

/// <summary>
/// Purpose: A Class used to manage Decoders.
/// </summary>
class DecoderManager
{
	public:
		DecoderManager();
		virtual ~DecoderManager();
		DecoderManager(const DecoderManager& other) = delete; // Delete Copy Constructor
		DecoderManager& operator=(const DecoderManager& other) = delete; // Delete Assignment Operator (Overloaded)
		DecoderManager(DecoderManager&& other) noexcept = delete; // Delete The Move Constructor
		DecoderManager& operator=(DecoderManager&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

		/// <summary>
		/// Purpose: Opens a Decoder for the specified filename and returns Memory-managed Smart Pointer to the Decoder.
		/// </summary>
		/// <param name="filenamePtr">The filename.</param>
		/// <returns>A Memory-managed Smart Pointer to the Decoder. The returned pointer can be NULL.</returns>
		std::unique_ptr<PureAbstractBaseDecoder> OpenDecoderSmartPointer(const wchar_t* filenamePtr);

		/// <summary>
		/// Purpose: Determines whether the filename is considered to be type supported by the available Decoders.
		/// </summary>
		/// <param name="filenamePtr">The filename to be checked for Decoder support.</param>
		/// <returns>TRUE if the file is supported by an available Decoder. Otherwise, FALSE if the file is not supported by any of the available Decoders.</returns>
		bool IsSupportedType(const wchar_t* filenamePtr);
		
		/// <summary>
		/// Purpose: Set the Decoder Manager configuration to enable/disable the use of Memory Buffers with managed Decoders.
		/// When Memory Buffers are enabled the executing Process will use more Memory during runtime.
		/// </summary>
		/// <param name="value"></param>
		void SetDecoderMemoryBufferEnabled(bool value);

		
	private:
		static constexpr auto UNSUPPORTED_TYPE = L"UNSUPPORTED";
		std::map<std::wstring, std::wstring> m_fileExtensionToDecoderNameMap{};
		bool decoderMemoryBufferEnabled{ false };

		/// <summary>
		/// Purpose: An internal helper function used to populate an internal Map used by the Decoder Manager, that 
		/// associated a File Extension with a Decoder. The Map is useful for quick file extension to Decoder name lookups.
		/// </summary>
		/// <param name="supportedTypesPtr">An Array of null-terminated File Extensions.</param>
		/// <param name="decoderName">The Decoder name to be associated with the File Extensions.</param>
		void PopulateFileExtensionToDecoderNameMap(const wchar_t* supportedTypesPtr, const wchar_t* decoderName);

		/// <summary>
		/// Purpose: A read-only private helper function used to find a Decoder Name for the given filename. The Object state is not modified.
		/// </summary>
		/// <param name="filenamePtr"></param>
		/// <returns></returns>
		std::wstring FindDecoderType(const wchar_t* filenamePtr) const;
		
		/// <summary>
		/// Purpose: Opens a Decoder on the specified filename.
		/// </summary>
		/// <param name="filenamePtr"></param>
		/// <returns>A Raw Pointer to a Pure Abstract Base Class (PABC) Interface for the Decoder.</returns>
		PureAbstractBaseDecoder* OpenDecoder(const wchar_t* filenamePtr) const;
};

#endif // DECODER_MANAGER_H