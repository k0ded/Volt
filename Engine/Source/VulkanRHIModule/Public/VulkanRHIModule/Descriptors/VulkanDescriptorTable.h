#pragma once

#include "VulkanRHIModule/Descriptors/VulkanDescriptorCommon.h"

#include <RHIModule/Descriptors/DescriptorTable.h>
#include <CoreUtilities/Containers/Map.h>

struct VkDescriptorPool_T;
struct VkDescriptorSet_T;
struct VkPipelineLayout_T;
struct VkBufferView_T;

namespace Volt::RHI
{
	class VulkanDescriptorTable : public DescriptorTable
	{
	public:
		VulkanDescriptorTable(const DescriptorTableCreateInfo& createInfo);
		~VulkanDescriptorTable() override;

		void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, uint32_t arrayIndex /* = 0 */) override;
		size_t GetHash() const override;

		void PrepareForRender() override;

		VkPipelineLayout_T* GetRelatedPipelineLayout() const;
		uint32_t GetRelatedBindPoint() const;
		
		VT_NODISCARD VT_INLINE const vt::map<uint32_t, VkDescriptorSet_T*>& GetDescriptorSets() const { return m_descriptorSets; }

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

		DescriptorTableCreateInfo m_createInfo;
		bool m_isDirty = false;

		VkDescriptorPool_T* m_descriptorPool = nullptr;
		vt::map<uint32_t, VkDescriptorSet_T*> m_descriptorSets;

		vt::map<uint32_t, vt::map<uint32_t, uint32_t>> m_writeDescriptorsMapping; // Set -> Binding
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DescriptorImageInfo>>> m_imageDescriptorInfos; // Set -> Binding -> Array Index
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DescriptorBufferInfo>>> m_bufferDescriptorInfos; // Set -> Binding -> Array Index
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, VkBufferView>>> m_texelBufferViews;
		vt::map<uint32_t, vt::map<uint32_t, vt::map<uint32_t, DefaultInvalid>>> m_activeDescriptorWritesMapping; // Set -> Binding -> ArrayIndex 

		Vector<DescriptorWrite> m_descriptorWrites;
		Vector<DescriptorWrite> m_activeDescriptorWrites;
	};
}
