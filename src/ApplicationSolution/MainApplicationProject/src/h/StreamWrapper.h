#ifndef STREAM_WRAPPER_H
#define STREAM_WRAPPER_H

// Standard I/O Header.
#include <cstdio>

// Windows Specific I/O Header.
#include <io.h>

#include <exception>
#include <string>

/// <summary>
/// Purpose: A File I/O Wrapper Class (Primarily uses C-Style File I/O Libraries). Uses available Memory, to prevent Disk Thrashing between Threads.
/// URI: https://cplusplus.com/reference/cstdio/
/// </summary>
class StreamWrapper
{
    public:
        StreamWrapper();
        virtual ~StreamWrapper();
        StreamWrapper(const StreamWrapper& other) = delete; // Delete Copy Constructor
        StreamWrapper& operator=(const StreamWrapper& other) = delete; // Delete Assignment Operator (Overloaded)
        StreamWrapper(StreamWrapper&& other) noexcept = delete; // Delete The Move Constructor
        StreamWrapper& operator=(StreamWrapper&& other) noexcept = delete; // Delete Move Assignment Operator (Overloaded)

        /// <summary>
        /// Purpose: 
        /// </summary>
        /// <param name="filenamePtr"></param>
        /// <param name="useMemoryBufferEnabled">
        /// Set to TRUE, to use available memory via a Memory Buffer, to prevent disk thrashing between Threads. 
        /// Set to FALSE to use the default buffering mechanism.
        /// </param>
        /// <returns>True if the File was opened and read successfully. False if a problem occurred while trying to read the File.</returns>
        bool Open(const wchar_t* filenamePtr, bool useMemoryBufferEnabled = true);

        void Close();

        /// <summary>
        /// Purpose: This function reads data from a buffer or a file stream and stores it in the memory location pointed to by bufferPtr.
        /// The File Position Indicator of the Stream is advanced by the total amount of the bytes read.
        /// </summary>
        /// <param name="bufferPtr">A pointer to a Buffer.</param>
        /// <param name="size">The Buffer size.</param>
        /// <returns>Returns the number of Bytes read.</returns>
        size_t Read(void* bufferPtr, size_t size);
        
        /// <summary>
        /// Purpose: Moves the Stream File Pointer to the specified location.
        /// </summary>
        /// <param name="offset">Initial position.</param>
        /// <param name="origin">(SEEK_SET, SEEK_CUR, SEEK_END) OR Number of bytes from origin.</param>
        /// <returns>If seek was successful then returns 0. If a seek error occurred then returns a non-zero value.</returns>
        int Seek(long long offset, int origin);

        /// <summary>
        /// Purpose: Gets the current value of the File Position Indicator for the File Stream.
        /// </summary>
        /// <returns></returns>
        /// 
        long long Tell();

        /// <summary>
        /// Purpose: Return the total length of the Stream in bytes.
        /// </summary>
        /// <returns>The total length of the Stream in bytes.</returns>
        long long Length();

        bool EndOfFile();
        bool isOpen() const;

    private:
        FILE* m_fileObjectStreamPtr{ nullptr };
        bool m_isEOF{ false };
        bool m_isOpen{ false };
        bool m_isMemoryBufferEnabled{ false };
        unsigned char* m_fileMemoryBufferPtr{ nullptr };
        char* m_fileModeBufferPtr{ nullptr };
        size_t m_fileMemoryBufferSize{};
        size_t m_fileMemoryBufferPosition{};
        
        /// <summary>
        /// Purpose: Opens the File Stream using Direct I/O that is Memory Buffered.
        /// </summary>
        /// <param name="filenamePtr"></param>
        /// <param name="readFixedSizeBlocksEnabled">
        /// TRUE reads using fixed-size Blocks. 
        /// FALSE read using a single Block that is the maximum size allowed.
        /// </param>
        /// <returns></returns>
        bool OpenFileWithMemoryBuffer(const wchar_t* filenamePtr, bool readFixedSizeBlocksEnabled);
        
        /// <summary>
        /// Purpose: Opens the File Stream using Direct I/O that is set by the Mode.
        /// </summary>
        /// <param name="filenamePtr"></param>
        /// <param name="mode">
        /// Supported Modes including the following: 
        /// _IOFBF (Full Buffering) Mode which uses more System Memory.
        /// _IONBF (No Buffering) which uses less System Memory.
        /// </param>
        /// <returns></returns>
        bool OpenFileMode(const wchar_t* filenamePtr, int mode);
};

#endif // STREAM_WRAPPER_H
