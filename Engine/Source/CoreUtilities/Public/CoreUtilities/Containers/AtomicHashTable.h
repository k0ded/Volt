#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/Optional.h"
#include "CoreUtilities/Containers/Vector.h"

template<typename ValueType, typename AllocatorType = DefaultHeapAllocator>
class AtomicHashTable
{
public:
	AtomicHashTable() = default;
	~AtomicHashTable() = default;

	AtomicHashTable(const AtomicHashTable&) = delete;
	AtomicHashTable& operator=(const AtomicHashTable&) = delete;

	/*
		Inserts a value, will override if the key already exists.
	*/
	template<typename KeyType> bool Insert(const KeyType& key, const ValueType& value);

	/*
		Inserts a value if it doesn't exist, otherwise returns the value in the table.
	*/
	template<typename KeyType> Optional<ValueType> GetOrInsert(const KeyType& key, const ValueType& value);
	template<typename KeyType> Optional<ValueType> GetAndErase(const KeyType& key);
	template<typename KeyType> bool Erase(const KeyType& key);
	template<typename KeyType> Optional<ValueType> Find(const KeyType& key) const;

	size_t GetSize() const;
	bool IsEmpty() const;

	void Reserve(uint64_t num);
	void Clear();

private:
	inline static constexpr uint64_t EmptySlot = 0;
	inline static constexpr uint64_t TombstoneSlot = std::numeric_limits<uint64_t>::max();
	inline static constexpr size_t NoTombstone = std::numeric_limits<size_t>::max();

	inline static constexpr bool ValueTypeCanBeAtomic = std::is_trivially_constructible_v<ValueType> &&
		std::is_copy_constructible_v<ValueType> &&
		std::is_move_constructible_v<ValueType> &&
		std::is_copy_assignable_v<ValueType> &&
		std::is_move_assignable_v<ValueType>;

	using StoredValueType = std::conditional_t<ValueTypeCanBeAtomic, std::atomic<ValueType>, ValueType>;

	struct alignas(64) Slot
	{
		Slot() {}
		~Slot() {}

		Slot(const Slot&) {}
		Slot(Slot&&) {}

		void Store(const ValueType& inValue, std::memory_order memoryOrder);
		ValueType Get(std::memory_order memoryOrder) const;
		ValueType Exchange(const ValueType& inValue, std::memory_order memoryOrder);

		std::atomic<uint64_t> key;
		std::atomic_flag valueAvailableFlag;
		StoredValueType value;
	};

	size_t GetStartIndex(uint64_t hash) const;
	template<typename KeyType> uint64_t HashKey(const KeyType& key) const;

	Vector<Slot, AllocatorType> m_slots;
	std::atomic<uint64_t> m_size;
};

#include "CoreUtilities/Containers/AtomicHashTable.inl"
