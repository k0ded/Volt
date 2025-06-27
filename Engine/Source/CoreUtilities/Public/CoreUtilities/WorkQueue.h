#pragma once

#include "CoreUtilities/Containers/AtomicBitVector.h"
#include "CoreUtilities/Allocators/HeapAllocator.h"
#include "CoreUtilities/Math/Math.h"

#include <new>
#include <span>
#include <bit>
#include <atomic>
#include <memory>

enum class QueueThreadingPolicy : uint8_t
{
	SPSC = 0,
	SPMC,
	MPSC,
	MPMC
};

enum class QueueWaitPolicy : uint8_t
{
	NoWaits = 0,
	PushAwait,
	PopAwait,
	BothWait
};

constexpr bool IsAwaitPushes(QueueWaitPolicy policy)
{
	return policy == QueueWaitPolicy::PushAwait || policy == QueueWaitPolicy::BothWait;
}

constexpr bool IsAwaitPop(QueueWaitPolicy policy)
{
	return policy == QueueWaitPolicy::PopAwait || policy == QueueWaitPolicy::BothWait;
}

template<typename DataType, QueueThreadingPolicy ThreadingPolicy, QueueWaitPolicy WaitingPolicy = QueueWaitPolicy::NoWaits, typename AllocatorType = HeapAllocator>
class WorkQueue
{
public:
	using ThisType = WorkQueue<DataType, ThreadingPolicy, WaitingPolicy, AllocatorType>;

	~WorkQueue()
	{
		Free();
	}

	template<typename... Args>
	bool Emplace(Args&&... args)
	{
		VT_ENSURE(IsAllocated());

		auto [numReserved, pushIndex, unwrappedIndex] = ReserveEmplaceSlot();
		
		if (numReserved == 0)
		{
			// Queue is full
			return false;
		}

		void* ptr = m_storagePtr + pushIndex * sizeof(DataType);
		new (ptr) DataType(std::forward<Args>(args)...);

		[[maybe_unused]] bool priorBitState = m_slotFullFlags.IsBitSet(pushIndex, std::memory_order::relaxed);
		VT_ASSERT_MSG(!priorBitState, "Reserved bit is set, this is invalid!");

		// Set bit to notify poppers that this slot is ready.
		constexpr std::memory_order order = ThisType::HasSinglePusher ? std::memory_order::relaxed : std::memory_order::release;
		m_slotFullFlags.SetBit(pushIndex, true, order);
	
		if constexpr (ThisType::HasSinglePusher)
		{
			AdvanceStoredIndex(unwrappedIndex, 1);
		}

		IncreaseSize(1);

		return true;
	}

	bool Pop(DataType& outData)
	{
		VT_ENSURE(IsAllocated());

		auto [numReserved, popIndex, unwrappedIndex] = ReservePopSlot();

		if (numReserved == 0)
		{
			return false;
		}

		void* ptr = m_storagePtr + popIndex * sizeof(DataType);
		DataType* dataPtr = std::launder(reinterpret_cast<DataType*>(ptr));
		outData = std::move(*dataPtr);
		dataPtr->~DataType();

		[[maybe_unused]] bool priorBitState = m_slotFullFlags.IsBitSet(popIndex, std::memory_order::relaxed);
		VT_ASSERT_MSG(priorBitState, "Reserved bit is not set, this is invalid!");
	
		constexpr std::memory_order order = ThisType::HasSinglePopper ? std::memory_order::relaxed : std::memory_order::release;
		m_slotFullFlags.SetBit(popIndex, false, order);

		if constexpr (ThisType::HasSinglePopper)
		{
			AdvanceStoredIndex(unwrappedIndex, 1);
		}

		DecreaseSize(1);
		return true;
	}

	size_t Capacity() const
	{
		return static_cast<size_t>(m_capacity);
	}

	size_t Size() const
	{
		int32_t size = m_size.load(std::memory_order_relaxed);
		return static_cast<size_t>(size & (~ThisType::SizeMask));
	}

	bool Empty() const
	{
		return Size() == 0;
	}

	bool IsAllocated() const
	{
		return m_storagePtr != nullptr;
	}

	void Allocate(const size_t capacity)
	{
		constexpr size_t alignment = std::max(CacheLineAlignment, alignof(DataType));

		const size_t numBytesToAllocate = sizeof(DataType) * capacity;
		m_storagePtr = reinterpret_cast<uint8_t*>(m_allocator.Allocate(numBytesToAllocate, alignment));
		VT_ASSERT(m_storagePtr != nullptr);

		m_capacity = capacity;
		m_slotFullFlags.Resize(capacity);

		const int32_t numWrapArounds = std::numeric_limits<int32_t>::max() / m_capacity;
		VT_ASSERT(numWrapArounds >= 2);

		m_indexEnd = m_capacity * numWrapArounds;
	}

private:
	struct alignas(2 * alignof(int32_t)) QueueIndices
	{
		int32_t unwrappedPushIndex = 0;
		int32_t unwrappedPopIndex = 0;
	};

	struct ReservedSlotResult
	{
		int32_t numReserved = 0;
		int32_t wrappedBeginIndex = 0;
		int32_t unwrappedBeginIndex = 0;
	};

	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;
	inline static constexpr bool HasSinglePusher = ThreadingPolicy == QueueThreadingPolicy::SPMC;
	inline static constexpr bool HasSinglePopper = ThreadingPolicy == QueueThreadingPolicy::MPSC;
	inline static constexpr bool HasAwaitPushes = IsAwaitPushes(WaitingPolicy);
	inline static constexpr bool HasAwaitPop = IsAwaitPop(WaitingPolicy);
	inline static constexpr int32_t SizeMask = 0x80000000;

	void Free()
	{
		if (!m_storagePtr)
		{
			return;
		}

		m_allocator.Free(m_storagePtr);

		m_storagePtr = nullptr;
		m_capacity = 0;
	}

	ReservedSlotResult ReserveEmplaceSlot() requires (ThisType::HasSinglePusher)
	{
		// Relaxed: Only use the push index, we are the only pusher
		uint64_t storedIndices = m_pushPopIndices.load(std::memory_order_relaxed);
		QueueIndices queueIndices = std::bit_cast<QueueIndices>(storedIndices);
		int32_t pushIndex = queueIndices.unwrappedPushIndex % Capacity();

		// Make sure slot is empty
		// Acquire: Object creation cannot be reordered above this.
		if (m_slotFullFlags.IsBitSet(pushIndex, std::memory_order::acquire))
		{
			return {};
		}

		return { 1, pushIndex, queueIndices.unwrappedPushIndex };
	}

	ReservedSlotResult ReserveEmplaceSlot() requires(!ThisType::HasSinglePusher)
	{
		int32_t pushIndex;
		QueueIndices queueIndices;
		uint64_t newStoredIndices;
		bool reserved = false;

		// MPSC relaxed, MPMC aquire.
		constexpr std::memory_order indexLoadOrder = ThisType::HasSinglePopper ? std::memory_order::relaxed : std::memory_order::acquire;
	
		uint64_t storedIndices = m_pushPopIndices.load(indexLoadOrder);

		do 
		{
			queueIndices = std::bit_cast<QueueIndices>(storedIndices);
			pushIndex = queueIndices.unwrappedPushIndex % Capacity();

			int32_t diff = queueIndices.unwrappedPushIndex - queueIndices.unwrappedPopIndex;
			if ((diff == Capacity()) || (diff == (Capacity() - m_indexEnd)))
			{
				// Queue is full
				return {};
			}

			// MPMC only: Guard against popper at this index.
			if constexpr (!ThisType::HasSinglePopper)
			{
				if (m_slotFullFlags.IsBitSet(pushIndex, std::memory_order::acquire))
				{
					newStoredIndices = m_pushPopIndices.load(indexLoadOrder);
					QueueIndices newIndices = std::bit_cast<QueueIndices>(newStoredIndices);

					if (newIndices.unwrappedPushIndex == queueIndices.unwrappedPushIndex)
					{
						// Queue is full.
						return {};
					}

					// Push index changed, try again
					storedIndices = newStoredIndices;
					continue;
				}
			}

			QueueIndices newIndices{ IncrementIndex(queueIndices.unwrappedPushIndex), queueIndices.unwrappedPopIndex };
			newStoredIndices = std::bit_cast<uint64_t>(newIndices);

			if constexpr (ThisType::HasSinglePopper)
			{
				reserved = m_pushPopIndices.compare_exchange_weak(storedIndices, newStoredIndices, std::memory_order::acquire, indexLoadOrder);
			}
			else
			{
				reserved = m_pushPopIndices.compare_exchange_strong(storedIndices, newStoredIndices, std::memory_order::acq_rel, indexLoadOrder);
			}

		} while (!reserved);

		return { 1, pushIndex, queueIndices.unwrappedPushIndex };
	}

	ReservedSlotResult ReservePopSlot() requires(ThisType::HasSinglePopper)
	{
		uint64_t storedIndices = m_pushPopIndices.load(std::memory_order::relaxed);
		QueueIndices queueIndices = std::bit_cast<QueueIndices>(storedIndices);

		if (queueIndices.unwrappedPushIndex == queueIndices.unwrappedPopIndex)
		{
			// Queue is full.
			return {};
		}

		// Make sure something is in the slot.
		int32_t popIndex = queueIndices.unwrappedPopIndex % Capacity();
		if (!m_slotFullFlags.IsBitSet(popIndex, std::memory_order::acquire))
		{
			// Queue is empty.
			return {};
		}

		return { 1, popIndex, queueIndices.unwrappedPopIndex };
	}

	ReservedSlotResult ReservePopSlot() requires(!ThisType::HasSinglePopper)
	{
		bool reserved = false;
		int32_t popIndex;
		QueueIndices queueIndices;

		constexpr std::memory_order indexLoadOrder = ThisType::HasSinglePopper ? std::memory_order::relaxed : std::memory_order::acquire;

		uint64_t storedIndices = m_pushPopIndices.load(indexLoadOrder);
		do 
		{
			queueIndices = std::bit_cast<QueueIndices>(storedIndices);
			if (queueIndices.unwrappedPushIndex == queueIndices.unwrappedPopIndex)
			{
				// Queue is empty.
				return {};
			}

			popIndex = queueIndices.unwrappedPopIndex % Capacity();

			if constexpr (!ThisType::HasSinglePusher)
			{
				if (!m_slotFullFlags.IsBitSet(popIndex, std::memory_order::acquire))
				{
					uint64_t newStoredIndices = m_pushPopIndices.load(indexLoadOrder);
					QueueIndices newIndices = std::bit_cast<QueueIndices>(newStoredIndices);

					if (newIndices.unwrappedPopIndex == queueIndices.unwrappedPopIndex)
					{
						return {};
					}

					storedIndices = newStoredIndices;
					continue;
				}
			}

			QueueIndices newIndices{ queueIndices.unwrappedPushIndex, IncrementIndex(queueIndices.unwrappedPopIndex) };
			VT_ENSURE((newIndices.unwrappedPushIndex >= newIndices.unwrappedPopIndex) || (newIndices.unwrappedPopIndex - newIndices.unwrappedPushIndex) > (m_indexEnd / 2));

			uint64_t newStoredIndices = std::bit_cast<uint64_t>(newIndices);

			if constexpr (ThisType::HasSinglePusher)
			{
				reserved = m_pushPopIndices.compare_exchange_weak(storedIndices, newStoredIndices, std::memory_order::acquire, indexLoadOrder);
			}
			else
			{
				reserved = m_pushPopIndices.compare_exchange_strong(storedIndices, newStoredIndices, std::memory_order::acq_rel, indexLoadOrder);
			}

		} while (!reserved);

		return { 1, popIndex, queueIndices.unwrappedPopIndex };
	}

	int32_t IncrementIndex(int32_t index) const
	{
		int32_t incremented = index + 1;
		return (incremented < m_indexEnd) ? incremented : 0;
	}

	void AdvanceStoredIndex(int32_t index, int32_t advance)
	{
		int32_t newUnwrappedIndex = index + advance;
		newUnwrappedIndex -= (newUnwrappedIndex >= m_indexEnd) ? m_indexEnd : 0;

		uint64_t originalIndices, newIndices;
		if constexpr (ThisType::HasSinglePusher) // SPMC: Advance push index
		{
			originalIndices = std::bit_cast<uint64_t>(QueueIndices{ index, 0 });
			newIndices = std::bit_cast<uint64_t>(QueueIndices{ newUnwrappedIndex, 0 });
		}
		else // MPSC: Advance pop index
		{
			originalIndices = std::bit_cast<uint64_t>(QueueIndices{ 0, index });
			newIndices = std::bit_cast<uint64_t>(QueueIndices{ 0, newUnwrappedIndex });
		}

		uint64_t toAdd = newIndices - originalIndices; // May wrap around, is intentional.
		m_pushPopIndices.fetch_add(toAdd, std::memory_order::release);
	}
	
	void IncreaseSize(int32_t numPushed)
	{
		static constexpr std::memory_order Order = ThisType::HasAwaitPop ? std::memory_order::release : std::memory_order::relaxed;
		[[maybe_unused]] int32_t priorSize = m_size.fetch_add(numPushed, Order);

		if constexpr (ThisType::HasAwaitPop)
		{
			// If it was empty, notify all waiting threads.
			if (priorSize <= 0)
			{
				m_size.notify_all();
			}
		}
	}

	void DecreaseSize(int32_t numPopped)
	{
		static constexpr std::memory_order Order = ThisType::HasAwaitPushes ? std::memory_order::release : std::memory_order::relaxed;
		[[maybe_unused]] int32_t priorSize = m_size.fetch_sub(numPopped, Order);

		if constexpr (ThisType::HasAwaitPushes)
		{
			// If it was full, notify all waiting threads.
			int32_t actualSize = priorSize & (~ThisType::SizeMask);
			if (actualSize >= Capacity())
			{
				m_size.notify_all();
			}
		}
	}

	alignas(CacheLineAlignment) std::atomic<uint64_t> m_pushPopIndices = 0;
	alignas(CacheLineAlignment) std::atomic<int32_t> m_size = 0;
	alignas(CacheLineAlignment) uint8_t* m_storagePtr = nullptr;

	AtomicBitVector<uint64_t> m_slotFullFlags;
	int32_t m_capacity = 0;
	int32_t m_indexEnd = 0;

	AllocatorType m_allocator;
};

///// SPSC Specialization /////
template<typename DataType, QueueWaitPolicy WaitingPolicy, typename AllocatorType>
class WorkQueue<DataType, QueueThreadingPolicy::SPSC, WaitingPolicy, AllocatorType>
{
private:
	inline static constexpr bool HasAwaitPushes = IsAwaitPushes(WaitingPolicy);
	inline static constexpr bool HasAwaitPop = IsAwaitPop(WaitingPolicy);

public:
	using ThisType = WorkQueue<DataType, QueueThreadingPolicy::SPSC, WaitingPolicy, AllocatorType>;

	~WorkQueue()
	{
		Free();
	}

	template<typename... Args>
	bool Emplace(Args&&... args)
	{
		VT_ASSERT(IsAllocated());

		// Relaxed, only this thread can modify it.
		int32_t unwrappedPushIndex = m_pushIndex.load(std::memory_order::relaxed);

		// Acquire, object creation cannot be reordererd above this.
		int32_t unwrappedPopIndex = m_popIndex.load(std::memory_order::acquire);

		int32_t indexDelta = unwrappedPushIndex - unwrappedPopIndex;
		if ((indexDelta == m_capacity) || (indexDelta == (m_capacity - m_indexEnd)))
		{
			// The queue storage is full.
			return false;
		}

		// Create the object
		int32_t pushIndex = unwrappedPushIndex % m_capacity;
		void* newPtr = m_storagePtr + pushIndex * sizeof(DataType);
		new (newPtr) DataType(std::forward<Args>(args)...);

		int32_t newPushIndex = IncrementIndex(unwrappedPushIndex);

		// Release: Object creation cannot be reordered below this.
		m_pushIndex.store(newPushIndex, std::memory_order::release);

		IncreaseSize(1);
		return true;
	}

	bool Pop(DataType& outData)
	{
		VT_ASSERT(IsAllocated());

		// Acquire: The pop cannot be reordered above this.
		int32_t unwrappedPushIndex = m_pushIndex.load(std::memory_order::acquire);

		// Relaxed: Only this thread can modify this.
		int32_t unwrappedPopIndex = m_popIndex.load(std::memory_order::relaxed);

		if (unwrappedPopIndex == unwrappedPushIndex)
		{
			// The queue storage is empty
			return false;
		}

		auto popIndex = unwrappedPopIndex % m_capacity;
		void* objectPtr = m_storagePtr + popIndex * sizeof(DataType);
		DataType* dataPtr = std::launder(reinterpret_cast<DataType*>(objectPtr));

		outData = std::move(*dataPtr);
		dataPtr->~DataType();

		int32_t newPopIndex = IncrementIndex(unwrappedPopIndex);

		// Release: The pop cannot be reordered below this.
		m_popIndex.store(newPopIndex, std::memory_order::release);

		DecreaseSize(1);
		return true;
	}

	std::span<DataType> EmplaceMultiple(const std::span<DataType>& inputSpan)
	{
		VT_ASSERT(IsAllocated());

		// Relaxed, only this thread can modify it.
		int32_t unwrappedPushIndex = m_pushIndex.load(std::memory_order::relaxed);

		// Acquire, object creation cannot be reordererd above this.
		int32_t unwrappedPopIndex = m_popIndex.load(std::memory_order::acquire);

		int32_t maxPushIndex = unwrappedPopIndex + m_capacity;
		int32_t maxSlotsAvailable = maxPushIndex - unwrappedPushIndex;
		maxSlotsAvailable -= (maxSlotsAvailable >= m_indexEnd) ? m_indexEnd : 0;

		const size_t spanSize = inputSpan.size();

		int32_t numItemsToPush = std::min(spanSize, maxSlotsAvailable);

		if (numItemsToPush)
		{
			// No items were pushed, return the entire span as unfinished.
			return inputSpan;
		}

		// Setup push
		int32_t pushIndex = unwrappedPushIndex % m_capacity;
		void* pushPtr = m_storagePtr + pushIndex * sizeof(DataType);
		DataType* dataPushPtr = std::launder(reinterpret_cast<DataType*>(pushPtr));
		int32_t distanceBeyondEnd = (pushIndex + numItemsToPush) - m_capacity;

		const DataType* spanData = inputSpan.data();

		if (distanceBeyondEnd <= 0)
		{
			std::uninitialized_move_n(spanData, numItemsToPush, dataPushPtr);
		}
		else
		{
			int32_t initialLength = numItemsToPush - distanceBeyondEnd;
			std::uninitialized_move_n(spanData, initialLength, dataPushPtr);

			dataPushPtr = std::launder(reinterpret_cast<DataType*>(m_storagePtr));
			const DataType* dataToPush = spanData + initialLength;
			std::uninitialized_move_n(dataToPush, distanceBeyondEnd, dataPushPtr);
		}

		int32_t newPushIndex = IncrementIndex(unwrappedPushIndex, numItemsToPush);

		// Release: Object creation cannot be reordered below this.
		m_pushIndex.store(newPushIndex, std::memory_order::release);

		IncreaseSize(numItemsToPush);

		const DataType* remainingDataBeing = spanData + numItemsToPush;
		return std::span<DataType>(remainingDataBeing, spanSize - numItemsToPush);
	}

	template<typename ContainerType>
	void PopMultiple(ContainerType& outContainer)
	{
		VT_ASSERT(IsAllocated());

		// Acquire: The pop cannot be reordered above this.
		int32_t unwrappedPushIndex = m_pushIndex.load(std::memory_order::acquire);

		// Relaxed: Only this thread can modify this.
		int32_t unwrappedPopIndex = m_popIndex.load(std::memory_order::relaxed);

		int32_t maxSlotsAvailable = unwrappedPushIndex - unwrappedPopIndex;
		maxSlotsAvailable += (maxSlotsAvailable < 0) ? m_indexEnd : 0;
		int32_t outputSpaceAvailable = static_cast<int32_t>(outContainer.capacity() - outContainer.size());
		int32_t numToPop = std::min(outputSpaceAvailable, maxSlotsAvailable);

		if (numToPop == 0)
		{
			// The queue is empty.
			return;
		}

		auto popAndDestroy = [&](DataType* data, int32_t numToPop)
		{
			outContainer.insert(std::end(outContainer), std::move_iterator(data), std::move_iterator(data + numToPop));
			std::destroy_n(data, numToPop);
		};

		int32_t popIndex = unwrappedPopIndex % m_capacity;
		void* popPtr = m_storagePtr + popIndex * sizeof(DataType);
		DataType* dataPopPtr = std::launder(reinterpret_cast<DataType*>(popPtr));
		int32_t distanceBeyondEnd = (popIndex + numToPop) - m_capacity;

		if (distanceBeyondEnd <= 0)
		{
			popAndDestroy(dataPopPtr, numToPop);
		}
		else
		{
			int32_t initialLength = numToPop - distanceBeyondEnd;
			popAndDestroy(dataPopPtr, initialLength);

			dataPopPtr = std::launder(reinterpret_cast<DataType*>(m_storagePtr));
			popAndDestroy(dataPopPtr, distanceBeyondEnd);
		}

		int32_t newPopIndex = IncrementIndex(unwrappedPopIndex, numToPop);

		// Release: The pop cannot be reordered below this.
		m_popIndex.store(newPopIndex, std::memory_order::release);

		DecreaseSize(numToPop);
	}

	template<typename... Args>
	void EmplaceAwait(Args&&... args) requires(ThisType::HasAwaitPushes)
	{
		VT_ASSERT(IsAllocated());

		// Acquire: Need sync to see latest queue indices.
		while (!Emplace(std::forward<Args>(args)...))
		{
			m_size.wait(m_capacity, std::memory_order::acquire);
		}
	}

	void EmplaceMultipleAwait(std::span<DataType> inputSpan) requires(ThisType::HasAwaitPushes)
	{
		VT_ASSERT(IsAllocated());

		while (true)
		{
			inputSpan = EmplaceMultiple(inputSpan);
			if (inputSpan.empty())
			{
				return;
			}

			// Acquire: Need sync to see latest queue indices.
			m_size.wait(m_capacity, std::memory_order::acquire);
		}
	}

	bool PopAwait(DataType& outData) requires(ThisType::HasAwaitPop)
	{
		VT_ASSERT(IsAllocated());

		while (true)
		{
			if (Pop(outData))
			{
				return true;
			}

			// Queue is empty, wait for something to be pushed.
			// Acquire: Need sync to see latest queue indices.
			m_size.wait(0, std::memory_order::acquire);

			// If size is SizeMask, we should exit.
			if (m_size.load(std::memory_order::relaxed) == ThisType::SizeMask)
			{
				return false;
			}
		}
	}

	template<typename ContainerType>
	void PopAwaitMultiple(ContainerType& outContainer) requires(ThisType::HasAwaitPop)
	{
		VT_ASSERT(IsAllocated());

		while (true)
		{
			PopMultiple(outContainer);
			if (!outContainer.empty())
			{
				return;
			}

			// Queue is empty, wait for something to be pushed.
			// Acquire: Need sync to see latest queue indices.
			// #TODO_Ivar: Should this be relaxed?
			m_size.wait(0, std::memory_order::acquire);

			// If size is SizeMask, we should exit.
			if (m_size.load(std::memory_order::relaxed) == ThisType::SizeMask)
			{
				return false;
			}
		}
	}

	size_t Size() const
	{
		int32_t size = m_size.load(std::memory_order_relaxed);
		return static_cast<size_t>(size & (~ThisType::SizeMask));
	}

	size_t Capacity() const
	{
		return static_cast<size_t>(m_capacity);
	}

	bool IsAllocated() const 
	{
		return m_storagePtr != nullptr;
	}

	void Allocate(const size_t capacity)
	{
		constexpr size_t alignment = std::max(CacheLineAlignment, alignof(DataType));

		const size_t numBytesToAllocate = sizeof(DataType) * capacity;
		m_storagePtr = reinterpret_cast<uint8_t*>(m_allocator.Allocate(numBytesToAllocate, alignment));
		VT_ASSERT(m_storagePtr != nullptr);

		m_capacity = capacity;

		const int32_t numWrapArounds = std::numeric_limits<int32_t>::max() / m_capacity;
		VT_ASSERT(numWrapArounds >= 2);

		m_indexEnd = m_capacity * numWrapArounds;
	}

private:
	void EndPopWaiting() requires(ThisType::HasAwaitPop)
	{
		// Release: Syncs indices, and prevents code reordering after this.
		int32_t priorSize = m_size.fetch_or(ThisType::SizeMask, std::memory_order::release);

		if (priorSize == 0)
		{
			m_size.notify_all();
		}
	}

	void Free()
	{
		if (!m_storagePtr)
		{
			return;
		}

		m_allocator.Free(m_storagePtr);

		m_storagePtr = nullptr;
		m_capacity = 0;
	}

	int32_t IncrementIndex(int32_t index, int32_t increment) const
	{
		int32_t newIndex = index + increment;
		newIndex -= (newIndex >= m_indexEnd) ? m_indexEnd : 0;

		return newIndex;
	}

	int32_t IncrementIndex(int32_t index) const
	{
		int32_t incremented = index + 1;
		return (incremented < m_indexEnd) ? incremented : 0;
	}

	void IncreaseSize(int32_t numPushed)
	{
		static constexpr std::memory_order Order = HasAwaitPop ? std::memory_order::release : std::memory_order::relaxed;
		[[maybe_unused]] int32_t priorSize = m_size.fetch_add(numPushed, Order);

		if constexpr (HasAwaitPop)
		{
			// If it was empty, notify all waiting threads.
			if (priorSize == 0)
			{
				m_size.notify_all();
			}
		}
	}

	void DecreaseSize(int32_t numPopped)
	{
		static constexpr std::memory_order Order = HasAwaitPushes ? std::memory_order::release : std::memory_order::relaxed;
		[[maybe_unused]] int32_t priorSize = m_size.fetch_sub(numPopped, Order);

		if constexpr (HasAwaitPushes)
		{
			// If it was full, notify all waiting threads.
			if ((priorSize & (~ThisType::SizeMask)) == m_capacity)
			{
				m_size.notify_all();
			}
		}
	}

	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;
	inline static constexpr int32_t SizeMask = 0x80000000;

	alignas(CacheLineAlignment) std::atomic<int32_t> m_pushIndex;
	alignas(CacheLineAlignment) std::atomic<int32_t> m_popIndex;
	alignas(CacheLineAlignment) std::atomic<int32_t> m_size = 0;
	alignas(CacheLineAlignment) uint8_t* m_storagePtr = nullptr;

	int32_t m_capacity = 0;
	int32_t m_indexEnd = 0;

	AllocatorType m_allocator;
};
