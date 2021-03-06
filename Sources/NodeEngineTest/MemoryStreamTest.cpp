#include "SimpleTest.hpp"
#include "NE_MemoryStream.hpp"

#include <memory>

using namespace NE;

namespace MemoryStreamTest
{

enum class TestEnum
{
	A,
	B
};

TEST (TypeTest)
{
	MemoryOutputStream outputStream;
	ASSERT (outputStream.Write (true) == Stream::Status::NoError);
	ASSERT (outputStream.Write ('a') == Stream::Status::NoError);
	ASSERT (outputStream.Write ((size_t) 1) == Stream::Status::NoError);
	ASSERT (outputStream.Write ((int) 2) == Stream::Status::NoError);
	ASSERT (outputStream.Write ((float) 3.0f) == Stream::Status::NoError);
	ASSERT (outputStream.Write ((double) 4.0) == Stream::Status::NoError);
	ASSERT (outputStream.Write ((short) 5) == Stream::Status::NoError);
	ASSERT (outputStream.Write (std::string ("apple")) == Stream::Status::NoError);
	ASSERT (outputStream.Write (std::wstring (L"orange")) == Stream::Status::NoError);
	ASSERT (outputStream.Write (std::wstring (L"unicode \u03c0")) == Stream::Status::NoError);
	ASSERT (WriteEnum (outputStream, TestEnum::A) == Stream::Status::NoError);

	bool boolVal;
	char charVal;
	size_t sizeVal;
	int intVal;
	float floatVal;
	double doubleVal;
	short shortVal;
	std::string stringVal;
	std::wstring wStringVal;
	std::wstring wStringValUnicode;
	TestEnum enumVal;

	MemoryInputStream inputStream (outputStream.GetBuffer ());
	ASSERT (inputStream.Read (boolVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (charVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (sizeVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (intVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (floatVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (doubleVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (shortVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (stringVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (wStringVal) == Stream::Status::NoError);
	ASSERT (inputStream.Read (wStringValUnicode) == Stream::Status::NoError);
	ASSERT (ReadEnum (inputStream, enumVal) == Stream::Status::NoError);

	ASSERT (boolVal == true);
	ASSERT (charVal == 'a');
	ASSERT (sizeVal == 1);
	ASSERT (intVal == 2);
	ASSERT (floatVal == 3.0f);
	ASSERT (doubleVal == 4.0);
	ASSERT (shortVal == 5);
	ASSERT (stringVal == "apple");
	ASSERT (wStringVal == L"orange");
	ASSERT (wStringValUnicode == L"unicode \u03c0");
	ASSERT (enumVal == TestEnum::A);
}

}
