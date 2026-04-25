#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/LastSubmissionTracker.h"

#include <RHIModule/Images/SamplerState.h>

#include <vulkan/vulkan.h>

struct VkSampler_T;

namespace Volt::RHI
{
	class VulkanSamplerState final : public SamplerState, public LastSubmissionTracker
	{
	public:
		struct DescriptorDescription
		{
			VkDescriptorGetInfoEXT vkDescriptorInfo;
			uint64_t descriptorSize;
		};

		VulkanSamplerState(const SamplerStateDesc& createInfo);
		~VulkanSamplerState() override;

		BindlessIndex GetBindlessIndex() const override;

		VT_NODISCARD VT_INLINE const DescriptorDescription& GetDescriptor() const { return m_descriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		VkSampler_T* m_sampler = nullptr;
		DescriptorDescription m_descriptor;

		BindlessIndex m_bindlessIndex;
	};
}
