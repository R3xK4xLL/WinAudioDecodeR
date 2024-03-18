#include "pch.h"
#include "CppUnitTest.h"
#include "AbstractBaseDecoder.h"
#include "AbstractBaseDecoder.cpp"
#include "FlacDecoder.h"
#include "FlacDecoder.cpp"
#include "StreamWrapper.h"
#include "StreamWrapper.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr auto TEST_FILE_PATH = L"C:\\git\\repos\\WinAudioDecodeR\\src\\ApplicationSolution\\MainApplicationUnitTestProject\\unit-test-data\\flac\\flac-test-files\\subset\\01 - blocksize 4096.flac";

namespace MainApplicationUnitTest
{
    /// <summary>
    /// Purpose: A Test Class for the FLAC Decoder.
    /// </summary>
    TEST_CLASS(FlacDecoderUnitTest)
    {
    public:

        /// <summary>
        /// Purpose: Test the no-args Constructor and the resulting Decoder Open status.
        /// </summary>
        TEST_METHOD(testDecoderNoArgConstructor)
        {
            // Arrange
            FlacDecoder myDecoder;
            bool expected = false;

            // Act
            bool actual = myDecoder.DecoderIsOpen();

            // Assert
            Assert::AreEqual(expected, actual);
        }

        /// <summary>
        /// Purpose: Test the with-args Constructor and the resulting Decoder Open status.
        /// </summary>
        TEST_METHOD(testDecoderWithArgsConstructor)
        {
            // Arrange
            std::wstring filename = TEST_FILE_PATH;
            FlacDecoder testDecoder(filename.c_str(), true);
            bool expected = true;

            // Act
            bool actual = testDecoder.DecoderIsOpen();

            // Assert
            Assert::AreEqual(expected, actual);
        }

        /// <summary>
        /// Purpose: Tests reading a single Audio Unit of Data within a FLAC File using the Decoder.
        /// </summary>
        TEST_METHOD(testDecoderRead)
        {
            // Arrange
            std::wstring filename = TEST_FILE_PATH;
            FlacDecoder testDecoder(filename.c_str(), true);
            long long expected = 4096;

            // Act
            long long actual = testDecoder.Read();

            // Assert
            Assert::AreEqual(expected, actual);
        }

        /// <summary>
        /// Purpose: Tests reading all of the Audio Data within a FLAC File using the Decoder.
        /// </summary>
        TEST_METHOD(testReadAllAudioData)
        {
            // Arrange
            std::wstring filename = TEST_FILE_PATH;
            FlacDecoder testDecoder(filename.c_str(), true);
            long long expected = testDecoder.GetDecodedAudioDataTotal();

            // Act
            long long actual = 0LL;
            long long loopVar = testDecoder.Read();
            actual += loopVar;
            while (loopVar > 0LL)
            {
                loopVar = testDecoder.Read();
                actual += loopVar;
            }

            // Assert
            Assert::AreEqual(expected, actual);
        }
    };
}
