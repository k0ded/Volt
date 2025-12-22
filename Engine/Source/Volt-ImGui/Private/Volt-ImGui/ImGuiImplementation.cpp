#include "Volt-ImGui/ImGuiImplementation.h"
#include "Volt-ImGui/ImGuiNotifications.h"
#include "Volt-ImGui/FontAwesome.h"

#include <WindowModule/Window.h>
#include <WindowModule/WindowManager.h>

#include <RenderCore/CommandBufferPool.h>
#include <RenderCore/CopyToSwapchainShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/PipelineStateCache.h>

#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Descriptors/ShaderBindingMap.h>
#include <RHIModule/Core/RenderingInfo.h>
#include <RHIModule/Globals.h>

#include <LogModule/Log.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <imgui.h>

namespace Volt
{
	std::filesystem::path GetOrCreateIniPath()
	{
		const std::filesystem::path userIniPath = "User/imgui.ini";
		const std::filesystem::path defaultIniPath = "Editor/imgui.ini";

		if (!std::filesystem::exists(userIniPath))
		{
			VT_LOG(Warning, "User ini file not found! Copying default!");

			std::filesystem::create_directories(userIniPath.parent_path());
			if (!std::filesystem::exists(defaultIniPath))
			{
				VT_LOG(Error, "Unable to find default ini file!");
				return "imgui.ini";
			}
			std::filesystem::copy(defaultIniPath, userIniPath.parent_path());
		}

		return userIniPath;
	}

	inline void MergeIconsWithLatestFont()
	{
		ImGuiIO& io = ImGui::GetIO();

		//float baseFontSize = font_size; // 13.0f is the size of the default font. Change to the font size you use.
		//float iconFontSize = baseFontSize; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

		// merge in icons from Font Awesome
		static const ImWchar icons_ranges[] = { VT_ICON_MIN_FA, VT_ICON_MAX_16_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		//icons_config.GlyphMinAdvanceX = iconFontSize;
		io.Fonts->AddFontFromFileTTF("Engine/Fonts/FontAwesome/" FONT_ICON_FILE_NAME_FAS, 0.0f, &icons_config, icons_ranges);
	}

	ImGuiImplementation::ImGuiImplementation(const ImGuiCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Initialize();
	}

	ImGuiImplementation::~ImGuiImplementation()
	{
		const std::filesystem::path iniPath = GetOrCreateIniPath();
		ImGui::SaveIniSettingsToDisk(iniPath.string().c_str());

		// Shared font altas will be destroyed here.
		for (auto& contextData : m_contextStack)
		{
			contextData.platform->Destroy();
			contextData.renderer->Destroy();
			ImGui::DestroyContext(contextData.context);
		}

		m_sharedFontAtlas = nullptr;
		m_contextStack.clear();
	}

	void ImGuiImplementation::Begin()
	{
		VT_PROFILE_FUNCTION();

		GetActivePlatform()->BeginFrame();
		ImGui::NewFrame();

		if (m_defaultFont)
		{
			ImGui::PushFont(m_defaultFont, 16.f);
		}
	}
	
	void ImGuiImplementation::End()
	{
		VT_PROFILE_FUNCTION();

		ImGuiNotifications::RenderNotifications();

		if (m_defaultFont)
		{
			ImGui::PopFont();
		}

		ImGui::Render();

		ImDrawData* drawData = ImGui::GetDrawData();

		GetActiveRenderer()->Render(drawData, m_createInfo.window, m_contextStack.size() > 1);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Composite all windows render targets to their respective swapchains.
		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		// Update globals
		{
			float* data = m_copyGlobalsUniformBuffer->Map<float>();
			*data = WindowManager::Get().GetPeakNits();
			m_copyGlobalsUniformBuffer->Unmap();
		}

		commandBuffer->Begin();

		const Map<Window*, ImGuiRenderTargetManager::RenderTarget>& renderTargets = m_renderTargetManager->GetAllRenderTargets();
		for (const auto& [window, renderTarget] : renderTargets)
		{
			auto& swapchain = window->GetSwapchain();

			{
				RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
				barrier.imageBarrier().srcAccess = RHI::BarrierAccess::None;
				barrier.imageBarrier().srcStage = RHI::BarrierStage::All;
				barrier.imageBarrier().srcLayout = RHI::ImageLayout::Undefined;
				barrier.imageBarrier().dstAccess = RHI::BarrierAccess::RenderTarget;
				barrier.imageBarrier().dstStage = RHI::BarrierStage::RenderTarget;
				barrier.imageBarrier().dstLayout = RHI::ImageLayout::RenderTarget;
				barrier.imageBarrier().resource = swapchain.GetCurrentImage();

				commandBuffer->ResourceBarrier({ barrier });
			}

			{
				RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
				barrier.imageBarrier().srcAccess = RHI::BarrierAccess::RenderTarget;
				barrier.imageBarrier().srcStage = RHI::BarrierStage::RenderTarget;
				barrier.imageBarrier().srcLayout = RHI::ImageLayout::RenderTarget;
				barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				barrier.imageBarrier().dstStage = RHI::BarrierStage::PixelShader;
				barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
				barrier.imageBarrier().resource = renderTarget.image;

				commandBuffer->ResourceBarrier({ barrier });
			}

			// Copy the render target to the swapchain
			RHI::AttachmentInfo attachment{};
			attachment.view = swapchain.GetCurrentImage()->GetView();
			attachment.clearMode = RHI::ClearMode::Clear;
			attachment.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };
			
			const uint32_t renderTargetWidth = swapchain.GetWidth();
			const uint32_t renderTargetHeight = swapchain.GetHeight();

			RHI::RenderingInfo renderingInfo{};
			renderingInfo.colorAttachments = { attachment };
			renderingInfo.renderArea.extent.width = renderTargetWidth;
			renderingInfo.renderArea.extent.height = renderTargetHeight;
		
			commandBuffer->BeginRendering(renderingInfo);

			RHI::Viewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(renderTargetHeight);
			viewport.width = static_cast<float>(renderTargetWidth);
			viewport.height = -static_cast<float>(renderTargetHeight);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			RHI::Rect2D scissor{};
			scissor.extent.width = renderTargetWidth;
			scissor.extent.height = renderTargetHeight;
			scissor.offset.x = 0;
			scissor.offset.y = 0;

			RefPtr<RHI::Shader> vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
			RefPtr<RHI::Shader> pixelShader;

			if (swapchain.IsHDREnabled())
			{
				pixelShader = ShaderMap::Get<CopyToSwapchain_HDR>();
			}
			else
			{
				pixelShader = ShaderMap::Get<CopyToSwapchain_SDR>();
			}

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto copyPipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			commandBuffer->SetViewports({ viewport });
			commandBuffer->BindPipeline(copyPipeline);
			commandBuffer->SetScissors({ scissor });

			RHI::ShaderBindingMap shaderBindingMap = RHI::ShaderBindingMap::InitializeFromPipeline(copyPipeline);

			const RHI::ShaderResourceBinding* srcTextureBinding = pixelShader->GetParameterMap().GetResourceBindingFromName("SrcColor"_sh);
			if (srcTextureBinding)
			{
				shaderBindingMap.SetTextureSRV(RHI::ShaderStage::Pixel, srcTextureBinding->binding, renderTarget.image->GetView());
			}

			if (swapchain.IsHDREnabled())
			{
				shaderBindingMap.SetUniformBufferWithSizeAndOffset(RHI::ShaderStage::Pixel, RHI::Globals::SHADER_GLOBALS_BINDING, m_copyGlobalsUniformBuffer->GetView(), m_copyGlobalsUniformBuffer->GetSize(), 0);
			}

			commandBuffer->BindShaderBindings(shaderBindingMap);
			commandBuffer->Draw(3, 1, 0, 0);
			commandBuffer->EndRendering();

			{
				RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
				barrier.imageBarrier().srcAccess = RHI::BarrierAccess::ShaderRead;
				barrier.imageBarrier().srcStage = RHI::BarrierStage::PixelShader;
				barrier.imageBarrier().srcLayout = RHI::ImageLayout::ShaderRead;
				barrier.imageBarrier().dstAccess = RHI::BarrierAccess::RenderTarget;
				barrier.imageBarrier().dstStage = RHI::BarrierStage::RenderTarget;
				barrier.imageBarrier().dstLayout = RHI::ImageLayout::RenderTarget;
				barrier.imageBarrier().resource = renderTarget.image;

				commandBuffer->ResourceBarrier({ barrier });
			}
		}

		commandBuffer->End();

		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
	}
	
	void ImGuiImplementation::RenderPreviousFrameContextStack()
	{
		for (size_t i = 0; i < m_contextStack.size() - 1; ++i)
		{
			ContextData& contextData = m_contextStack.at(i);
			contextData.renderer->RenderPreviousFrame(m_createInfo.window);
		}
	}

	void ImGuiImplementation::PushNewContext()
	{
		m_contextStack.emplace_back() = CreateAndInitializeNewContext();
		ImGui::SetCurrentContext(m_contextStack.back().context);
	}

	void ImGuiImplementation::PopContext()
	{
		VT_ENSURE(m_contextStack.size() > 1);

		ContextData lastContext = m_contextStack.back();
		m_contextStack.pop_back();

		lastContext.platform->Destroy();
		lastContext.renderer->Destroy();

		ImGui::DestroyContext(lastContext.context);
		ImGui::SetCurrentContext(m_contextStack.back().context);
	}

	void ImGuiImplementation::SetDefaultFont(ImFont* font)
	{
		m_defaultFont = font;
	}

	ImFont* ImGuiImplementation::AddFont(const std::filesystem::path& fontPath)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImFont* newFont = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str());
		MergeIconsWithLatestFont();

		return newFont;
	}

	Vector<ImFont*> ImGuiImplementation::AddFonts(const Vector<std::filesystem::path>& fontPaths)
	{
		ImGuiIO& io = ImGui::GetIO();

		Vector<ImFont*> resultFonts;

		for (const auto& fontPath : fontPaths)
		{
			resultFonts.emplace_back() = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str());
			MergeIconsWithLatestFont();
		}

		return resultFonts;
	}

	ImTextureID ImGuiImplementation::GetTextureID(RefPtr<RHI::Image> image, int32_t mipIndex)
	{
		return GetActiveRenderer()->AddTexture(image);
	}

	void ImGuiImplementation::Initialize()
	{
		IMGUI_CHECKVERSION();
		
		CreateCopyGlobalsUniformBuffer();

		m_renderTargetManager = CreateScope<ImGuiRenderTargetManager>();

		// Create and add the default context
		m_contextStack.emplace_back() = CreateAndInitializeNewContext();

		const std::filesystem::path iniPath = GetOrCreateIniPath();
		ImGui::LoadIniSettingsFromDisk(iniPath.string().c_str());
	}

	void ImGuiImplementation::CreateCopyGlobalsUniformBuffer()
	{
		RHI::UniformBufferDesc desc{};
		desc.size = sizeof(float);
		desc.debugName = "ImGuiImplementation.GlobalsBuffer";
		m_copyGlobalsUniformBuffer = RHI::UniformBuffer::Create(desc);
	}

	ImGuiImplementation::ContextData ImGuiImplementation::CreateAndInitializeNewContext()
	{
		ImGuiContext* context = ImGui::CreateContext(m_sharedFontAtlas);
		if (!m_sharedFontAtlas)
		{
			m_sharedFontAtlas = ImGui::GetIO().Fonts;
		}

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		if (m_createInfo.enableViewports)
		{
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}

		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.IniFilename = nullptr;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.f;
		}

		style.Colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.23f, 0.23f, 0.23f, 1.000f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);

		style.Colors[ImGuiCol_Border] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);

		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);

		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);

		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);

		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_Button] = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);


		style.Colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);

		style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_Tab] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.258f, 0.258f, 0.258f, 1.000f);

		style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.4f, 0.67f, 1.000f, 0.781f);
		style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);

		style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);

		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);

		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);

		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4f, 0.67f, 1.000f, 1.000f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style.ChildRounding = 0;
		style.FrameRounding = 0;
		style.GrabMinSize = 7.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 12.0f;
		style.ScrollbarSize = 13.0f;
		style.TabBorderSize = 0.0f;
		style.TabRounding = 0.0f;
		style.WindowRounding = 0.0f;
		style.WindowBorderSize = 2.f;

		ImGui::SetCurrentContext(context);

		ContextData result;
		result.context = context;
		result.platform = CreateRef<ImGuiPlatform>();
		result.renderer = CreateRef<ImGuiRenderer>(m_renderTargetManager.get());

		return result;
	}
}
