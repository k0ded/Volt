#pragma once

#include <CoreUtilities/Filesystem/Path.h>

struct VkPipelineCache_T;

namespace Volt::RHI
{
	class VulkanPipelineCache
	{
	public:
		~VulkanPipelineCache();
		void Initialize(const Filesystem::Path& pipelineCacheFilepath);
		void Shutdown();

		VT_INLINE VkPipelineCache_T* GetCache() const { return m_pipelineCache; }

	private:
		VkPipelineCache_T* m_pipelineCache = nullptr;
		Filesystem::Path m_cachePath;
	};
}
