#include <CoreUtilities/Archive/MemoryArchive.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	struct ComplexStruct
	{
		String str;
		size_t num;

		VT_INLINE friend Archive& operator<<(Archive& archive, ComplexStruct& value)
		{
			archive << value.str;
			archive << value.num;

			return archive;
		}
	};

	TEST(MemoryArchive, OperatorString)
	{
		MemoryWriter writer;

		String inputString = "This is a test";
		writer << inputString;
		writer.Close();
	
		MemoryReader reader(writer.GetData(), writer.GetSize());

		String outputString;
		reader << outputString;

		ASSERT_STREQ(outputString.c_str(), "This is a test");
	}

	TEST(MemoryArchive, OperatorFilepath)
	{
		MemoryWriter writer;

		Filesystem::Path inputPath = "File/Path/To/File.txt";
		writer << inputPath;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Filesystem::Path outputPath;
		reader << outputPath;

		ASSERT_STREQ(outputPath.CStr(), L"File/Path/To/File.txt");
	}

	TEST(MemoryArchive, OperatorGUID)
	{
		constexpr VoltGUID guid = "{E81B82AC-8637-4F4D-81C2-65AB74C52B4E}"_guid;

		MemoryWriter writer;

		VoltGUID inputGuid = guid;
		writer << inputGuid;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());
		
		VoltGUID outputGuid;
		reader << outputGuid;

		ASSERT_EQ(outputGuid, guid);
	}

	TEST(MemoryArchive, OperatorVectorSimple)
	{
		Vector<size_t> inputVector;
		inputVector.resize_uninitialized(50);

		for (size_t i = 0; i < inputVector.size(); ++i)
		{
			inputVector[i] = i;
		}

		MemoryWriter writer;
		writer << inputVector;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Vector<size_t> outputVector;
		reader << outputVector;

		ASSERT_EQ(outputVector.size(), inputVector.size());

		for (size_t i = 0; i < outputVector.size(); ++i)
		{
			ASSERT_EQ(outputVector[i], i);
		}
	}

	TEST(MemoryArchive, OperatorVectorComplex)
	{
		Vector<ComplexStruct> inputVector;
		inputVector.resize(50);

		for (size_t i = 0; i < inputVector.size(); ++i)
		{
			inputVector[i].str = "This is a test";
			inputVector[i].num = i;
		}

		MemoryWriter writer;
		writer << inputVector;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Vector<ComplexStruct> outputVector;
		reader << outputVector;

		ASSERT_EQ(outputVector.size(), inputVector.size());

		for (size_t i = 0; i < outputVector.size(); ++i)
		{
			ASSERT_STREQ(outputVector[i].str.c_str(), "This is a test");
			ASSERT_EQ(outputVector[i].num, i);
		}
	}

	TEST(MemoryArchive, OperatorArraySimple)
	{
		Array<size_t, 50> inputArray;

		for (size_t i = 0; i < inputArray.size(); ++i)
		{
			inputArray[i] = i;
		}

		MemoryWriter writer;
		writer << inputArray;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Array<size_t, 50> outputArray;
		reader << outputArray;

		ASSERT_EQ(outputArray.size(), inputArray.size());

		for (size_t i = 0; i < outputArray.size(); ++i)
		{
			ASSERT_EQ(outputArray[i], i);
		}
	}

	TEST(MemoryArchive, OperatorArrayComplex)
	{
		Array<ComplexStruct, 50> inputArray;

		for (size_t i = 0; i < inputArray.size(); ++i)
		{
			inputArray[i].str = "This is a test";
			inputArray[i].num = i;
		}

		MemoryWriter writer;
		writer << inputArray;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Array<ComplexStruct, 50> outputArray;
		reader << outputArray;

		ASSERT_EQ(outputArray.size(), inputArray.size());

		for (size_t i = 0; i < outputArray.size(); ++i)
		{
			ASSERT_STREQ(outputArray[i].str.c_str(), "This is a test");
			ASSERT_EQ(outputArray[i].num, i);
		}
	}

	TEST(MemoryArchive, OperatorMap)
	{
		Map<uint32_t, String> inputMap;

		for (uint32_t i = 0; i < 50; ++i)
		{
			inputMap[i] = "Test";
		}

		MemoryWriter writer;
		writer << inputMap;
		writer.Close();

		MemoryReader reader(writer.GetData(), writer.GetSize());

		Map<uint32_t, String> outputMap;
		reader << outputMap;

		ASSERT_EQ(outputMap.size(), inputMap.size());

		for (uint32_t i = 0; i < 50; ++i)
		{
			ASSERT_TRUE(outputMap.contains(i));
			ASSERT_STREQ(outputMap.at(i).c_str(), "Test");
		}
	}

	TEST(MemoryArchive, OperatorArchive)
	{
		Vector<ComplexStruct> inputVector;
		inputVector.resize(50);

		for (size_t i = 0; i < inputVector.size(); ++i)
		{
			inputVector[i].str = "This is a test";
			inputVector[i].num = i;
		}

		MemoryWriter mainWriter;
		mainWriter << inputVector;

		MemoryWriter subWriter;
		subWriter << inputVector;
		subWriter.Close();

		mainWriter << subWriter;
		mainWriter.Close();

		MemoryReader mainReader(mainWriter.GetData(), mainWriter.GetSize());
	
		Vector<ComplexStruct> outputVector;
		mainReader << outputVector;

		for (size_t i = 0; i < outputVector.size(); ++i)
		{
			ASSERT_STREQ(outputVector[i].str.c_str(), "This is a test");
			ASSERT_EQ(outputVector[i].num, i);
		}

		MemoryReader subReader;
		mainReader << subReader;

		subReader << outputVector;

		for (size_t i = 0; i < outputVector.size(); ++i)
		{
			ASSERT_STREQ(outputVector[i].str.c_str(), "This is a test");
			ASSERT_EQ(outputVector[i].num, i);
		}
	}
}
