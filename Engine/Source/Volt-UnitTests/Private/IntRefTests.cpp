#include <CoreUtilities/Pointers/IntRefCounted.h>

#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <atomic>

namespace UnitTests
{
	static std::atomic<int32_t> s_intRefTestInstances = 0;

	class IntRefTestObject : public IntRefCounted<IntRefTestObject>
	{
	public:
		IntRefTestObject(int32_t v = 0) : value(v) { s_intRefTestInstances.fetch_add(1, std::memory_order::relaxed); }
		~IntRefTestObject() { s_intRefTestInstances.fetch_sub(1, std::memory_order::relaxed); }

		int32_t value;
	};

	class IntRefDerivedObject : public IntRefTestObject
	{
	public:
		IntRefDerivedObject(int32_t v = 0) : IntRefTestObject(v) {}
	};

	// Default constructor
	TEST(IntRef, DefaultConstructor)
	{
		IntRef<IntRefTestObject> ptr;
		ASSERT_FALSE(ptr);
		ASSERT_EQ(ptr.GetRaw(), nullptr);
	}

	// Construct from nullptr
	TEST(IntRef, ConstructFromNullptr)
	{
		IntRef<IntRefTestObject> ptr(nullptr);
		ASSERT_FALSE(ptr);
		ASSERT_EQ(ptr.GetRaw(), nullptr);
	}

	// Create
	TEST(IntRef, Create)
	{
		s_intRefTestInstances.store(0);
		{
			auto ptr = IntRef<IntRefTestObject>::Create(42);
			ASSERT_TRUE(ptr);
			ASSERT_EQ(ptr->value, 42);
			ASSERT_EQ(ptr->GetRefCount(), 1);
			ASSERT_EQ(s_intRefTestInstances.load(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Copy constructor increments ref count
	TEST(IntRef, CopyConstructor)
	{
		s_intRefTestInstances.store(0);
		{
			auto original = IntRef<IntRefTestObject>::Create(10);
			ASSERT_EQ(original->GetRefCount(), 1);

			IntRef<IntRefTestObject> copy(original);
			ASSERT_TRUE(copy);
			ASSERT_EQ(copy->value, 10);
			ASSERT_EQ(copy.GetRaw(), original.GetRaw());
			ASSERT_EQ(original->GetRefCount(), 2);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Copy assignment increments ref count
	TEST(IntRef, CopyAssignment)
	{
		s_intRefTestInstances.store(0);
		{
			auto a = IntRef<IntRefTestObject>::Create(10);
			auto b = IntRef<IntRefTestObject>::Create(20);

			ASSERT_EQ(s_intRefTestInstances.load(), 2);

			b = a;
			ASSERT_EQ(b->value, 10);
			ASSERT_EQ(b.GetRaw(), a.GetRaw());
			ASSERT_EQ(a->GetRefCount(), 2);
			// Old 'b' object should have been destroyed
			ASSERT_EQ(s_intRefTestInstances.load(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Copy self-assignment
	TEST(IntRef, CopySelfAssignment)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		ASSERT_EQ(ptr->GetRefCount(), 1);

		ptr = ptr;
		ASSERT_TRUE(ptr);
		ASSERT_EQ(ptr->value, 42);
		ASSERT_EQ(ptr->GetRefCount(), 1);
	}

	// Move constructor
	TEST(IntRef, MoveConstructor)
	{
		s_intRefTestInstances.store(0);
		auto original = IntRef<IntRefTestObject>::Create(10);
		IntRefTestObject* rawPtr = original.GetRaw();

		IntRef<IntRefTestObject> moved(std::move(original));

		ASSERT_FALSE(original);
		ASSERT_TRUE(moved);
		ASSERT_EQ(moved.GetRaw(), rawPtr);
		ASSERT_EQ(moved->value, 10);
		ASSERT_EQ(moved->GetRefCount(), 1);
	}

	// Move assignment
	TEST(IntRef, MoveAssignment)
	{
		s_intRefTestInstances.store(0);
		{
			auto a = IntRef<IntRefTestObject>::Create(10);
			auto b = IntRef<IntRefTestObject>::Create(20);

			ASSERT_EQ(s_intRefTestInstances.load(), 2);

			IntRefTestObject* rawA = a.GetRaw();
			b = std::move(a);

			ASSERT_FALSE(a);
			ASSERT_TRUE(b);
			ASSERT_EQ(b.GetRaw(), rawA);
			ASSERT_EQ(b->value, 10);
			ASSERT_EQ(b->GetRefCount(), 1);
			// Old 'b' object destroyed
			ASSERT_EQ(s_intRefTestInstances.load(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Move self-assignment
	TEST(IntRef, MoveSelfAssignment)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		IntRefTestObject* rawPtr = ptr.GetRaw();

		ptr = std::move(ptr);
		ASSERT_TRUE(ptr);
		ASSERT_EQ(ptr.GetRaw(), rawPtr);
		ASSERT_EQ(ptr->value, 42);
		ASSERT_EQ(ptr->GetRefCount(), 1);
	}

	// Assign nullptr
	TEST(IntRef, AssignNullptr)
	{
		s_intRefTestInstances.store(0);
		{
			auto ptr = IntRef<IntRefTestObject>::Create(42);
			ASSERT_EQ(s_intRefTestInstances.load(), 1);

			ptr = nullptr;
			ASSERT_FALSE(ptr);
			ASSERT_EQ(ptr.GetRaw(), nullptr);
			ASSERT_EQ(s_intRefTestInstances.load(), 0);
		}
	}

	// Reset
	TEST(IntRef, Reset)
	{
		s_intRefTestInstances.store(0);
		{
			auto ptr = IntRef<IntRefTestObject>::Create(42);
			ASSERT_EQ(s_intRefTestInstances.load(), 1);

			ptr.Reset();
			ASSERT_FALSE(ptr);
			ASSERT_EQ(ptr.GetRaw(), nullptr);
			ASSERT_EQ(s_intRefTestInstances.load(), 0);
		}
	}

	// Reset on null is safe
	TEST(IntRef, ResetOnNull)
	{
		IntRef<IntRefTestObject> ptr;
		ptr.Reset();
		ASSERT_FALSE(ptr);
	}

	// Release
	TEST(IntRef, Release)
	{
		s_intRefTestInstances.store(0);
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		IntRefTestObject* raw = ptr.Release();

		ASSERT_FALSE(ptr);
		ASSERT_NE(raw, nullptr);
		ASSERT_EQ(raw->value, 42);
		ASSERT_EQ(raw->GetRefCount(), 1);
		ASSERT_EQ(s_intRefTestInstances.load(), 1);

		// Manual cleanup since Release doesn't DecRef
		raw->DecRef();
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// operator->
	TEST(IntRef, ArrowOperator)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(99);
		ASSERT_EQ(ptr->value, 99);

		ptr->value = 100;
		ASSERT_EQ(ptr->value, 100);
	}

	// operator-> const
	TEST(IntRef, ArrowOperatorConst)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(99);
		const IntRef<IntRefTestObject>& constRef = ptr;
		ASSERT_EQ(constRef->value, 99);
	}

	// operator*
	TEST(IntRef, DereferenceOperator)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(55);
		IntRefTestObject& ref = *ptr;
		ASSERT_EQ(ref.value, 55);

		ref.value = 56;
		ASSERT_EQ(ptr->value, 56);
	}

	// operator* const
	TEST(IntRef, DereferenceOperatorConst)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(55);
		const IntRef<IntRefTestObject>& constRef = ptr;
		const IntRefTestObject& ref = *constRef;
		ASSERT_EQ(ref.value, 55);
	}

	// operator bool
	TEST(IntRef, BoolConversion)
	{
		IntRef<IntRefTestObject> null;
		ASSERT_FALSE(null);

		auto valid = IntRef<IntRefTestObject>::Create(1);
		ASSERT_TRUE(valid);
	}

	// operator== same type
	TEST(IntRef, EqualitySameType)
	{
		IntRef<IntRefTestObject> a;
		IntRef<IntRefTestObject> b;
		ASSERT_TRUE(a == b);

		auto c = IntRef<IntRefTestObject>::Create(1);
		ASSERT_FALSE(a == c);

		IntRef<IntRefTestObject> d(c);
		ASSERT_TRUE(c == d);
	}

	// operator== nullptr
	TEST(IntRef, EqualityNullptr)
	{
		IntRef<IntRefTestObject> null;
		ASSERT_TRUE(null == nullptr);

		auto valid = IntRef<IntRefTestObject>::Create(1);
		ASSERT_FALSE(valid == nullptr);
	}

	// operator== raw pointer
	TEST(IntRef, EqualityRawPointer)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		IntRefTestObject* raw = ptr.GetRaw();
		ASSERT_TRUE(ptr == raw);
		ASSERT_FALSE(ptr == static_cast<IntRefTestObject*>(nullptr));
	}

	// GetRaw
	TEST(IntRef, GetRaw)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		ASSERT_NE(ptr.GetRaw(), nullptr);
		ASSERT_EQ(ptr.GetRaw()->value, 42);
	}

	// GetRaw const
	TEST(IntRef, GetRawConst)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		const IntRef<IntRefTestObject>& constRef = ptr;
		ASSERT_NE(constRef.GetRaw(), nullptr);
		ASSERT_EQ(constRef.GetRaw()->value, 42);
	}

	// GetHash
	TEST(IntRef, GetHash)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		size_t hash = ptr.GetHash();
		ASSERT_EQ(hash, std::hash<void*>()(ptr.GetRaw()));

		IntRef<IntRefTestObject> null;
		ASSERT_EQ(null.GetHash(), std::hash<void*>()(nullptr));
	}

	// std::hash specialization
	TEST(IntRef, StdHash)
	{
		auto ptr = IntRef<IntRefTestObject>::Create(42);
		std::hash<IntRef<IntRefTestObject>> hasher;
		ASSERT_EQ(hasher(ptr), ptr.GetHash());
	}

	// Attach bumps ref count
	TEST(IntRef, Attach)
	{
		s_intRefTestInstances.store(0);
		{
			auto original = IntRef<IntRefTestObject>::Create(42);
			IntRefTestObject* raw = original.GetRaw();
			ASSERT_EQ(raw->GetRefCount(), 1);

			auto attached = IntRef<IntRefTestObject>::Attach(raw);
			ASSERT_EQ(raw->GetRefCount(), 2);
			ASSERT_EQ(attached.GetRaw(), raw);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// AttachNoRef does not bump ref count
	TEST(IntRef, AttachNoRef)
	{
		s_intRefTestInstances.store(0);
		{
			auto original = IntRef<IntRefTestObject>::Create(42);
			IntRefTestObject* raw = original.GetRaw();
			ASSERT_EQ(raw->GetRefCount(), 1);

			// Release so only AttachNoRef holds it
			raw = original.Release();
			ASSERT_EQ(raw->GetRefCount(), 1);

			auto attached = IntRef<IntRefTestObject>::AttachNoRef(raw);
			ASSERT_EQ(raw->GetRefCount(), 1);
			ASSERT_EQ(attached.GetRaw(), raw);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Polymorphic copy construct from derived
	TEST(IntRef, CopyConstructFromDerived)
	{
		s_intRefTestInstances.store(0);
		{
			auto derived = IntRef<IntRefDerivedObject>::Create(77);
			ASSERT_EQ(derived->GetRefCount(), 1);

			IntRef<IntRefTestObject> base(derived);
			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 77);
			ASSERT_EQ(base.GetRaw(), derived.GetRaw());
			ASSERT_EQ(derived->GetRefCount(), 2);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Polymorphic copy assign from derived
	TEST(IntRef, CopyAssignFromDerived)
	{
		s_intRefTestInstances.store(0);
		{
			auto derived = IntRef<IntRefDerivedObject>::Create(88);
			IntRef<IntRefTestObject> base;
			base = derived;

			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 88);
			ASSERT_EQ(derived->GetRefCount(), 2);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Polymorphic move construct from derived
	TEST(IntRef, MoveConstructFromDerived)
	{
		s_intRefTestInstances.store(0);
		{
			auto derived = IntRef<IntRefDerivedObject>::Create(77);
			IntRef<IntRefTestObject> base(std::move(derived));

			ASSERT_FALSE(derived);
			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 77);
			ASSERT_EQ(base->GetRefCount(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Polymorphic move assign from derived
	TEST(IntRef, MoveAssignFromDerived)
	{
		s_intRefTestInstances.store(0);
		{
			auto derived = IntRef<IntRefDerivedObject>::Create(88);
			IntRef<IntRefTestObject> base;
			base = std::move(derived);

			ASSERT_FALSE(derived);
			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 88);
			ASSERT_EQ(base->GetRefCount(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// As cast
	TEST(IntRef, AsCast)
	{
		s_intRefTestInstances.store(0);
		{
			auto derived = IntRef<IntRefDerivedObject>::Create(55);
			IntRef<IntRefTestObject> base = derived.template As<IntRefTestObject>();

			ASSERT_TRUE(base);
			ASSERT_EQ(base->value, 55);
			ASSERT_EQ(base.GetRaw(), derived.GetRaw());
			ASSERT_EQ(derived->GetRefCount(), 2);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Multiple copies share ownership
	TEST(IntRef, MultipleCopiesSharedOwnership)
	{
		s_intRefTestInstances.store(0);
		{
			auto a = IntRef<IntRefTestObject>::Create(1);
			auto b = a;
			auto c = b;
			auto d = c;

			ASSERT_EQ(a->GetRefCount(), 4);
			ASSERT_EQ(a.GetRaw(), d.GetRaw());

			d.Reset();
			ASSERT_EQ(a->GetRefCount(), 3);

			c.Reset();
			ASSERT_EQ(a->GetRefCount(), 2);

			b.Reset();
			ASSERT_EQ(a->GetRefCount(), 1);
			ASSERT_EQ(s_intRefTestInstances.load(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Destructor decrements ref count and destroys when zero
	TEST(IntRef, DestructorDecrementsRefCount)
	{
		s_intRefTestInstances.store(0);
		auto outer = IntRef<IntRefTestObject>::Create(1);
		{
			auto inner = outer;
			ASSERT_EQ(outer->GetRefCount(), 2);
		}
		ASSERT_EQ(outer->GetRefCount(), 1);
		ASSERT_EQ(s_intRefTestInstances.load(), 1);
	}

	// --- Multithreading tests ---

	// Concurrent copy and destroy from multiple threads
	TEST(IntRef, MT_ConcurrentCopyAndDestroy)
	{
		s_intRefTestInstances.store(0);
		{
			auto shared = IntRef<IntRefTestObject>::Create(42);

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
						IntRef<IntRefTestObject> local(shared);
						ASSERT_EQ(local->value, 42);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_EQ(shared->GetRefCount(), 1);
			ASSERT_EQ(shared->value, 42);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Concurrent copy assignment from multiple threads
	TEST(IntRef, MT_ConcurrentCopyAssignment)
	{
		s_intRefTestInstances.store(0);
		{
			auto shared = IntRef<IntRefTestObject>::Create(99);

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
						IntRef<IntRefTestObject> local;
						local = shared;
						ASSERT_EQ(local->value, 99);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_EQ(shared->GetRefCount(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Pass ownership between threads via move
	TEST(IntRef, MT_MoveAcrossThreads)
	{
		s_intRefTestInstances.store(0);
		{
			auto ptr = IntRef<IntRefTestObject>::Create(77);

			std::thread t([moved = std::move(ptr)]()
			{
				ASSERT_TRUE(moved);
				ASSERT_EQ(moved->value, 77);
				ASSERT_EQ(moved->GetRefCount(), 1);
			});

			ASSERT_FALSE(ptr);
			t.join();
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Multiple threads create copies, store in vector, then all go out of scope
	TEST(IntRef, MT_ConcurrentCopyToVector)
	{
		s_intRefTestInstances.store(0);
		{
			auto shared = IntRef<IntRefTestObject>::Create(123);

			constexpr int32_t numThreads = 8;
			constexpr int32_t copiesPerThread = 1000;

			std::vector<std::vector<IntRef<IntRefTestObject>>> threadLocalCopies(numThreads);

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

			ASSERT_EQ(shared->GetRefCount(), 1 + numThreads * copiesPerThread);

			// Clear all thread-local copies
			for (auto& vec : threadLocalCopies)
			{
				vec.clear();
			}

			ASSERT_EQ(shared->GetRefCount(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Concurrent Attach from multiple threads
	TEST(IntRef, MT_ConcurrentAttach)
	{
		s_intRefTestInstances.store(0);
		{
			auto original = IntRef<IntRefTestObject>::Create(42);
			IntRefTestObject* raw = original.GetRaw();

			constexpr int32_t numThreads = 16;
			constexpr int32_t iterationsPerThread = 5000;

			std::vector<std::thread> threads;
			threads.reserve(numThreads);

			for (int32_t t = 0; t < numThreads; ++t)
			{
				threads.emplace_back([raw]()
				{
					for (int32_t i = 0; i < iterationsPerThread; ++i)
					{
						auto attached = IntRef<IntRefTestObject>::Attach(raw);
						ASSERT_EQ(attached->value, 42);
					}
				});
			}

			for (auto& thread : threads)
			{
				thread.join();
			}

			ASSERT_EQ(original->GetRefCount(), 1);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}

	// Stress test: many threads rapidly copying and resetting
	TEST(IntRef, MT_StressCopyReset)
	{
		s_intRefTestInstances.store(0);
		{
			auto shared = IntRef<IntRefTestObject>::Create(7);

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
						IntRef<IntRefTestObject> local(shared);
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

			ASSERT_EQ(shared->GetRefCount(), 1);
			ASSERT_EQ(shared->value, 7);
		}
		ASSERT_EQ(s_intRefTestInstances.load(), 0);
	}
}
