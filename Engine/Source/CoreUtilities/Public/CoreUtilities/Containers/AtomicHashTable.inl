#include "AtomicHashTable.h"
#pragma once

template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline bool AtomicHashTable<ValueType, AllocatorType>::Insert(const KeyType& key, const ValueType& value)
{
	const uint64_t mask = m_slots.size() - 1;
	const uint64_t keyHash = HashKey(key);
	const size_t startIndex = GetStartIndex(keyHash);

	size_t firstTombstone = NoTombstone;

	for (size_t i = 0; i < m_slots.size(); ++i)
	{
		const size_t index = (startIndex + i) & mask;
		Slot& slot = m_slots[index];

		uint64_t existing = slot.key.load(std::memory_order::acquire);

		// Key exists in table, insert new value
		if (existing == key)
		{
			slot.Store(value, std::memory_order::release);
			return true;
		}

		if (existing == TombstoneSlot)
		{
			if (firstTombstone == NoTombstone)
			{
				firstTombstone = index;
			}
			continue;
		}

		// Occupied
		if (existing != EmptySlot)
		{
			continue;
		}

		if (firstTombstone != NoTombstone)
		{
			uint64_t expected = TombstoneSlot;
			if (m_slots[firstTombstone].key.compare_exchange_strong(
				expected, keyHash,
				std::memory_order::acq_rel,
				std::memory_order::acquire))
			{
				m_slots[firstTombstone].Store(value, std::memory_order::release);
				m_size.fetch_add(1, std::memory_order::relaxed);

				return true;
			}

			firstTombstone = NoTombstone;
		}

		uint64_t expected = EmptySlot;
		if (slot.key.compare_exchange_strong(
				expected, keyHash,
				std::memory_order::acq_rel,
				std::memory_order::acquire))
		{
			slot.Store(value, std::memory_order::release);
			m_size.fetch_add(1, std::memory_order::relaxed);
			return true;
		}

		if (expected == keyHash)
		{
			slot.Store(value, std::memory_order::release);
			return true;
		}
	}

	return false;
}

template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline Optional<ValueType> AtomicHashTable<ValueType, AllocatorType>::GetOrInsert(const KeyType& key, const ValueType& value)
{
	const uint64_t mask = m_slots.size() - 1;
	const uint64_t keyHash = HashKey(key);
	const size_t startIndex = GetStartIndex(keyHash);

	size_t firstTombstone = NoTombstone;

	for (size_t i = 0; i < m_slots.size(); ++i)
	{
		const size_t index = (startIndex + i) & mask;
		Slot& slot = m_slots[index];

		uint64_t existing = slot.key.load(std::memory_order::acquire);

		// Key exists in table, insert new value
		if (existing == keyHash)
		{
			return slot.Get(std::memory_order::acquire);
		}

		if (existing == TombstoneSlot)
		{
			if (firstTombstone == NoTombstone)
			{
				firstTombstone = index;
			}
			continue;
		}

		// Occupied
		if (existing != EmptySlot)
		{
			continue;
		}

		const size_t claimIndex = (firstTombstone != NoTombstone) ? firstTombstone : index;
		Slot& claimSlot = m_slots[claimIndex];
		
		uint64_t claimExpected = (firstTombstone != NoTombstone) ? TombstoneSlot : EmptySlot;

		if (claimSlot.key.compare_exchange_strong(
			claimExpected, keyHash,
			std::memory_order::acq_rel,
			std::memory_order::acquire))
		{
			claimSlot.Store(value, std::memory_order::release);
			m_size.fetch_add(1, std::memory_order::relaxed);

			return value;
		}

		// CAS failed, another thread inserted the same key.
		// We need to do a brief wait to ensure the value has bee written.
		if (claimExpected == keyHash)
		{
			std::atomic_thread_fence(std::memory_order::acquire);

			ValueType v{};

			for (int32_t spin = 0; spin < 1024; ++spin)
			{
				v = claimSlot.Get(std::memory_order::acquire);
			}


			return v;
		}

		// Another key got the slot, continue.
		firstTombstone = NoTombstone;
	}

	return {};
}

template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline bool AtomicHashTable<ValueType, AllocatorType>::Erase(const KeyType& key)
{
	const uint64_t mask = m_slots.size() - 1;
	const uint64_t keyHash = HashKey(key);
	const size_t startIndex = GetStartIndex(keyHash);

	for (size_t i = 0; i < m_slots.size(); ++i)
	{
		const size_t index = (startIndex + i) & mask;
		Slot& slot = m_slots[index];

		uint64_t key = slot.key.load(std::memory_order::acquire);

		if (key == EmptySlot)
		{
			return false;
		}

		if (key == keyHash)
		{
			uint64_t expected = key;
			if (slot.key.compare_exchange_strong(
				expected, TombstoneSlot,
				std::memory_order::acq_rel,
				std::memory_order::acquire))
			{
				// Reset value.
				slot.Store({}, std::memory_order::release);
				m_size.fetch_sub(1, std::memory_order::relaxed);
				return true;
			}

			return false;
		}
	}

	return false;
}


template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline Optional<ValueType> AtomicHashTable<ValueType, AllocatorType>::GetAndErase(const KeyType& key)
{
	const uint64_t mask = m_slots.size() - 1;
	const uint64_t keyHash = HashKey(key);
	const size_t startIndex = GetStartIndex(keyHash);

	for (size_t i = 0; i < m_slots.size(); ++i)
	{
		const size_t index = (startIndex + i) & mask;
		Slot& slot = m_slots[index];

		uint64_t key = slot.key.load(std::memory_order::acquire);

		if (key == EmptySlot)
		{
			return {};
		}

		if (key == keyHash)
		{
			uint64_t expected = key;
			if (slot.key.compare_exchange_strong(
				expected, TombstoneSlot,
				std::memory_order::acq_rel,
				std::memory_order::acquire))
			{
				// Reset value.
				ValueType valueInSlot = slot.value.exchange({}, std::memory_order::release);
				m_size.fetch_sub(1, std::memory_order::relaxed);

				return valueInSlot;
			}

			return {};
		}
	}

	return {};
}


template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline Optional<ValueType> AtomicHashTable<ValueType, AllocatorType>::Find(const KeyType& key) const
{
	const uint64_t mask = m_slots.size() - 1;
	const uint64_t keyHash = HashKey(key);
	const size_t startIndex = GetStartIndex(keyHash);

	for (size_t i = 0; i < m_slots.size(); ++i)
	{
		const size_t index = (startIndex + i) & mask;
		const Slot& slot = m_slots[index];

		const uint64_t key = slot.key.load(std::memory_order::acquire);

		if (key == EmptySlot)
		{
			return {};
		}

		if (key == keyHash)
		{
			return slot.Get(std::memory_order::acquire);
		}
	}

	return {};
}

template<typename ValueType, typename AllocatorType>
template<typename KeyType>
inline uint64_t AtomicHashTable<ValueType, AllocatorType>::HashKey(const KeyType& key) const
{
	return std::hash<KeyType>()(key);
}

template<typename ValueType, typename AllocatorType>
inline size_t AtomicHashTable<ValueType, AllocatorType>::GetSize() const
{
	return m_size.load(std::memory_order::relaxed);
}

template<typename ValueType, typename AllocatorType>
inline bool AtomicHashTable<ValueType, AllocatorType>::IsEmpty() const
{
	return GetSize() == 0;
}

template<typename ValueType, typename AllocatorType>
inline void AtomicHashTable<ValueType, AllocatorType>::Reserve(uint64_t num)
{
	m_slots.resize(num);
}

template<typename ValueType, typename AllocatorType>
void AtomicHashTable<ValueType, AllocatorType>::Clear()
{
	m_slots.clear();
	m_size.store(0);
}

template<typename ValueType, typename AllocatorType>
inline size_t AtomicHashTable<ValueType, AllocatorType>::GetStartIndex(uint64_t hash) const
{
	return hash & (m_slots.size() - 1);
}

template<typename ValueType, typename AllocatorType>
inline void AtomicHashTable<ValueType, AllocatorType>::Slot::Store(const ValueType& inValue, std::memory_order memoryOrder)
{
	if constexpr (ValueTypeCanBeAtomic)
	{
		value.store(inValue, memoryOrder);
	}
	else
	{
		value = inValue;
	}
}

template<typename ValueType, typename AllocatorType>
inline ValueType AtomicHashTable<ValueType, AllocatorType>::Slot::Get(std::memory_order memoryOrder) const
{
	if constexpr (ValueTypeCanBeAtomic)
	{
		return value.load(memoryOrder);
	}
	else
	{
		return value;
	}
}


template<typename ValueType, typename AllocatorType>
inline ValueType AtomicHashTable<ValueType, AllocatorType>::Slot::Exchange(const ValueType& inValue, std::memory_order memoryOrder)
{
	if constexpr (ValueTypeCanBeAtomic)
	{
		return value.exchange(inValue, memoryOrder);
	}
	else
	{
		ValueType temp = value;
		value = inValue;

		return temp;
	}
}
