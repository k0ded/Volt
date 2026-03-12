#include <CoreUtilities/Containers/Vector.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	struct ValueWithDestructor
	{
		ValueWithDestructor() = default;
		ValueWithDestructor(int32_t inValue)
			: value(inValue)
		{
		}
		~ValueWithDestructor()
		{
			value = -1;
		}

		inline bool operator==(const ValueWithDestructor& other) const
		{
			return value == other.value;
		}

		int32_t value;
	};

	template<typename VectorType>
	int32_t SetupVectorWithValues(VectorType& vector)
	{
		// We use resize and operator[] here,
		// as they are the most primitive way to set data to a vector.
		// We also take the vector by reference to make sure no copies or moved happen.
		constexpr int32_t NumValues = 32;

		vector.resize(NumValues);

		for (int32_t i = 0; i < NumValues; ++i)
		{
			vector[i].value = i;
		}

		return NumValues;
	}

	template<typename VectorType>
	void Test_Swap()
	{
		VectorType v0;
		SetupVectorWithValues(v0);

		const size_t v0Size = v0.size();

		VectorType v1;

		v1.swap(v0);

		ASSERT_EQ(v1.size(), v0Size);

		for (size_t i = 0; i < v1.size(); ++i)
		{
			ASSERT_EQ(v1[i].value, i);
		}
	}

	template<typename VectorType>
	void Test_AssignCountValue()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.assign(NumValues, ValueWithDestructor(2));

		ASSERT_EQ(v0.size(), NumValues);

		for (int32_t i = 0; i < static_cast<int32_t>(v0.size()); ++i)
		{
			ASSERT_EQ(v0[i].value, 2);
		}
	}

	template<typename VectorType>
	void Test_AssignIterator()
	{
		VectorType v0;
		SetupVectorWithValues(v0);

		VectorType v1;
		v1.assign(v0.begin(), v0.end());

		ASSERT_EQ(v1.size(), v0.size());

		for (size_t i = 0; i < v1.size(); ++i)
		{
			ASSERT_EQ(v0[i].value, v1[i].value);
		}
	}

	template<typename VectorType>
	void Test_AssignInitializerList()
	{
		std::initializer_list<ValueWithDestructor> initList = { { 0 }, { 1 }, { 2 }, { 3 }, { 4 } };

		VectorType v0;
		v0.assign(initList);

		ASSERT_EQ(v0.size(), 5);

		for (size_t i = 0; i < 5; ++i)
		{
			ASSERT_EQ(v0[i].value, i);
		}
	}

	template<typename VectorType>
	void Test_Append()
	{
		VectorType v0;
		SetupVectorWithValues(v0);

		VectorType v1;
		SetupVectorWithValues(v1);

		const size_t v0Size = v0.size();

		v0.append(v1);

		ASSERT_EQ(v0.size(), v0Size + v1.size());

		for (size_t i = 0; i < v0.size(); ++i)
		{
			ASSERT_EQ(v0[i].value, i % v0Size);
		}
	}

	template<typename VectorType>
	void Test_Empty()
	{
		VectorType v0;
		ASSERT_EQ(v0.empty(), true);

		v0.push_back({ 0 });
		ASSERT_EQ(v0.empty(), false);
	}

	template<typename VectorType>
	void Test_Size()
	{
		VectorType v0;
		const int32_t numValues = SetupVectorWithValues(v0);

		ASSERT_EQ(v0.size(), numValues);
	}

	template<typename VectorType>
	void Test_ByteSize()
	{
		VectorType v0;
		const int32_t numValues = SetupVectorWithValues(v0);

		ASSERT_EQ(v0.byte_size(), numValues * sizeof(int32_t));
	}

	template<typename VectorType>
	void Test_Capacity()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.reserve(NumValues);

		ASSERT_EQ(v0.capacity(), NumValues);
	}

	template<typename VectorType>
	void Test_ResizeValue()
	{
		constexpr int32_t NumValues = 32;
		constexpr int32_t Value = 2;

		VectorType v0;
		v0.resize(NumValues, { Value });

		ASSERT_EQ(v0.size(), NumValues);

		for (int32_t i = 0; i < NumValues; ++i)
		{
			ASSERT_EQ(v0[i].value, Value);
		}
	}

	template<typename VectorType>
	void Test_Resize()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.resize(NumValues);

		ASSERT_EQ(v0.size(), NumValues);
	}

	template<typename VectorType>
	void Test_ResizeUninitialized()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.resize(NumValues);

		ASSERT_EQ(v0.size(), NumValues);
	}

	template<typename VectorType>
	void Test_Reserve()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.reserve(NumValues);

		ASSERT_EQ(v0.capacity(), NumValues);
	}

	template<typename VectorType>
	void Test_SetCapacity()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.reserve(NumValues);
		v0.push_back({ 0 });

		// Set capacity with no argument will reallocate the vector
		// to the size of the vector.
		v0.set_capacity();

		ASSERT_EQ(v0.size(), v0.capacity());

		v0.set_capacity(NumValues);

		ASSERT_EQ(v0.capacity(), NumValues);
	}

	template<typename VectorType>
	void Test_ShrinkToFit()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;
		v0.reserve(NumValues);
		v0.push_back({ 2 });

		ASSERT_EQ(v0.capacity(), NumValues);

		v0.shrink_to_fit();

		ASSERT_EQ(v0.capacity(), v0.size());
	}

	template<typename VectorType>
	void Test_PushBack()
	{
		constexpr int32_t NumValues = 32;

		VectorType v0;

		for (int32_t i = 0; i < NumValues; ++i)
		{
			v0.push_back({ i });
		}

		ASSERT_EQ(v0.size(), NumValues);

		for (int32_t i = 0; i < NumValues; ++i)
		{
			ASSERT_EQ(v0[i].value, i);
		}
	}

	template<typename VectorType>
	void Test_PopBack()
	{
		VectorType v0;
		const size_t numValues = SetupVectorWithValues(v0);

		v0.pop_back();

		ASSERT_EQ(v0.size(), numValues - 1);

		for (size_t i = 0; i < v0.size(); ++i)
		{
			ASSERT_EQ(v0[i].value, i);
		}
	}

	template<typename VectorType>
	void Test_Emplace()
	{
		VectorType v0;

		v0.resize(2, { 2 });
		v0.emplace(v0.begin() + 1, ValueWithDestructor{ 3 });

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 2);
		ASSERT_EQ(v0[1].value, 3);
		ASSERT_EQ(v0[2].value, 2);
	}

	template<typename VectorType>
	void Test_EmplaceBack()
	{
		VectorType v0;

		v0.resize(2, { 2 });

		auto& value = v0.emplace_back(4);

		ASSERT_EQ(value.value, 4);
		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 2);
		ASSERT_EQ(v0[1].value, 2);
		ASSERT_EQ(v0[2].value, 4);
	}

	template<typename VectorType>
	void Test_InsertValue()
	{
		VectorType v0;

		v0.resize(2, { 2 });
		v0.insert(v0.begin() + 1, { 3 });

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 2);
		ASSERT_EQ(v0[1].value, 3);
		ASSERT_EQ(v0[2].value, 2);
	}

	template<typename VectorType>
	void Test_InsertValueRange()
	{
		VectorType v0;

		v0.resize(2, { 2 });
		v0.insert(v0.begin() + 1, 2, { 3 });

		ASSERT_EQ(v0.size(), 4);

		ASSERT_EQ(v0[0].value, 2);
		ASSERT_EQ(v0[1].value, 3);
		ASSERT_EQ(v0[2].value, 3);
		ASSERT_EQ(v0[3].value, 2);
	}

	template<typename VectorType>
	void Test_InsertIterator()
	{
		VectorType v0;
		v0.resize(2, { 2 });

		VectorType v1;
		v1.resize(3, { 3 });

		v0.insert(v0.begin() + 1, v1.begin(), v1.end());

		ASSERT_EQ(v0.size(), 5);

		ASSERT_EQ(v0[0].value, 2);
		ASSERT_EQ(v0[1].value, 3);
		ASSERT_EQ(v0[2].value, 3);
		ASSERT_EQ(v0[3].value, 3);
		ASSERT_EQ(v0[4].value, 2);
	}

	template<typename VectorType>
	void Test_EraseFirst()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_first(2);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 2);
		ASSERT_EQ(v0[2].value, 1);
	}

	template<typename VectorType>
	void Test_EraseFirstUnsorted()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_first_unsorted(2);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 1);
		ASSERT_EQ(v0[2].value, 2);
	}

	template<typename VectorType>
	void Test_EraseLast()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_last(2);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 2);
		ASSERT_EQ(v0[2].value, 1);
	}

	template<typename VectorType>
	void Test_EraseLastUnsorted()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_last_unsorted(2);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 2);
		ASSERT_EQ(v0[2].value, 1);
	}

	template<typename VectorType>
	void Test_Erase()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase(v0.begin() + 1);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 2);
		ASSERT_EQ(v0[2].value, 1);
	}

	template<typename VectorType>
	void Test_EraseValueRange()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase(v0.begin() + 1, v0.begin() + 3);

		ASSERT_EQ(v0.size(), 2);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 1);
	}

	template<typename VectorType>
	void Test_EraseUnsorted()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_unsorted(v0.begin() + 1);

		ASSERT_EQ(v0.size(), 3);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 1);
		ASSERT_EQ(v0[2].value, 2);
	}

	template<typename VectorType>
	void Test_EraseWithPredicate()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.erase_with_predicate([](const auto& value)
		{
			return value.value == 2;
		});

		ASSERT_EQ(v0.size(), 2);

		ASSERT_EQ(v0[0].value, 1);
		ASSERT_EQ(v0[1].value, 1);
	}

	template<typename VectorType>
	void Test_Clear()
	{
		VectorType v0;

		v0.push_back({ 1 });
		v0.push_back({ 2 });
		v0.push_back({ 2 });
		v0.push_back({ 1 });

		v0.clear();

		ASSERT_EQ(v0.size(), 0);
	}

	template<typename VectorType>
	void Test_ResizeMultiple()
	{
		VectorType v0;

		for (size_t i = 0; i < 2048; ++i)
		{
			v0.emplace_back(i);
		}

		VectorType v1;
		for (size_t i = 0; i < v0.size(); ++i)
		{
			v1.emplace_back(v0.at(i));
		}

		v0.clear();
		v1.clear();

		ASSERT_EQ(v0.size(), 0);
		ASSERT_EQ(v1.size(), 0);
	}

	TEST(Vector, Swap)
	{
		Test_Swap<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, AssignCountValue)
	{
		Test_AssignCountValue<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, AssignIterator)
	{
		Test_AssignIterator<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, AssignInitializerList)
	{
		Test_AssignInitializerList<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Append)
	{
		Test_Append<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Empty)
	{
		Test_Empty<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Size)
	{
		Test_Size<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, ByteSize)
	{
		Test_ByteSize<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Capacity)
	{
		Test_Capacity<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, ResizeValue)
	{
		Test_ResizeValue<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Resize)
	{
		Test_Resize<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, ResizeUninitialized)
	{
		Test_ResizeUninitialized<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Reserve)
	{
		Test_Reserve<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, SetCapacity)
	{
		Test_SetCapacity<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, ShrinkToFit)
	{
		Test_ShrinkToFit<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, PushBack)
	{
		Test_PushBack<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, PopBack)
	{
		Test_PopBack<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Emplace)
	{
		Test_Emplace<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EmplaceBack)
	{
		Test_EmplaceBack<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, InsertValue)
	{
		Test_InsertValue<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, InsertValueRange)
	{
		Test_InsertValueRange<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, InsertIterator)
	{
		Test_InsertIterator<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseFirst)
	{
		Test_EraseFirst<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseFirstUnsorted)
	{
		Test_EraseFirstUnsorted<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseLast)
	{
		Test_EraseLast<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseLastUnsorted)
	{
		Test_EraseLastUnsorted<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Erase)
	{
		Test_Erase<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseValueRange)
	{
		Test_EraseValueRange<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseUnsorted)
	{
		Test_EraseUnsorted<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, EraseWithPredicate)
	{
		Test_EraseWithPredicate<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, Clear)
	{
		Test_Clear<Vector<ValueWithDestructor>>();
	}

	TEST(Vector, ResizeMultiple)
	{
		Test_ResizeMultiple<Vector<ValueWithDestructor>>();
	}

	using VectorWithInlineAllocator = Vector<ValueWithDestructor, InlineAllocator<128>>;

	TEST(Vector_InlineAllocator, Swap)
	{
		Test_Swap<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, AssignCountValue)
	{
		Test_AssignCountValue<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, AssignIterator)
	{
		Test_AssignIterator<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, AssignInitializerList)
	{
		Test_AssignInitializerList<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Append)
	{
		Test_Append<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Empty)
	{
		Test_Empty<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Size)
	{
		Test_Size<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, ByteSize)
	{
		Test_ByteSize<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Capacity)
	{
		Test_Capacity<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, ResizeValue)
	{
		Test_ResizeValue<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Resize)
	{
		Test_Resize<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, ResizeUninitialized)
	{
		Test_ResizeUninitialized<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Reserve)
	{
		Test_Reserve<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, SetCapacity)
	{
		Test_SetCapacity<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, ShrinkToFit)
	{
		Test_ShrinkToFit<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, PushBack)
	{
		Test_PushBack<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, PopBack)
	{
		Test_PopBack<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Emplace)
	{
		Test_Emplace<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EmplaceBack)
	{
		Test_EmplaceBack<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, InsertValue)
	{
		Test_InsertValue<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, InsertValueRange)
	{
		Test_InsertValueRange<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, InsertIterator)
	{
		Test_InsertIterator<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseFirst)
	{
		Test_EraseFirst<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseFirstUnsorted)
	{
		Test_EraseFirstUnsorted<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseLast)
	{
		Test_EraseLast<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseLastUnsorted)
	{
		Test_EraseLastUnsorted<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Erase)
	{
		Test_Erase<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseValueRange)
	{
		Test_EraseValueRange<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseUnsorted)
	{
		Test_EraseUnsorted<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, EraseWithPredicate)
	{
		Test_EraseWithPredicate<VectorWithInlineAllocator>();
	}

	TEST(Vector_InlineAllocator, Clear)
	{
		Test_Clear<VectorWithInlineAllocator>();
	}

}
