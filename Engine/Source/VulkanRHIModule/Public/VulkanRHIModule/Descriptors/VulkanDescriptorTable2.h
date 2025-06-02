#pragma once

#include "VulkanRHIModule/Descriptors/VulkanDescriptorCommon.h"

#include <RHIModule/Descriptors/DescriptorTable.h>
#include <CoreUtilities/Containers/Map.h>

struct VkDescriptorPool_T;
struct VkDescriptorSet_T;

namespace Volt::RHI
{
	class VulkanDescriptorTable2 : public DescriptorTable
	{
	public:
		VulkanDescriptorTable2(const DescriptorTableCreateInfo& createInfo);
		~VulkanDescriptorTable2() override;

		void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, uint32_t arrayIndex /* = 0 */) override;

		void SetImageView(std::string_view name, RawPtr<ImageView> view, uint32_t arrayIndex = 0) override;
		void SetBufferView(std::string_view name, RawPtr<BufferView> view, uint32_t arrayIndex = 0) override;
		void SetSamplerState(std::string_view name, RawPtr<SamplerState> samplerState, uint32_t arrayIndex = 0) override;

		void PrepareForRender() override;
		void Bind(CommandBuffer& commandBuffer) override;

	protected:
		void Invalidate();
		void Release();

		void CreateFromComputePipeline();
		void CreateFromRenderPipeline();

		void* GetHandleImpl() const override;

	private:
		struct DefaultInvalid
		{
			inline static constexpr uint32_t INVALID_VALUE = std::numeric_limits<uint32_t>::max();
			uint32_t value = INVALID_VALUE;
		};

		void BuildWriteDescriptors();
		void InitializeWriteDescriptor(DescriptorWrite& writeDescriptor, const uint32_t binding, const uint32_t descriptorType, VkDescriptorSet_T* dstDescriptorSet);
		void InitializeInfoStructs();

		DescriptorTableCreateInfo m_createInfo;
		bool m_isDirty = false;

		VkDescriptorPool_T* m_descriptorPool = nullptr;
		Vector<VkDescriptorSet_T*> m_descriptorSets;

		vt::map<uint32_t, vt::map<uint32_t, uint32_t>> m_writeDescriptorsMapping; // Set -> Binding
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DescriptorImageInfo>>> m_imageDescriptorInfos; // Set -> Binding -> Array Index
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DescriptorBufferInfo>>> m_bufferDescriptorInfos; // Set -> Binding -> Array Index
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DefaultInvalid>>> m_activeDescriptorWritesMapping; // Set -> Binding -> ArrayIndex 

		Vector<DescriptorWrite> m_descriptorWrites;
		Vector<DescriptorWrite> m_activeDescriptorWrites;
	};
}
