#include "Volt-ImGui/ImGuiRenderer.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Core/RenderingInfo.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

#include <RenderCore/DescriptorTableCache.h>

#include <WindowModule/Window.h>

#include <imgui.h>

namespace Volt
{
	ImGuiRenderer::ImGuiRenderer(Window* window)
		: m_window(window)
	{
		// Create sampler state
		{
			RHI::SamplerStateDesc samplerDesc{};
			samplerDesc.minFilter = RHI::TextureFilter::Linear;
			samplerDesc.magFilter = RHI::TextureFilter::Linear;
			samplerDesc.mipFilter = RHI::TextureFilter::Linear;
			samplerDesc.wrapMode = RHI::TextureWrap::Repeat;
			samplerDesc.anisotropyLevel = RHI::AnisotropyLevel::X16;
			samplerDesc.compareOperator = RHI::CompareOperator::None;
			samplerDesc.maxLod = 2.f;

			m_textureSampler = RHI::SamplerState::Create(samplerDesc);
		}

		CreatePipeline();

		// Setup imgui state
		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererUserData = this;
		io.BackendPlatformName = "Volt";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

		m_usedImages.resize(RHI::Swapchain::FramesInFlight);

		InitalizeMultiViewportSupport();

		AddViewportRenderContext(m_window);
	}

	ImGuiRenderer::~ImGuiRenderer()
	{

	}

	void ImGuiRenderer::Destroy()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererUserData = nullptr;
	}

	void ImGuiRenderer::Render(ImDrawData* drawData)
	{
		if (drawData->Textures != nullptr)
		{
			for (ImTextureData* tex : *drawData->Textures)
			{
				if (tex->Status != ImTextureStatus_OK)
				{
					UpdateTexture(tex);
				}
			}
		}

		RenderImGuiViewport(drawData, m_window);

		m_usedImages.at(m_frameIndex).clear();
		m_frameIndex = (m_frameIndex + 1) % RHI::Swapchain::FramesInFlight;
	}

	void ImGuiRenderer::RenderImGuiViewport(ImDrawData* drawData, Window* window)
	{
		RenderContext& renderContext = m_renderContexts.at(window);

		int32_t renderWidth = static_cast<int32_t>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
		int32_t renderHeight = static_cast<int32_t>(drawData->DisplaySize.y * drawData->FramebufferScale.y);

		if (renderWidth <= 0 || renderHeight <= 0)
		{
			return;
		}

		auto commandBuffer = renderContext.commandBufferSet.IncrementAndGetCommandBuffer();
		auto fence = renderContext.commandBufferSet.GetCurrentFence();

		const uint32_t index = renderContext.commandBufferSet.GetCurrentIndex();

		if (drawData->TotalVtxCount > 0)
		{
			renderContext.vertexBuffers.at(index)->Resize(drawData->TotalVtxCount * sizeof(ImDrawVert));
			renderContext.indexBuffers.at(index)->Resize(drawData->TotalIdxCount * sizeof(ImDrawIdx));

			ImDrawVert* mappedVertices = renderContext.vertexBuffers.at(index)->Map<ImDrawVert>();
			ImDrawIdx* mappedIndices = renderContext.indexBuffers.at(index)->Map<ImDrawIdx>();

			for (int32_t n = 0; n < drawData->CmdListsCount; ++n)
			{
				const ImDrawList* drawList = drawData->CmdLists[n];
				memcpy(mappedVertices, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(mappedIndices, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));

				mappedVertices += drawList->VtxBuffer.Size;
				mappedIndices += drawList->IdxBuffer.Size;
			}

			renderContext.indexBuffers.at(index)->Unmap();
			renderContext.vertexBuffers.at(index)->Unmap();
		}

		// Update globals
		{
			float scale[2];

			float* data = renderContext.globalsUniformBuffer->Map<float>();
			scale[0] = 2.0f / drawData->DisplaySize.x;
			scale[1] = 2.0f / drawData->DisplaySize.y;


			data[0] = scale[0];
			data[1] = scale[1];
			data[2] = -1.0f - drawData->DisplayPos.x * scale[0];
			data[3] = -1.0f - drawData->DisplayPos.y * scale[1];
			renderContext.globalsUniformBuffer->Unmap();
		}

		fence->WaitUntilSignaled();
		fence->Reset();

		commandBuffer->Begin();
		commandBuffer->BeginMarker("Draw ImGui", { 1.f, 1.f, 1.f, 1.f });

		auto& swapchain = window->GetSwapchain();

		{
			RHI::ResourceBarrierInfo barrier{};
			barrier.type = RHI::BarrierType::Image;
			barrier.imageBarrier().srcAccess = RHI::BarrierAccess::None;
			barrier.imageBarrier().srcStage = RHI::BarrierStage::All;
			barrier.imageBarrier().srcLayout = RHI::ImageLayout::Undefined;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::RenderTarget;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::RenderTarget;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::RenderTarget;
			barrier.imageBarrier().resource = swapchain.GetCurrentImage();

			commandBuffer->ResourceBarrier({ barrier });
		}

		const uint32_t swapchainWidth = swapchain.GetWidth();
		const uint32_t swapchainHeight = swapchain.GetHeight();

		RHI::AttachmentInfo attachment{};
		attachment.view = swapchain.GetCurrentImage()->GetView();
		attachment.clearMode = RHI::ClearMode::Clear;
		attachment.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };

		RHI::RenderingInfo renderingInfo{};
		renderingInfo.colorAttachments = { attachment };
		renderingInfo.renderArea.extent.width = swapchainWidth;
		renderingInfo.renderArea.extent.height = swapchainHeight;

		commandBuffer->BeginRendering(renderingInfo);

		RHI::Viewport viewport{};
		viewport.x = 0.f;
		viewport.y = static_cast<float>(swapchainHeight);
		viewport.width = static_cast<float>(swapchainWidth);
		viewport.height = -static_cast<float>(swapchainHeight);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		commandBuffer->SetViewports({ viewport });

		commandBuffer->BindVertexBuffers({ renderContext.vertexBuffers.at(index) }, 0);
		commandBuffer->BindIndexBuffer(renderContext.indexBuffers.at(index), sizeof(ImDrawIdx) == sizeof(uint16_t) ? RHI::IndexType::UInt16 : RHI::IndexType::UInt32);
		commandBuffer->BindPipeline(m_imguiRenderPipeline);

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
		ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		int32_t globalVertexOffset = 0;
		int32_t globalIndexOffset = 0;
		for (int32_t n = 0; n < drawData->CmdListsCount; ++n)
		{
			const ImDrawList* drawList = drawData->CmdLists[n];
			for (int32_t cmdIndex = 0; cmdIndex < drawList->CmdBuffer.Size; ++cmdIndex)
			{
				const ImDrawCmd* cmd = &drawList->CmdBuffer[cmdIndex];

				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clipMin((cmd->ClipRect.x - clip_off.x) * clip_scale.x, (cmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clipMax((cmd->ClipRect.z - clip_off.x) * clip_scale.x, (cmd->ClipRect.w - clip_off.y) * clip_scale.y);

				// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
				if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
				if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
				if (clipMax.x > renderWidth) { clipMax.x = (float)renderWidth; }
				if (clipMax.y > renderHeight) { clipMax.y = (float)renderHeight; }
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				// Apply scissor/clipping rectangle
				RHI::Rect2D scissor{};
				scissor.extent.width = (uint32_t)(clipMax.x - clipMin.x);
				scissor.extent.height = (uint32_t)(clipMax.y - clipMin.y);
				scissor.offset.x = (int32_t)(clipMin.x);
				scissor.offset.y = (int32_t)(clipMin.y);

				commandBuffer->SetScissors({ scissor });

				RHI::Image* image = (RHI::Image*)cmd->GetTexID();

				RefPtr<RHI::ImageView> imageView = image->GetView();
				RefPtr<RHI::DescriptorTable> descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(m_imguiRenderPipeline);
				descriptorTable->SetBufferView(renderContext.globalsUniformBuffer->GetView(), GetDescriptorSetIndexFromShaderStage(RHI::ShaderStage::Vertex), 0);
				descriptorTable->SetBufferView(renderContext.globalsUniformBuffer->GetView(), GetDescriptorSetIndexFromShaderStage(RHI::ShaderStage::Pixel), 0);
				descriptorTable->SetImageView(imageView, GetDescriptorSetIndexFromShaderStage(RHI::ShaderStage::Pixel), 1);
				descriptorTable->SetSamplerState(m_textureSampler, GetDescriptorSetIndexFromShaderStage(RHI::ShaderStage::Pixel), 2);

				commandBuffer->BindDescriptorTable(descriptorTable);

				commandBuffer->DrawIndexed(cmd->ElemCount, 1, cmd->IdxOffset + globalIndexOffset, cmd->VtxOffset + globalVertexOffset, 0);
			}

			globalIndexOffset += drawList->IdxBuffer.Size;
			globalVertexOffset += drawList->VtxBuffer.Size;
		}

		commandBuffer->EndRendering();
		commandBuffer->EndMarker();
		commandBuffer->End();

		RHI::CommandBufferUtils::ExecuteCommandBufferWithFence(commandBuffer, fence);
	}

	void ImGuiRenderer::CreatePipeline()
	{
		RefPtr<RHI::Shader> vertexShader;
		RefPtr<RHI::Shader> pixelShader;

		RHI::ShaderCreateInfo shaderCreateInfo{};

		{
			shaderCreateInfo.entryPoint = "MainVS";
			shaderCreateInfo.name = "ImGuiVS";
			shaderCreateInfo.sourceFilepath = "Engine/Shaders/Source/ImGui/ImGui.hlsl";
			shaderCreateInfo.stage = RHI::ShaderStage::Vertex;

			vertexShader = RHI::Shader::Create(shaderCreateInfo);
		}

		{
			shaderCreateInfo.entryPoint = "MainPS";
			shaderCreateInfo.name = "ImGuiPS";
			shaderCreateInfo.sourceFilepath = "Engine/Shaders/Source/ImGui/ImGui.hlsl";
			shaderCreateInfo.stage = RHI::ShaderStage::Pixel;

			pixelShader = RHI::Shader::Create(shaderCreateInfo);
		}

		RHI::RenderPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.shaders = { vertexShader, pixelShader };
		pipelineCreateInfo.cullMode = RHI::CullMode::None;
		pipelineCreateInfo.attachmentBlendStates[0].enabled = true;
		pipelineCreateInfo.attachmentBlendStates[0].srcColorBlend = RHI::AttachmentBlendFactor::SrcAlpha;
		pipelineCreateInfo.attachmentBlendStates[0].dstColorBlend = RHI::AttachmentBlendFactor::OneMinusSrcAlpha;
		pipelineCreateInfo.attachmentBlendStates[0].colorBlendOp = RHI::AttachmentBlendOp::Add;
		pipelineCreateInfo.attachmentBlendStates[0].srcAlphaBlend = RHI::AttachmentBlendFactor::One;
		pipelineCreateInfo.attachmentBlendStates[0].dstAlphaBlend = RHI::AttachmentBlendFactor::OneMinusSrcAlpha;
		pipelineCreateInfo.attachmentBlendStates[0].alphaBlendOp = RHI::AttachmentBlendOp::Add;

		m_imguiRenderPipeline = RHI::RenderPipeline::Create(pipelineCreateInfo);
	}

	void ImGuiRenderer::UpdateTexture(ImTextureData* textureData)
	{
		if (textureData->Status == ImTextureStatus_OK)
		{
			return;
		}

		if (textureData->Status == ImTextureStatus_WantCreate)
		{
			RHI::ImageDesc imageDesc;
			imageDesc.format = RHI::PixelFormat::R8G8B8A8_UNORM;
			imageDesc.width = textureData->Width;
			imageDesc.height = textureData->Height;
			imageDesc.mips = 1;
			imageDesc.layers = 1;
			imageDesc.initializeImage = false;

			RefPtr<RHI::Image> image = RHI::Image::Create(imageDesc);
			m_images.insert(image);

			textureData->SetTexID((ImTextureID)image.GetRaw());
		}

		if (textureData->Status == ImTextureStatus_WantCreate || textureData->Status == ImTextureStatus_WantUpdates)
		{
			// Update full texture or selected blocks. We only ever write to textures regions which have never been used before!
			// This backend choose to use tex->UpdateRect but you can use tex->Updates[] to upload individual regions.
			// We could use the smaller rect on _WantCreate but using the full rect allows us to clear the texture.
			const int32_t uploadX = (textureData->Status == ImTextureStatus_WantCreate) ? 0 : textureData->UpdateRect.x;
			const int32_t uploadY = (textureData->Status == ImTextureStatus_WantCreate) ? 0 : textureData->UpdateRect.y;
			const int32_t uploadW = (textureData->Status == ImTextureStatus_WantCreate) ? textureData->Width : textureData->UpdateRect.w;
			const int32_t uploadH = (textureData->Status == ImTextureStatus_WantCreate) ? textureData->Height : textureData->UpdateRect.h;

			size_t uploadPitch = uploadW * textureData->BytesPerPixel;
			size_t uploadSize = uploadH * uploadPitch;

			RHI::BufferDesc stagingDesc{};
			stagingDesc.count = 1;
			stagingDesc.elementSize = uploadSize;
			stagingDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			stagingDesc.usage = RHI::BufferUsage::TransferSrc;

			Handle<RHI::Allocation> stagingAlloc = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

			// Upload to buffer
			{
				uint8_t* ptr = stagingAlloc->Map<uint8_t>();
				for (int y = 0; y < uploadH; y++)
				{
					memcpy(ptr + uploadPitch * y, textureData->GetPixelsAt(uploadX, uploadY + y), (size_t)uploadPitch);
				}
				stagingAlloc->Unmap();
			}

			RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();

			RawPtr<RHI::Image> image = (RHI::Image*)textureData->GetTexID();

			commandBuffer->Begin();

			RHI::ResourceState resourceState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image);

			RHI::ResourceBarrierInfo barrierInfo{};
			barrierInfo.type = RHI::BarrierType::Image;
			barrierInfo.imageBarrier().srcAccess = resourceState.access;
			barrierInfo.imageBarrier().srcStage = resourceState.stage;
			barrierInfo.imageBarrier().srcLayout = resourceState.layout;
			barrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::CopyDest;
			barrierInfo.imageBarrier().dstStage = RHI::BarrierStage::Copy;
			barrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::CopyDest;
			barrierInfo.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrierInfo });

			commandBuffer->CopyBufferToImage(stagingAlloc, image, uploadW, uploadH, 1, uploadX, uploadY, 0);

			barrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::CopyDest;
			barrierInfo.imageBarrier().srcStage = RHI::BarrierStage::Copy;
			barrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::CopyDest;
			barrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrierInfo.imageBarrier().dstStage = RHI::BarrierStage::PixelShader;
			barrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrierInfo.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrierInfo });

			commandBuffer->End();
			RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);

			textureData->SetStatus(ImTextureStatus_OK);

			RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);
		}

		if (textureData->Status == ImTextureStatus_WantDestroy)
		{
			RHI::Image* image = (RHI::Image*)textureData->GetTexID();
			RefPtr<RHI::Image> refImage = RefPtr<RHI::Image>::Attach(image);
			m_images.erase(refImage);
		}
	}

	uint64_t ImGuiRenderer::AddTexture(RefPtr<RHI::Image> image)
	{
		VT_ENSURE(image);

		// Store a reference to the image to make sure it doesn't get destroyed until we
		// are finished using it.
		m_usedImages.at(m_frameIndex).push_back(image);
		return (ImTextureID)image.GetRaw();
	}

	static void CreateImGuiWindow(ImGuiViewport* viewport)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiRenderer* renderer = (ImGuiRenderer*)io.BackendRendererUserData;
		Window* window = (Window*)viewport->PlatformHandle;

		renderer->AddViewportRenderContext(window);
	}

	static void DestroyImGuiWindow(ImGuiViewport* viewport)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiRenderer* renderer = (ImGuiRenderer*)io.BackendRendererUserData;
		Window* window = (Window*)viewport->PlatformHandle;

		if (renderer)
		{
			renderer->RemoveViewportRenderContext(window);
		}
	}

	static void Unused_SetImGuiWindowSize(ImGuiViewport*, ImVec2)
	{}

	static void Unused_ImGuiWindowSwapBuffers(ImGuiViewport* viewport, void*)
	{}

	static void ImGuiWindowRender(ImGuiViewport* viewport, void*)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGuiRenderer* renderer = (ImGuiRenderer*)io.BackendRendererUserData;
		Window* window = (Window*)viewport->PlatformHandle;

		renderer->RenderImGuiViewport(viewport->DrawData, window);
	}

	void ImGuiRenderer::InitalizeMultiViewportSupport()
	{
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Renderer_CreateWindow = CreateImGuiWindow;
		platformIO.Renderer_DestroyWindow = DestroyImGuiWindow;
		platformIO.Renderer_SetWindowSize = Unused_SetImGuiWindowSize;
		platformIO.Renderer_RenderWindow = ImGuiWindowRender;
		platformIO.Renderer_SwapBuffers = Unused_ImGuiWindowSwapBuffers;
	}

	void ImGuiRenderer::AddViewportRenderContext(Window* window)
	{
		RenderContext& renderContext = m_renderContexts[window] = RenderContext(RHI::Swapchain::FramesInFlight);

		{
			RHI::BufferDesc desc{};
			desc.elementSize = sizeof(ImDrawVert);
			desc.count = 1;
			desc.debugName = "ImGui.VertexBuffer";
			desc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			desc.usage = RHI::BufferUsage::VertexBuffer;

			renderContext.vertexBuffers.resize(RHI::Swapchain::FramesInFlight);

			for (uint32_t i = 0; i < RHI::Swapchain::FramesInFlight; ++i)
			{
				renderContext.vertexBuffers[i] = RHI::StorageBuffer::Create(desc);
			}
		}

		{
			RHI::BufferDesc desc{};
			desc.elementSize = sizeof(ImDrawIdx);
			desc.count = 1;
			desc.debugName = "ImGui.IndexBuffer";
			desc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			desc.usage = RHI::BufferUsage::IndexBuffer;

			renderContext.indexBuffers.resize(RHI::Swapchain::FramesInFlight);

			for (uint32_t i = 0; i < RHI::Swapchain::FramesInFlight; ++i)
			{
				renderContext.indexBuffers[i] = RHI::StorageBuffer::Create(desc);
			}
		}

		renderContext.globalsUniformBuffer = RHI::UniformBuffer::Create(sizeof(glm::vec2) * 2);
	}

	void ImGuiRenderer::RemoveViewportRenderContext(Window* window)
	{
		m_renderContexts.erase(window);
	}
}
