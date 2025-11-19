#include <CoreUtilities/Variant.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	struct CounterType
	{
		static int32_t instances;
		int32_t value;
		CounterType(int32_t v = 0) : value(v) { instances++; }
		CounterType(const CounterType& other) : value(other.value) { instances++; }
		CounterType(CounterType&& other) noexcept : value(other.value) { instances++; }
		~CounterType() { instances--; }
	};

	int32_t CounterType::instances = 0;

	using TestVariant = Variant<int32_t, float, CounterType>;

	TEST(VariantTests, DefaultConstruct_IsInvalid)
	{
		TestVariant v;
		EXPECT_FALSE(v.IsValid());
	}

	TEST(VariantTests, EmplaceAndGet_int32_t)
	{
		TestVariant v;
		v.Emplace<int32_t>(42);

		EXPECT_TRUE(v.IsValid());
		EXPECT_EQ(v.Get<int32_t>(), 42);
	}

	TEST(VariantTests, EmplaceAndGet_Float)
	{
		TestVariant v;
		v.Emplace<float>(3.14f);

		EXPECT_TRUE(v.IsValid());
		EXPECT_FLOAT_EQ(v.Get<float>(), 3.14f);
	}

	TEST(VariantTests, AssignmentToDifferentType)
	{
		TestVariant v;
		v = 10;
		EXPECT_EQ(v.Get<int32_t>(), 10);

		v = 5.5f;
		EXPECT_FLOAT_EQ(v.Get<float>(), 5.5f);
	}

	TEST(VariantTests, DestructorCalledOnReassign)
	{
		{
			TestVariant v;
			v.Emplace<CounterType>(7);
			EXPECT_EQ(CounterType::instances, 1);

			v.Emplace<CounterType>(8);
			EXPECT_EQ(CounterType::instances, 1);
		}

		EXPECT_EQ(CounterType::instances, 0);
	}

	TEST(VariantTests, CopyConstructor)
	{
		TestVariant v1;
		v1.Emplace<int32_t>(123);

		TestVariant v2 = v1;

		EXPECT_TRUE(v2.IsValid());
		EXPECT_EQ(v2.Get<int32_t>(), 123);
	}

	TEST(VariantTests, CopyConstructor_ComplexType)
	{
		CounterType::instances = 0;

		TestVariant v1;
		v1.Emplace<CounterType>(9);

		EXPECT_EQ(CounterType::instances, 1);

		TestVariant v2 = v1;
		EXPECT_EQ(CounterType::instances, 2);
		EXPECT_EQ(v2.Get<CounterType>().value, 9);
	}

	TEST(VariantTests, CopyAssignment)
	{
		TestVariant v1;
		v1.Emplace<float>(99.5f);

		TestVariant v2;
		v2 = v1;

		EXPECT_TRUE(v2.IsValid());
		EXPECT_FLOAT_EQ(v2.Get<float>(), 99.5f);
	}

	TEST(VariantTests, MoveConstructor)
	{
		TestVariant v1;
		v1.Emplace<int32_t>(55);

		TestVariant v2 = std::move(v1);

		EXPECT_TRUE(v2.IsValid());
		EXPECT_EQ(v2.Get<int32_t>(), 55);
	}

	TEST(VariantTests, MoveConstructor_ComplexType)
	{
		CounterType::instances = 0;

		TestVariant v1;
		v1.Emplace<CounterType>(12);
		EXPECT_EQ(CounterType::instances, 1);

		TestVariant v2 = std::move(v1);
		EXPECT_EQ(CounterType::instances, 1);
		EXPECT_EQ(v2.Get<CounterType>().value, 12);
	}

	TEST(VariantTests, MoveAssignment)
	{
		TestVariant v1;
		v1.Emplace<int32_t>(777);

		TestVariant v2;
		v2 = std::move(v1);

		EXPECT_TRUE(v2.IsValid());
		EXPECT_EQ(v2.Get<int32_t>(), 777);
	}

	TEST(VariantTests, DestroyOnScopeExit)
	{
		CounterType::instances = 0;

		{
			TestVariant v;
			v.Emplace<CounterType>(42);
			EXPECT_EQ(CounterType::instances, 1);
		}

		EXPECT_EQ(CounterType::instances, 0);
	}

	TEST(VariantTests, ReassigningToNewTypeDestroysOld)
	{
		CounterType::instances = 0;

		TestVariant v;
		v.Emplace<CounterType>(123);
		EXPECT_EQ(CounterType::instances, 1);

		v.Emplace<int32_t>(99);

		EXPECT_EQ(CounterType::instances, 0);
		EXPECT_EQ(v.Get<int32_t>(), 99);
	}
}
