#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	class VTRC_API DescriptorTableCache
	{
	public:
		DescriptorTableCache();
		~DescriptorTableCache();

		RefPtr<RHI::DescriptorTable> GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::ComputePipeline> pipeline);
		RefPtr<RHI::DescriptorTable> GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::RenderPipeline> pipeline);

		void Update();

		static DescriptorTableCache& Get() { return *s_instance; }

	private:
		struct DescriptorTableContainer
		{
			Vector<RefPtr<RHI::DescriptorTable>> descriptorTables;
			Ref<std::mutex> mutex;
		};

		struct ActiveDescriptorTable
		{
			RefPtr<RHI::DescriptorTable> descriptorTable;
			size_t pipelineHash;
		};

		inline static DescriptorTableCache* s_instance = nullptr;

		Vector<Vector<ActiveDescriptorTable>> m_activeDescriptorTables;
		vt::map<size_t, DescriptorTableContainer> m_descriptorTableCache;
		uint32_t m_frameIndex = 0;
	};
}
