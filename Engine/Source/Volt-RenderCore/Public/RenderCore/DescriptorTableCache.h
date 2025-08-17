#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	class VTRC_API ActiveDescriptorTableCache
	{
	public:
		struct ActiveDescriptorTable
		{
			RefPtr<RHI::DescriptorTable> descriptorTable;
			size_t pipelineHash;
		};

		void AddDescriptorTable(RefPtr<RHI::DescriptorTable> descriptorTable, size_t pipelineHash);
		Vector<ActiveDescriptorTable> UpdateAndGetInactiveDescriptorTables(uint64_t frameIndex);

		void FlushDescriptorTableCacheForPipeline(size_t pipelineHash);

	private:
		struct ActiveDescriptorTableContainer
		{
			ActiveDescriptorTable activeDescriptorTable;
			uint64_t lastUsedFrameIndex = 0;
		};

		std::mutex m_mutex;
		Vector<ActiveDescriptorTableContainer> m_activeDescriptorTables;
		uint64_t m_frameIndex;
	};

	class VTRC_API DescriptorTableCache
	{
	public:
		DescriptorTableCache();
		~DescriptorTableCache();

		RefPtr<RHI::DescriptorTable> GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::ComputePipeline> pipeline);
		RefPtr<RHI::DescriptorTable> GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::RenderPipeline> pipeline);

		void FlushDescriptorTableCacheForPipeline(size_t pipelineHash);

		void Update(uint64_t frameIndex);

		static DescriptorTableCache& Get() { return *s_instance; }

	private:
		struct DescriptorTableContainer
		{
			Vector<RefPtr<RHI::DescriptorTable>> descriptorTables;
			Ref<std::mutex> mutex;
		};

		inline static DescriptorTableCache* s_instance = nullptr;

		ActiveDescriptorTableCache m_activeDescriptorTableCache;
		Map<size_t, DescriptorTableContainer> m_descriptorTableCache;
	};
}
