#include "cupch.h"

#include "CoreUtilities/MemoryTracker.h"

VT_REGISTER_MEMORY_TAG(Unknown);

bool MemoryTracker::s_isInitialized = false;

thread_local Vector<uint32_t> g_activeMemoryTag;
thread_local uint32_t g_depth = 0; // Used to ensure that we do not end up in a recursive state adding to the above vector.

MemoryTagRegistry& MemoryTagRegistry::Get()
{
	static MemoryTagRegistry instance;
	return instance;
}

void MemoryTagRegistry::AddTagToTrackerIfRequired(size_t tagHash, uint32_t tagIndex)
{
	if (MemoryTracker::s_isInitialized)
	{
		MemoryTracker::Get().AddTag(tagHash, tagIndex);
	}
}

void MemoryTracker::Initialize()
{
	MemoryTracker& instance = Get();
	MemoryTagRegistry& registryInstance = MemoryTagRegistry::Get();

	for (const auto&[hash, tagInfo] : registryInstance.m_memoryTagInfos)
	{
		instance.m_tagHashToTagIndex[hash] = tagInfo.tagIndex;
	}

	instance.m_memoryTagCounters.resize(registryInstance.m_nextTagIndex);

	s_isInitialized = true;
}

void MemoryTracker::PopMemoryTag()
{
	if (s_isInitialized)
	{
		Get().PopMemoryTagInternal();
	}
}

void MemoryTracker::OnAllocate(MemoryTrackerHeader* header)
{
	if (!s_isInitialized)
	{
		return;
	}

	if (g_depth > 0)
	{
		return;
	}

	g_depth++;
	MemoryTracker& instance = Get();

	if (g_activeMemoryTag.empty())
	{
		g_activeMemoryTag.push_back(instance.m_tagHashToTagIndex.at(MemoryTag::Unknown::Hash.hash));
	}
	
	header->memoryTagIndex = g_activeMemoryTag.back();
	instance.m_memoryTagCounters[header->memoryTagIndex].counter.fetch_add(header->size, std::memory_order::relaxed);

	g_depth--;
}

void MemoryTracker::OnFree(MemoryTrackerHeader* header)
{
	if (!s_isInitialized)
	{
		return;
	}

	if (g_depth > 0)
	{
		return;
	}

	g_depth++;

	MemoryTracker& instance = Get();
	instance.m_memoryTagCounters[header->memoryTagIndex].counter.fetch_sub(header->size, std::memory_order::relaxed);

	g_depth--;
}

void MemoryTracker::OnReallocate(MemoryTrackerHeader* header, uint64_t oldSize)
{
	if (!s_isInitialized)
	{
		return;
	}

	if (g_depth > 0)
	{
		return;
	}

	g_depth++;

	MemoryTracker& instance = Get();
	instance.m_memoryTagCounters[header->memoryTagIndex].counter.fetch_sub(oldSize, std::memory_order::relaxed);
	instance.m_memoryTagCounters[header->memoryTagIndex].counter.fetch_add(header->size, std::memory_order::relaxed);

	g_depth--;
}

MemoryTracker& MemoryTracker::Get()
{
	static MemoryTracker instance;
	return instance;
}

void MemoryTracker::AddTag(size_t tagHash, uint32_t tagIndex)
{
	g_depth++;

	m_tagHashToTagIndex[tagHash] = tagIndex;
	m_memoryTagCounters.resize(tagIndex + 1);

	g_depth--;
}

void MemoryTracker::PushMemoryTagInternal(size_t tagHash)
{
	g_activeMemoryTag.push_back(m_tagHashToTagIndex.at(tagHash));
}

void MemoryTracker::PopMemoryTagInternal()
{
	g_activeMemoryTag.pop_back();
}
