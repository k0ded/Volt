#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/ImGui/ImGuiImplementation.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferSet.h>

#include <CoreUtilities/Pointers/RefPtr.h>

struct GLFWwindow;

struct VkDescriptorPool_T;

namespace Volt::RHI
{
	class VulkanImGuiImplementation final : public ImGuiImplementation
	{
	public:
		VulkanImGuiImplementation(const ImGuiCreateInfo& createInfo);
		~VulkanImGuiImplementation() override;

		ImTextureID GetTextureID(RefPtr<Image> image, int32_t mipIndex) const override;
		ImFont* AddFont(const std::filesystem::path& fontPath) override;
		Vector<ImFont*> AddFonts(const Vector<std::filesystem::path>& fontInfos) override;

	protected:
		void BeginAPI() override;
		void EndAPI() override;

		void InitializeAPI(ImGuiContext* context) override;
		void ShutdownAPI() override;

		void* GetHandleImpl() const override;

	private:
		void InitializeVulkanData();
		void ReleaseVulkanData();
	
		RefPtr<RHI::SamplerState> m_textureSampler;
		GLFWwindow* m_windowPtr = nullptr;
		RawPtr<Swapchain> m_swapchain;
		VkDescriptorPool_T* m_descriptorPool;
		
		CommandBufferSet m_commandBufferSet;
	};
}
