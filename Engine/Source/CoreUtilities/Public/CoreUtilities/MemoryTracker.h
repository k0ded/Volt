#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/String/StringView.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Containers/Map.h"
#include "CoreUtilities/String/StringHash.h"

class MemoryTagBase
{
};

class MemoryTagRegistry
{
public:
	template<typename T> void RegisterMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>);
	template<typename T> void UnregisterMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>);

	static MemoryTagRegistry& Get();

private:
	friend class MemoryTracker;

	struct MemoryTagInfo
	{
		StringView name;
		uint32_t tagIndex;
	};

	Map<size_t, MemoryTagInfo> m_memoryTagInfos;
	uint32_t m_nextTagIndex = 0;
};

class MemoryTracker
{
public:
	VTCOREUTIL_API static void Initialize();

	template<typename T> static void PushMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>);
	static void PopMemoryTag();

	static void OnAllocate(MemoryTrackerHeader* header);
	static void OnFree(MemoryTrackerHeader* header);
	static void OnReallocate(MemoryTrackerHeader* header, uint64_t oldSize);

	static MemoryTracker& Get();

public:
	// Since the counters never will be moved or copied,
	// it's fine that those constructors doesn't do anything.
	struct alignas(std::hardware_destructive_interference_size) AtomicContainer
	{
		AtomicContainer() = default;
		AtomicContainer(const AtomicContainer&) noexcept {}
		AtomicContainer(AtomicContainer&&) noexcept {}

		std::atomic<uint64_t> counter;
	};

	void PushMemoryTagInternal(size_t tagHash);
	void PopMemoryTagInternal();

	inline static bool s_isInitialized = false;

	Map<size_t, uint32_t> m_tagHashToTagIndex;
	Vector<AtomicContainer> m_memoryTagCounters;
};

template<typename T>
inline void MemoryTagRegistry::RegisterMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>)
{
	constexpr StringView name = T::Name;
	constexpr StringHash hash = T::Hash;

	MemoryTagRegistry& instance = Get();

	VT_ASSERT(!instance.m_memoryTagInfos.contains(hash.hash));

	MemoryTagInfo& tagInfo = instance.m_memoryTagInfos[hash.hash];
	tagInfo.name = name;
	tagInfo.tagIndex = instance.m_nextTagIndex++;
}

template<typename T>
inline void MemoryTagRegistry::UnregisterMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>)
{
	constexpr StringHash hash = T::Hash;

	MemoryTagRegistry& instance = Get();

	VT_ASSERT(!instance.m_memoryTagInfos.contains(hash.hash));
	instance.m_memoryTagInfos.erase(hash.hash);
}

template<typename T>
inline void MemoryTracker::PushMemoryTag() requires(std::is_base_of_v<MemoryTagBase, T>)
{
	constexpr StringHash hash = T::Hash;
	if (s_isInitialized)
	{
		Get().PushMemoryTagInternal(hash.hash);
	}
}

#define VT_DECLARE_MEMORY_TAG(tagName) \
	namespace MemoryTag \
	{ \
		class tagName : public MemoryTagBase \
		{ \
		public: \
			inline static constexpr StringView Name = #tagName; \
			inline static constexpr StringHash Hash = StringHash::Construct(Name); \
		}; \
	} \

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_MEMORY_TAG(tagName) \
	class MemoryTagRegistrar_##tagName \
	{ \
	public: \
		VT_INLINE MemoryTagRegistrar_##tagName() \
		{ \
			MemoryTagRegistry::Get().RegisterMemoryTag<MemoryTag::tagName>(); \
		} \
		VT_INLINE ~MemoryTagRegistrar_##tagName() \
		{ \
			MemoryTagRegistry::Get().UnregisterMemoryTag<MemoryTag::tagName>(); \
		} \
	} g_memoryTagRegistrar_##tagName \

VT_DECLARE_MEMORY_TAG(Unknown)
