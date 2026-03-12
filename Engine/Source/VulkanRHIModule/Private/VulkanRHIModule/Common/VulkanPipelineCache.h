#pragma once

struct VkPipelineCache_T;

namespace Volt::RHI
{
	class VulkanPipelineCache
	{
	public:
		~VulkanPipelineCache();
		void Initialize(const std::filesystem::path& pipelineCacheFilepath);
		void Shutdown();

		VT_INLINE VkPipelineCache_T* GetCache() const { return m_pipelineCache; }

	private:
		VkPipelineCache_T* m_pipelineCache = nullptr;
		std::filesystem::path m_cachePath;
	};
}
