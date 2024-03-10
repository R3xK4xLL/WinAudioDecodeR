#include "StreamWrapper.h"
#include <windows.h>

// ****************************************************************************
//							File I/O Wrapper Class
// ****************************************************************************

/// <summary>
/// Purpose: The maximum Block Size for NTFS and ReFS (64k).
/// </summary>
constexpr auto BLOCK_SIZE = 65536;

/// <summary>
/// Purpose: No-arg Constructor.
/// </summary>
StreamWrapper::StreamWrapper()
{

}

/// <summary>
/// Purpose: Destructor
/// </summary>
StreamWrapper::~StreamWrapper()
{
	this->Close();
}

bool StreamWrapper::Open(const wchar_t* filenamePtr, bool useMemoryBufferEnabled)
{
	this->m_isMemoryBufferEnabled = useMemoryBufferEnabled;
	bool fileWasOpenedFlag = false;

	if (useMemoryBufferEnabled)
	{
		// Open the File using the Memory Buffer.
		fileWasOpenedFlag = this->OpenFileWithMemoryBuffer(filenamePtr, true);
	}
	else
	{
		// Open the File using the specified Mode.
		fileWasOpenedFlag = this->OpenFileMode(filenamePtr, _IONBF);
	}

	return fileWasOpenedFlag;
}

/// <summary>
/// Purpose: Closes the Stream, cleans up used Memory Resources, and resets all Stream members to defaults.
/// </summary>
void StreamWrapper::Close()
{
	if (this->m_fileObjectStreamPtr != nullptr)
	{
		// URI: https://cplusplus.com/reference/cstdio/fclose/
		fclose(this->m_fileObjectStreamPtr); // Close the File.
		this->m_fileObjectStreamPtr = nullptr;
	}

	if (this->m_fileMemoryBufferPtr != nullptr)
	{
		delete[] this->m_fileMemoryBufferPtr;
		this->m_fileMemoryBufferPtr = nullptr;
	}

	if (this->m_fileModeBufferPtr != nullptr)
	{
		delete[] this->m_fileModeBufferPtr;
		this->m_fileModeBufferPtr = nullptr;
	}

	// Reset to default values.
	this->m_fileMemoryBufferSize = 0;
	this->m_fileMemoryBufferPosition = 0;
	this->m_isEOF = false;
	this->m_isOpen = false; // Mark the File Stream as Closed.
	this->m_isMemoryBufferEnabled = false;
}

size_t StreamWrapper::Read(void* bufferPtr, size_t size)
{
	if (this->m_fileMemoryBufferPtr != nullptr && this->m_isMemoryBufferEnabled)
	{
		// Use Manual Read Operation to read the Data from the Buffer.

		// Given the current File Buffer Position, check if the requested Read() size exceeds the size of File Buffer.
		if ( (this->m_fileMemoryBufferPosition + size) > this->m_fileMemoryBufferSize)
		{
			// File Buffer Size exceeded, adjust the Read() requested size so that it will fit within the File Buffer.
			size = this->m_fileMemoryBufferSize - this->m_fileMemoryBufferPosition;
			
			// This satisfies the condition for reaching EOF.
			this->m_isEOF = true;
		}
		
		if (size > 0)
		{
			// Copy the data to the memory buffer location from the file buffer.

			const void* srcFileBuffferPtr = this->m_fileMemoryBufferPtr + this->m_fileMemoryBufferPosition;
			memcpy(bufferPtr, srcFileBuffferPtr, size); // URI: https://cplusplus.com/reference/cstring/memcpy/

			// Update the file buffer position by adding the size of data read.
			this->m_fileMemoryBufferPosition += size;
		}

		// Return the number of Bytes read.
		return size;
	}
	else
	{
		// Use C-Style File I/O Library to Read.
		// Read data from the file stream using the fread() function and return the number of Bytes read.
		return fread(bufferPtr, sizeof(char), size, this->m_fileObjectStreamPtr);
	}
}

int StreamWrapper::Seek(long long offset, int origin)
{
	if (this->m_fileMemoryBufferPtr != nullptr && this->m_isMemoryBufferEnabled)
	{
		// Use Manual Seek Operation.
		this->m_isEOF = false;
		switch (origin)
		{
			case SEEK_SET:
				// Seeking from the beginning of the file.
				if (offset < 0 || unsigned(offset) > this->m_fileMemoryBufferSize)
				{
					// Seek Error
					return -1;
				}
				else
				{
					// Seek Successful
					this->m_fileMemoryBufferPosition = static_cast<size_t>(offset);
					return 0;
				}
			case SEEK_CUR:
				// Seeking from the current file position.
				if ( (this->m_fileMemoryBufferPosition + offset) < 0 || (this->m_fileMemoryBufferPosition + offset) > this->m_fileMemoryBufferSize )
				{
					// Seek Error
					return -1;
				}
				else
				{
					// Seek Successful
					this->m_fileMemoryBufferPosition = static_cast<size_t>(this->m_fileMemoryBufferPosition + offset);
					return 0;
				}
			case SEEK_END:
				// 
				// Seeking from the end of the file.
				if (offset > 0 || this->m_fileMemoryBufferSize + offset < 0)
				{
					// Seek Error
					return -1;
				}
				else
				{
					// Seek Successful
					this->m_fileMemoryBufferPosition = static_cast<size_t>(this->m_fileMemoryBufferSize + offset);
					return 0;
				}
			default:
				// Seek Error
				return -1;
		}
	}
	else
	{
		// Use C-Style File I/O Library to Seek.
		// URI: https://en.cppreference.com/w/cpp/io/c/fseek
		// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fseek-fseeki64
		return _fseeki64(this->m_fileObjectStreamPtr, offset, origin);
	}
}

long long StreamWrapper::Tell()
{
	if (this->m_fileMemoryBufferPtr != nullptr && this->m_isMemoryBufferEnabled)
	{
		return this->m_fileMemoryBufferPosition;
	}
	else
	{
		// Use C-Style File I/O Library to tell.
		// URI: https://en.cppreference.com/w/cpp/io/c/ftell
		// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/ftell-ftelli64
		return _ftelli64(this->m_fileObjectStreamPtr);
	}
}

long long StreamWrapper::Length()
{
	if (this->m_fileMemoryBufferPtr != nullptr && this->m_isMemoryBufferEnabled)
	{
		return this->m_fileMemoryBufferSize;
	}
	else
	{
		// Use C-Style File I/O Library to determine File Length.
		return _filelengthi64(_fileno(this->m_fileObjectStreamPtr));
	}
}

bool StreamWrapper::EndOfFile()
{
	if (this->isOpen()) {
		if (feof(this->m_fileObjectStreamPtr) != 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return this->m_isEOF;
	}
}

bool StreamWrapper::isOpen() const
{
	return this->m_isOpen;
}

bool StreamWrapper::OpenFileWithMemoryBuffer(const wchar_t* filenamePtr, bool readFixedSizeBlocksEnabled)
{
	// Open the File Stream. (Using security enhanced function)
	// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-s-wfopen-s

	bool fileWasOpenedFlag = false;
	errno_t errorCode = _wfopen_s(&this->m_fileObjectStreamPtr, filenamePtr, L"rb"); // Open the File for reading in binary mode.

	if (errorCode == 0 && this->m_fileObjectStreamPtr != nullptr)
	{
		this->m_isOpen = true;  // Mark the File Stream as Open.

		// Use Direct I/O which bypasses the buffering mechanism of the Standard I/O library and reads or writes data directly to or from a File.
		// NOTE: Direct I/O is useful when you need to read or write large amounts of data and want to avoid the overhead of default Standard I/O library buffering.

		// Get the file descriptor associated with the File Stream.
		// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno
		int fileDescriptor = _fileno(this->m_fileObjectStreamPtr);

		// Get the File Length in Bytes.
		// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/filelength-filelengthi64
		size_t fileLengthBytes = _filelength(fileDescriptor); // Initialize the loop control variable.

		if (fileLengthBytes > 0)
		{
			this->m_fileMemoryBufferSize = fileLengthBytes;
			try {

				// Free any existing memory pointed to by the Memory Buffer.
				if (this->m_fileMemoryBufferPtr != nullptr) {
					delete[] this->m_fileMemoryBufferPtr;
					this->m_fileMemoryBufferPtr = nullptr;
					this->m_fileMemoryBufferPosition = 0;
					this->m_isEOF = false;
				}

				// Create a new memory buffer of bytes on the Heap, equal in size to the number of bytes in the File.
				this->m_fileMemoryBufferPtr = new unsigned char[this->m_fileMemoryBufferSize]{};

				// Initialize the buffer pointer used for reading the File, at the start of the Memory Buffer.
				unsigned char* readBufferPtr = this->m_fileMemoryBufferPtr; // Pointer to a block of Memory with a size of at least (elementSize * elementCount) bytes.

				bool readEndOfFileFlagEnabled = false;
				bool readErrorFlagEnabled = false;
				size_t fileBytesRead = 0;
				size_t bufferSize = this->m_fileMemoryBufferSize;
				size_t elementSize = sizeof(char); // Representing 1-byte (Size in bytes, of each element to be read.)
				size_t elementCount = 0; // Number of elements, each one with a size of elementSize bytes.
				
				if (readFixedSizeBlocksEnabled)
				{
					// Read a fixed-size Block at a time using multiple calls to the fread_s() function.

					while (fileLengthBytes > 0)
					{
						// Set the maximum number of bytes to be read for this iteration.
						elementCount = (fileLengthBytes < BLOCK_SIZE) ? fileLengthBytes : BLOCK_SIZE;

						// Returns the total number of elements successfully read from the file.
						// If this number differs from the count parameter, either a reading error occurred or the EOF was reached while reading.

						// Reads the File Bytes into the memory Read Buffer. Reads 1-Block or less, at a time. (Using security enhanced function)
						// URI: https://cplusplus.com/reference/cstdio/fread/
						// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fread-s
						fileBytesRead = fread_s(readBufferPtr, bufferSize, elementSize, elementCount, this->m_fileObjectStreamPtr);

						if (fileBytesRead == elementCount)
						{
							readBufferPtr += elementCount; // Increment the read buffer pointer forward by the size of the last read length of file bytes.
							fileLengthBytes -= elementCount; // Decrement the loop control variable.
						}
						else
						{
							// Either EOF was reached while reading the file, or an error occurred.
							if (feof(this->m_fileObjectStreamPtr) != 0)
							{
								// URI: https://cplusplus.com/reference/cstdio/feof/
								readEndOfFileFlagEnabled = true;
							}

							if (ferror(this->m_fileObjectStreamPtr) != 0)
							{
								// URI: https://cplusplus.com/reference/cstdio/ferror/
								readErrorFlagEnabled = true;
							}

							break;
						}

					}
				}
				else
				{
					// Read using a single maximum-sized Block, using a single call to the fread_s() function.

					// Set the maximum Element Count (Block) size to the total File length in bytes.
					elementCount = fileLengthBytes;

					// Read the maximum Element Count (Reduced number of System Calls) (Using security enhanced function)
					// URI: https://cplusplus.com/reference/cstdio/fread/
					// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fread-s
					fileBytesRead = fread_s(readBufferPtr, bufferSize, elementSize, elementCount, this->m_fileObjectStreamPtr);

					if (fileBytesRead != elementCount)
					{
						// Either EOF was reached while reading the file, or an error occurred.
						if (feof(this->m_fileObjectStreamPtr) != 0)
						{
							// URI: https://cplusplus.com/reference/cstdio/feof/
							readEndOfFileFlagEnabled = true;
						}

						if (ferror(this->m_fileObjectStreamPtr) != 0)
						{
							// URI: https://cplusplus.com/reference/cstdio/ferror/
							readErrorFlagEnabled = true;
						}
					}
				}
				
				if (readEndOfFileFlagEnabled || readErrorFlagEnabled) {
					this->Close();
				}
			}
			catch (std::exception& e) {
				this->Close();
				std::string message("An Exception occurred while reading the File into the Memory Buffer. Exception Message: ");
				message += e.what();
				throw std::exception(message.c_str());
			}
		}

		// File was opened and read successfully into the Memory Buffer.
		fileWasOpenedFlag = true;
	}
	else
	{
		// The File Stream was not opened.
		fileWasOpenedFlag = false;
	}

	return fileWasOpenedFlag;
}

bool StreamWrapper::OpenFileMode(const wchar_t* filenamePtr, int mode)
{
	// Open the File Stream. (Using security enhanced function)
	// URI: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-s-wfopen-s

	bool fileWasOpenedFlag = false;
	errno_t errorCode = _wfopen_s(&this->m_fileObjectStreamPtr, filenamePtr, L"rb"); // Open the File for reading in binary mode.

	if (errorCode == 0 && this->m_fileObjectStreamPtr != nullptr)
	{
		if (mode == _IOFBF)
		{
			// Set the File Stream to be buffered. (Full Buffering)
			int fileDescriptor = _fileno(this->m_fileObjectStreamPtr);
			size_t fileBufferSize = _filelength(fileDescriptor);

			// Create a new memory buffer of bytes on the Heap, equal in size to the number of bytes in the File.
			this->m_fileModeBufferPtr = new char[fileBufferSize]{};

			// Set the File Stream to be buffered.
			// Using _IOFBF mode.
			// URI: https://cplusplus.com/reference/cstdio/setvbuf/
			setvbuf(this->m_fileObjectStreamPtr, this->m_fileModeBufferPtr, _IOFBF, fileBufferSize);

			fileWasOpenedFlag = true; // The File Stream was opened.
			this->m_isOpen = true; // Mark the File Stream as Open.
		}
		else if (mode == _IONBF)
		{
			// Set the File Stream to be unbuffered. (No Buffering)
			// Using _IONBF mode.
			// In this mode, each I/O operation is written as soon as possible. In this case, the buffer and size parameters are ignored.
			// URI: https://cplusplus.com/reference/cstdio/setvbuf/
			setvbuf(this->m_fileObjectStreamPtr, nullptr, _IONBF, 0);

			fileWasOpenedFlag = true; // The File Stream was opened.
			this->m_isOpen = true; // Mark the File Stream as Open.
		}
		else
		{
			// Unsupported Mode encountered.
			fileWasOpenedFlag = false; // The File Stream was not opened.
			this->Close(); // Close the File Stream.
		}
	}
	else
	{
		// The File Stream was not opened.
		fileWasOpenedFlag = false;
	}

	return fileWasOpenedFlag;
}
