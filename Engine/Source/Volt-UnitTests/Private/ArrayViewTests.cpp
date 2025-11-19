#include <CoreUtilities/Containers/ArrayView.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	template<typename T>
	using TestVector = Vector<T>;

	template<typename T, size_t N>
	using TestArray = Array<T, N>;

	TEST(ArrayViewTests, DefaultConstructor)
	{
		ArrayView<int32_t> v;
		EXPECT_TRUE(v.empty());
		EXPECT_EQ(v.size(), 0u);
		EXPECT_EQ(v.data(), nullptr);
		EXPECT_EQ(v.begin(), nullptr);
		EXPECT_EQ(v.end(), nullptr);
	}

	TEST(ArrayViewTests, ConstructFromArray)
	{
		TestArray<int32_t, 4> arr = { 1, 2, 3, 4 };
		ArrayView<int32_t> v(arr);

		EXPECT_FALSE(v.empty());
		EXPECT_EQ(v.size(), 4u);
		EXPECT_EQ(v.data(), arr.data());

		EXPECT_EQ(v[0], 1);
		EXPECT_EQ(v[3], 4);
		EXPECT_EQ(v.front(), 1);
		EXPECT_EQ(v.back(), 4);
	}

	TEST(ArrayViewTests, ConstructFromVector)
	{
		TestVector<int32_t> vec;
		vec.push_back(10);
		vec.push_back(20);
		vec.push_back(30);

		ArrayView<int32_t> v(vec);

		EXPECT_FALSE(v.empty());
		EXPECT_EQ(v.size(), 3u);
		EXPECT_EQ(v.data(), vec.data());

		EXPECT_EQ(v[0], 10);
		EXPECT_EQ(v[2], 30);
	}

	TEST(ArrayViewTests, CopyConstructor)
	{
		TestArray<int32_t, 3> arr = { 4, 5, 6 };
		ArrayView<int32_t> v1(arr);
		ArrayView<int32_t> v2(v1);

		EXPECT_EQ(v1.data(), v2.data());
		EXPECT_EQ(v1.size(), v2.size());
	}

	TEST(ArrayViewTests, MoveConstructor)
	{
		TestArray<int32_t, 3> arr = { 7, 8, 9 };
		ArrayView<int32_t> v1(arr);

		ArrayView<int32_t> v2(std::move(v1));

		EXPECT_EQ(v2.size(), 3u);
		EXPECT_EQ(v2.front(), 7);

		EXPECT_EQ(v1.data(), nullptr);
		EXPECT_EQ(v1.size(), 0u);
	}

	TEST(ArrayViewTests, CopyAssignment)
	{
		TestArray<int32_t, 2> arr1 = { 1, 2 };
		TestArray<int32_t, 3> arr2 = { 3, 4, 5 };

		ArrayView<int32_t> v1(arr1);
		ArrayView<int32_t> v2(arr2);

		v2 = v1;

		EXPECT_EQ(v2.size(), 2u);
		EXPECT_EQ(v2[0], 1);
	}

	TEST(ArrayViewTests, MoveAssignment)
	{
		TestArray<int32_t, 2> arr = { 5, 6 };

		ArrayView<int32_t> v1(arr);
		ArrayView<int32_t> v2;

		v2 = std::move(v1);

		EXPECT_EQ(v2.size(), 2u);
		EXPECT_EQ(v2[1], 6);

		EXPECT_EQ(v1.data(), nullptr);
		EXPECT_EQ(v1.size(), 0u);
	}

	TEST(ArrayViewTests, AssignmentFromVector)
	{
		TestVector<int32_t> vec;
		vec.push_back(11);
		vec.push_back(22);

		ArrayView<int32_t> v;
		v = vec;

		EXPECT_EQ(v.size(), 2u);
		EXPECT_EQ(v[0], 11);
		EXPECT_EQ(v[1], 22);
	}

	TEST(ArrayViewTests, AssignmentFromArray)
	{
		TestArray<int32_t, 3> arr = { 100, 200, 300 };

		ArrayView<int32_t> v;
		v = arr;

		EXPECT_EQ(v.size(), 3u);
		EXPECT_EQ(v[2], 300);
	}

	TEST(ArrayViewTests, IteratorsWork)
	{
		TestArray<int32_t, 4> arr = { 1, 2, 3, 4 };
		ArrayView<int32_t> v(arr);

		int32_t sum = 0;
		for (auto it = v.begin(); it != v.end(); ++it)
			sum += *it;

		EXPECT_EQ(sum, 10);
	}

	TEST(ArrayViewTests, ReverseIteratorsWork)
	{
		TestArray<int32_t, 4> arr = { 1, 2, 3, 4 };
		ArrayView<int32_t> v(arr);

		std::vector<int32_t> reversed;
		for (auto it = v.rbegin(); it != v.rend(); ++it)
			reversed.push_back(*it);

		ASSERT_EQ(reversed.size(), 4u);
		EXPECT_EQ(reversed[0], 4);
		EXPECT_EQ(reversed[1], 3);
		EXPECT_EQ(reversed[2], 2);
		EXPECT_EQ(reversed[3], 1);
	}

	TEST(ArrayViewTests, ByteSize)
	{
		TestArray<int32_t, 4> arr = { 1, 2, 3, 4 };
		ArrayView<int32_t> v(arr);

		EXPECT_EQ(v.byte_size(), sizeof(int32_t) * 4);
	}

	TEST(ArrayViewTests, AtOperator)
	{
		TestArray<int32_t, 3> arr = { 9, 8, 7 };
		ArrayView<int32_t> v(arr);

		EXPECT_EQ(v.at(0), 9);
		EXPECT_EQ(v.at(2), 7);
	}

	TEST(ArrayViewTests, FrontBack)
	{
		TestArray<int32_t, 5> arr = { 10, 20, 30, 40, 50 };
		ArrayView<int32_t> v(arr);

		EXPECT_EQ(v.front(), 10);
		EXPECT_EQ(v.back(), 50);
	}
}
