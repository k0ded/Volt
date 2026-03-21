#include <CoreUtilities/Pointers/Ref.h>
#include <CoreUtilities/Pointers/Weak.h>

#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <atomic>

namespace UnitTests
{
	static std::atomic<int32_t> s_refTestInstances = 0;

	struct RefTestObject
	{
		int32_t value;

		RefTestObject(int32_t v = 0) : value(v) { s_refTestInstances.fetch_add(1, std::memory_order::relaxed); }
		~RefTestObject() { s_refTestInstances.fetch_sub(1, std::memory_order::relaxed); }
	};

	struct RefDerivedObject : RefTestObject
	{
		RefDerivedObject(int32_t v = 0) : RefTestObject(v) {}
	};

	// Default constructor
	TEST(Ref, DefaultConstructor)
	{
		Ref<RefTestObject> ptr;
		ASSERT_FALSE(ptr);
		ASSERT_EQ(ptr.GetRaw(), nullptr);
	}

	// Construct from nullptr
	TEST(Ref, ConstructFromNullptr)
	{
		Ref<RefTestObject> ptr(nullptr);
		ASSERT_FALSE(ptr);
		ASSERT_EQ(ptr.GetRaw(), nullptr);
	}

	// Construct from raw pointer
	TEST(Ref, ConstructFromRawPointer)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(42));
			ASSERT_TRUE(ptr);
			ASSERT_EQ(ptr->value, 42);
			ASSERT_EQ(s_refTestInstances.load(), 1);

			Weak<RefTestObject> weak = ptr;
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Copy constructor
	TEST(Ref, CopyConstructor)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> original(new RefTestObject(10));
			Ref<RefTestObject> copy(original);

			ASSERT_TRUE(copy);
			ASSERT_EQ(copy->value, 10);
			ASSERT_EQ(copy.GetRaw(), original.GetRaw());
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Copy assignment
	TEST(Ref, CopyAssignment)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> a(new RefTestObject(10));
			Ref<RefTestObject> b(new RefTestObject(20));

			ASSERT_EQ(s_refTestInstances.load(), 2);

			b = a;
			ASSERT_EQ(b->value, 10);
			ASSERT_EQ(b.GetRaw(), a.GetRaw());
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Copy self-assignment
	TEST(Ref, CopySelfAssignment)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(42));
			ptr = ptr;

			ASSERT_TRUE(ptr);
			ASSERT_EQ(ptr->value, 42);
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Move constructor
	TEST(Ref, MoveConstructor)
	{
		s_refTestInstances.store(0);
		Ref<RefTestObject> original(new RefTestObject(10));
		RefTestObject* rawPtr = original.GetRaw();

		Ref<RefTestObject> moved(std::move(original));

		ASSERT_FALSE(original);
		ASSERT_TRUE(moved);
		ASSERT_EQ(moved.GetRaw(), rawPtr);
		ASSERT_EQ(moved->value, 10);
		ASSERT_EQ(s_refTestInstances.load(), 1);
	}

	// Move assignment
	TEST(Ref, MoveAssignment)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> a(new RefTestObject(10));
			Ref<RefTestObject> b(new RefTestObject(20));

			ASSERT_EQ(s_refTestInstances.load(), 2);

			RefTestObject* rawA = a.GetRaw();
			b = std::move(a);

			ASSERT_FALSE(a);
			ASSERT_TRUE(b);
			ASSERT_EQ(b.GetRaw(), rawA);
			ASSERT_EQ(b->value, 10);
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Move self-assignment
	TEST(Ref, MoveSelfAssignment)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(42));
			RefTestObject* rawPtr = ptr.GetRaw();

			ptr = std::move(ptr);

			ASSERT_TRUE(ptr);
			ASSERT_EQ(ptr.GetRaw(), rawPtr);
			ASSERT_EQ(ptr->value, 42);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Assign nullptr
	TEST(Ref, AssignNullptr)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(42));
			ASSERT_EQ(s_refTestInstances.load(), 1);

			ptr = nullptr;
			ASSERT_FALSE(ptr);
			ASSERT_EQ(ptr.GetRaw(), nullptr);
			ASSERT_EQ(s_refTestInstances.load(), 0);
		}
	}

	// Reset
	TEST(Ref, Reset)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(42));
			ASSERT_EQ(s_refTestInstances.load(), 1);

			ptr.Reset();
			ASSERT_FALSE(ptr);
			ASSERT_EQ(ptr.GetRaw(), nullptr);
			ASSERT_EQ(s_refTestInstances.load(), 0);
		}
	}

	// Reset on null
	TEST(Ref, ResetOnNull)
	{
		Ref<RefTestObject> ptr;
		ptr.Reset();
		ASSERT_FALSE(ptr);
	}

	// Reset with new value
	TEST(Ref, ResetWithNewValue)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(10));
			ASSERT_EQ(s_refTestInstances.load(), 1);

			ptr.Reset(new RefTestObject(20));
			ASSERT_EQ(ptr->value, 20);
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// operator->
	TEST(Ref, ArrowOperator)
	{
		Ref<RefTestObject> ptr(new RefTestObject(99));
		ASSERT_EQ(ptr->value, 99);

		ptr->value = 100;
		ASSERT_EQ(ptr->value, 100);
	}

	// operator-> const
	TEST(Ref, ArrowOperatorConst)
	{
		Ref<RefTestObject> ptr(new RefTestObject(99));
		const Ref<RefTestObject>& constRef = ptr;
		ASSERT_EQ(constRef->value, 99);
	}

	// operator*
	TEST(Ref, DereferenceOperator)
	{
		Ref<RefTestObject> ptr(new RefTestObject(55));
		RefTestObject& ref = *ptr;
		ASSERT_EQ(ref.value, 55);

		ref.value = 56;
		ASSERT_EQ(ptr->value, 56);
	}

	// operator* const
	TEST(Ref, DereferenceOperatorConst)
	{
		Ref<RefTestObject> ptr(new RefTestObject(55));
		const Ref<RefTestObject>& constRef = ptr;
		const RefTestObject& ref = *constRef;
		ASSERT_EQ(ref.value, 55);
	}

	// operator bool
	TEST(Ref, BoolConversion)
	{
		Ref<RefTestObject> null;
		ASSERT_FALSE(null);

		Ref<RefTestObject> valid(new RefTestObject(1));
		ASSERT_TRUE(valid);
	}

	// operator== same type
	TEST(Ref, EqualitySameType)
	{
		Ref<RefTestObject> a;
		Ref<RefTestObject> b;
		ASSERT_TRUE(a == b);

		Ref<RefTestObject> c(new RefTestObject(1));
		ASSERT_FALSE(a == c);

		Ref<RefTestObject> d(c);
		ASSERT_TRUE(c == d);
	}

	// operator== nullptr
	TEST(Ref, EqualityNullptr)
	{
		Ref<RefTestObject> null;
		ASSERT_TRUE(null == nullptr);

		Ref<RefTestObject> valid(new RefTestObject(1));
		ASSERT_FALSE(valid == nullptr);
	}

	// GetRaw
	TEST(Ref, GetRaw)
	{
		Ref<RefTestObject> ptr(new RefTestObject(42));
		ASSERT_NE(ptr.GetRaw(), nullptr);
		ASSERT_EQ(ptr.GetRaw()->value, 42);
	}

	// GetRaw const
	TEST(Ref, GetRawConst)
	{
		Ref<RefTestObject> ptr(new RefTestObject(42));
		const Ref<RefTestObject>& constRef = ptr;
		ASSERT_NE(constRef.GetRaw(), nullptr);
		ASSERT_EQ(constRef.GetRaw()->value, 42);
	}

	// Swap
	TEST(Ref, Swap)
	{
		Ref<RefTestObject> a(new RefTestObject(10));
		Ref<RefTestObject> b(new RefTestObject(20));
		RefTestObject* rawA = a.GetRaw();
		RefTestObject* rawB = b.GetRaw();

		a.Swap(b);

		ASSERT_EQ(a.GetRaw(), rawB);
		ASSERT_EQ(b.GetRaw(), rawA);
		ASSERT_EQ(a->value, 20);
		ASSERT_EQ(b->value, 10);
	}

	// Polymorphic copy construct from derived
	TEST(Ref, CopyConstructFromDerived)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefDerivedObject> derived(new RefDerivedObject(77));
			Ref<RefTestObject> base(derived);

			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 77);
			ASSERT_EQ(base.GetRaw(), derived.GetRaw());
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Polymorphic copy assign from derived
	TEST(Ref, CopyAssignFromDerived)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefDerivedObject> derived(new RefDerivedObject(88));
			Ref<RefTestObject> base;
			base = derived;

			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 88);
			ASSERT_EQ(base.GetRaw(), derived.GetRaw());
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Polymorphic move construct from derived
	TEST(Ref, MoveConstructFromDerived)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefDerivedObject> derived(new RefDerivedObject(77));
			Ref<RefTestObject> base(std::move(derived));

			ASSERT_FALSE(derived);
			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 77);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Polymorphic move assign from derived
	TEST(Ref, MoveAssignFromDerived)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefDerivedObject> derived(new RefDerivedObject(88));
			Ref<RefTestObject> base;
			base = std::move(derived);

			ASSERT_FALSE(derived);
			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 88);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Multiple copies share ownership
	TEST(Ref, MultipleCopiesSharedOwnership)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> a(new RefTestObject(1));
			auto b = a;
			auto c = b;
			auto d = c;

			ASSERT_EQ(a.GetRaw(), d.GetRaw());

			d.Reset();
			c.Reset();
			b.Reset();

			ASSERT_TRUE(a);
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Destructor decrements ref count and destroys when zero
	TEST(Ref, DestructorDecrementsAndDestroys)
	{
		s_refTestInstances.store(0);
		Ref<RefTestObject> outer(new RefTestObject(1));
		{
			auto inner = outer;
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_TRUE(outer);
		ASSERT_EQ(s_refTestInstances.load(), 1);
	}

	// Object is deleted only when last ref is gone
	TEST(Ref, LastRefDeletesObject)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> a(new RefTestObject(42));
			{
				Ref<RefTestObject> b(a);
				{
					Ref<RefTestObject> c(b);
					ASSERT_EQ(s_refTestInstances.load(), 1);
				}
				ASSERT_EQ(s_refTestInstances.load(), 1);
			}
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// --- Multithreading tests ---

	// Concurrent copy and destroy from multiple threads
	TEST(Ref, MT_ConcurrentCopyAndDestroy)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> shared(new RefTestObject(42));

			constexpr int32_t numThreads = 16;
			constexpr int32_t iterationsPerThread = 10000;

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([&shared]()
				{
					for (int32_t i = 0; i < iterationsPerThread; ++i)
					{
						Ref<RefTestObject> local(shared);
						ASSERT_EQ(local->value, 42);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_TRUE(shared);
			ASSERT_EQ(shared->value, 42);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Concurrent copy assignment from multiple threads
	TEST(Ref, MT_ConcurrentCopyAssignment)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> shared(new RefTestObject(99));

			constexpr int32_t numThreads = 16;
			constexpr int32_t iterationsPerThread = 10000;

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([&shared]()
				{
					for (int32_t i = 0; i < iterationsPerThread; ++i)
					{
						Ref<RefTestObject> local;
						local = shared;
						ASSERT_EQ(local->value, 99);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_TRUE(shared);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Pass ownership between threads via move
	TEST(Ref, MT_MoveAcrossThreads)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> ptr(new RefTestObject(77));

			std::thread t([moved = std::move(ptr)]()
			{
				ASSERT_TRUE(moved);
				ASSERT_EQ(moved->value, 77);
			});

			ASSERT_FALSE(ptr);
			t.join();
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Multiple threads create copies, store in vector, then all go out of scope
	TEST(Ref, MT_ConcurrentCopyToVector)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> shared(new RefTestObject(123));

			constexpr int32_t numThreads = 8;
			constexpr int32_t copiesPerThread = 1000;

			std::vector<std::vector<Ref<RefTestObject>>> threadLocalCopies(numThreads);

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([&shared, &threadLocalCopies, t]()
				{
					threadLocalCopies[t].reserve(copiesPerThread);
					for (int32_t i = 0; i < copiesPerThread; ++i)
					{
						threadLocalCopies[t].emplace_back(shared);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_EQ(s_refTestInstances.load(), 1);

			for (auto& vec : threadLocalCopies)
			{
				vec.clear();
			}

			ASSERT_TRUE(shared);
			ASSERT_EQ(s_refTestInstances.load(), 1);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Stress test: many threads rapidly copying and resetting
	TEST(Ref, MT_StressCopyReset)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> shared(new RefTestObject(7));

			constexpr int32_t numThreads = 16;
			constexpr int32_t iterationsPerThread = 10000;

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([&shared]()
				{
					for (int32_t i = 0; i < iterationsPerThread; ++i)
					{
						Ref<RefTestObject> local(shared);
						ASSERT_TRUE(local);
						local.Reset();
						ASSERT_FALSE(local);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_TRUE(shared);
			ASSERT_EQ(shared->value, 7);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}

	// Concurrent swap between thread-local Refs
	TEST(Ref, MT_ConcurrentSwap)
	{
		s_refTestInstances.store(0);
		{
			Ref<RefTestObject> shared(new RefTestObject(42));

			constexpr int32_t numThreads = 8;
			constexpr int32_t iterationsPerThread = 5000;

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([&shared]()
				{
					for (int32_t i = 0; i < iterationsPerThread; ++i)
					{
						Ref<RefTestObject> local(shared);
						Ref<RefTestObject> other;
						local.Swap(other);

						ASSERT_FALSE(local);
						ASSERT_TRUE(other);
						ASSERT_EQ(other->value, 42);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_TRUE(shared);
		}
		ASSERT_EQ(s_refTestInstances.load(), 0);
	}
}
