#include "pch.h"
#include "CppUnitTest.h"
#include "HelloWorld.h"
#include "HelloWorld.cpp"
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MainApplicationUnitTest
{
	TEST_CLASS(HelloWorldUnitTest)
	{
	public:
		
		TEST_METHOD(testGetMessage)
		{
			// Arrange
			HelloWorld helloWorld;
			std::string expected = "Hello World!";

			// Act
			std::string actual = helloWorld.getMessage();

			// Assert
			Assert::AreEqual(expected, actual);
		}

		TEST_METHOD(testGetNumber)
		{
			// Arrange
			HelloWorld helloWorld;
			int expected = 0;

			// Act
			int actual = helloWorld.getNumber();

			// Assert
			Assert::AreEqual(expected, actual);
		}

	};
}
