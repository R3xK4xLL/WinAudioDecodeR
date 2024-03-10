#ifndef PURE_ABSTRACT_BASE_DECODER_H
#define PURE_ABSTRACT_BASE_DECODER_H

#include <string>

// The maximum size for C-style strings used for Error Messages.
constexpr auto MAX_ERROR_SIZE = 256;

/// <summary>
/// Purpose: A Pure Abstract Base Class (PABC) for Decoders. A PABC only has pure virtual functions and no concrete member functions. 
/// In contrast, an Abstract Base Class (ABC) contains both pure virtual functions and concrete (non-virtual) member functions.
/// </summary>
class PureAbstractBaseDecoder
{
	public:
		PureAbstractBaseDecoder() = default;
		virtual ~PureAbstractBaseDecoder() = default;
		PureAbstractBaseDecoder(const PureAbstractBaseDecoder& other) = delete; // Delete Copy Constructor
		PureAbstractBaseDecoder& operator=(const PureAbstractBaseDecoder& other) = delete; // Delete Assignment Operator (Overloaded)
		PureAbstractBaseDecoder(PureAbstractBaseDecoder&& other) noexcept = delete; // Delete The Move Constructor
		PureAbstractBaseDecoder& operator=(PureAbstractBaseDecoder&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

		/// <summary>
		/// Purpose: The name of the Decoder.
		/// </summary>
		/// <returns>The name.</returns>
		virtual std::wstring GetName() = 0;

		/// <summary>
		/// Use the Decoder to read Audio Units and return the number of Decoded Audio Units read during the operation.
		/// </summary>
		/// <returns>The number of Decoded Audio Units read OR (0 on end of file, -1 on error) during the operation.</returns>
		virtual long long Read() = 0;

		/// <summary>
		/// Purpose: A number representing the total number of Decoded Audio Units read using the Decoder.
		/// </summary>
		/// <returns>The total number of Decoded Audio Units read using the Decoder.</returns>
		virtual unsigned long long GetDecodedAudioDataTotal() = 0;

		/// <summary>
		/// Purpose: Gets the last error message reported by the Decoder.
		/// </summary>
		/// <returns>The last error message reported by the Decoder.</returns>
		virtual const wchar_t* GetLastErrorMessage() = 0;
};

#endif // PURE_ABSTRACT_BASE_DECODER_H