#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"

template<typename AllocatorType = DefaultHeapAllocator>
class AtomicHashTable
{
public:
	AtomicHashTable() = default;

	~AtomicHashTable()
	{

	}

	void Reserve(uint32_t numEntries)
	{
		VT_ENSURE_MSG(m_checksum.empty(), "HashTable may only be reserved once, as otherwise all hashes become invalid.");

		m_checksum.resize(numEntries);
	}

	template<typename KeyType>
	bool Insert(const KeyType& key, uint64_t& outIndex)
	{
		uint64_t hash = std::hash<KeyType>()(key);
		outIndex = hash % m_checksum.size();

		uint64_t checksum = 0;

		constexpr uint32_t NumMaxIterations = 20;

		uint32_t iteration = 0;
		while (iteration++ < NumMaxIterations)
		{
			checksum = shiftxor(hash);

			uint64_t expected = 0;
 			uint64_t storedChecksum = m_checksum[outIndex].atomic.compare_exchange_strong(expected, checksum, std::memory_order::relaxed);

			if (storedChecksum == 0 || storedChecksum == checksum)
			{
				break;
			}
			else
			{
				hash = std::hash<uint64_t>()(hash);
				outIndex = hash % m_checksum.size();
			}
		}

		return iteration <= NumMaxIterations;
	}

	template<typename KeyType>
	void Remove(const KeyType& key)
	{
		uint64_t hash = std::hash<KeyType>()(key);
		uint64_t hashIndex = hash % m_checksum.size();

		uint64_t checksum = 0;

		constexpr uint32_t NumMaxIterations = 20;

		uint32_t iteration = 0;
		while (iteration++ < NumMaxIterations)
		{
			checksum = shiftxor(hash);
		
			uint64_t storedChecksum = m_checksum[hashIndex].compare_exchange_strong(checksum, 0ull, std::memory_order::relaxed);
			if (storedChecksum == 0 || storedChecksum == checksum)
			{
				break;
			}
			else
			{
				hash = std::hash<uint64_t>()(hash);
				hashIndex = hash % m_checksum.size();
			}
		}
	}

	template<typename KeyType>
	bool Get(const KeyType& key, uint64_t& outIndex) const
	{
		uint64_t hash = std::hash<KeyType>()(key);
		outIndex = hash % m_checksum.size();

		uint64_t checksum = 0;

		constexpr uint32_t NumMaxIterations = 20;

		uint32_t iteration = 0;
		while (iteration++ < NumMaxIterations)
		{
			checksum = shiftxor(hash);

			uint64_t storedChecksum = m_checksum[outIndex].atomic.load(std::memory_order::relaxed);

			if (storedChecksum == checksum)
			{
				return true;
			}
			else if (storedChecksum == 0)
			{
				return false;
			}
			else
			{
				hash = std::hash<uint64_t>()(hash);
				outIndex = hash % m_checksum.size();
			}
		}

		return iteration <= NumMaxIterations;
	}

private:
	uint64_t shiftxor(uint64_t seed) const
	{
		seed ^= (seed << 13);
		seed ^= (seed >> 7);
		seed ^= (seed << 17);
		return seed;
	}

	struct AtomicWrapper
	{
		AtomicWrapper()
			: atomic(0)
		{}

		AtomicWrapper(const AtomicWrapper& other)
		{
			atomic.store(other.atomic);
		}

		std::atomic_uint64_t atomic;
	};

	Vector<AtomicWrapper, AllocatorType> m_checksum;
};
