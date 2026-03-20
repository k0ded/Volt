#include "vrpch.h"
#include "Volt-Renderer/UISceneRenderer.h"

#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Utility/StagedBufferUpload.h"

#include "Volt-Renderer/Texture/Texture2D.h"

//#include "Volt/GameUI/UIScene.h"
//#include "Volt/GameUI/UIComponents.h"

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/Shader/ShaderMap.h>
#if 0
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#endif
namespace Volt
{
#if 0
	struct UI2DGridVSPS
	{
		BEGIN_SHADER_DEFINITION(UI2DGridVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/2DGrid_ps.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(float4x4, InverseViewProjection)
			SHADER_PARAMETER(float, Scale)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(UI2DGridVSPS)

	struct UIWidgetIDVSPS
	{
		BEGIN_SHADER_DEFINITION(UIWidgetIDVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/UIWidgetID.hlsl", "MainVS", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/UIWidgetID.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(float4x4, ViewProjection)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(UIWidgetIDVSPS)

	struct UIMainVSPS
	{
		BEGIN_SHADER_DEFINITION(UIMainVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/UI/UIMain_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/UI/UIMain_ps.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(float4x4, ViewProjection)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(UIMainVSPS)

	struct UIRenderingData
	{
		RenderGraphBufferHandle vertexBuffer;
		RenderGraphBufferHandle indexBuffer;

		uint32_t indexCount = 0;
	};
#endif

	struct UIVertex
	{
		glm::vec4 position;
		glm::vec2 texCoords;
		glm::vec4 color;
		ResourceHandle imageHandle = Resource::Invalid;
		uint32_t id;
	};

	static constexpr uint32_t s_quadVertexCount = 4;
	static constexpr uint32_t s_quadIndexCount = 6;
	static constexpr glm::vec4 s_quadVertexPositions[s_quadVertexCount] =
	{
		{ 0.f,  0.f, 0.f, 1.f },
		{ 1.f,  0.f, 0.f, 1.f },
		{ 0.f, -1.f, 0.f, 1.f },
		{ 1.f, -1.f, 0.f, 1.f }
	};

	static constexpr glm::vec2 s_quadVertexTexCoords[s_quadVertexCount] =
	{
		{ 0.f, 0.f },
		{ 1.f, 0.f },
		{ 0.f, 1.f },
		{ 1.f, 1.f }
	};

	static constexpr uint32_t s_quadVertexIndices[s_quadIndexCount] =
	{
		0, 1, 2,
		2, 1, 3
	};

	UISceneRenderer::UISceneRenderer(const UISceneRendererSpecification& specification)
		: m_scene(specification.scene), m_isEditor(specification.isEditor), m_commandBufferSet(Renderer::GetFramesInFlight())
	{

	}

	UISceneRenderer::~UISceneRenderer()
	{
	}

	void UISceneRenderer::OnRender(IntRef<RHI::Image> targetImage, const glm::mat4& projectionMatrix)
	{
#if 0
		RenderGraphBlackboard blackboard;
		RenderGraph renderGraph{ m_commandBufferSet.IncrementAndGetCommandBuffer() };
	
		if (PrepareForRender(renderGraph, blackboard))
		{
			const auto& renderingData = blackboard.Get<UIRenderingData>();

			RenderGraphImageHandle targetImageHandle = renderGraph.AddExternalImage(targetImage);

			struct UIPassData
			{
				RenderGraphImageHandle depthImage;
			};

			struct UISelectionData
			{
				RenderGraphImageHandle widgetIDImage;
				RenderGraphImageHandle depthImage;
			};

			renderGraph.AddPass("Grid Pass", 
			[&](RenderGraph::Builder& builder) 
			{
				builder.WriteResource(targetImageHandle);
				builder.SetHasSideEffect();
			}, 
			[=](RenderContext& context) 
			{
				RenderingInfo renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), { targetImageHandle });

				RHI::RenderPipelineCreateInfo pipelineInfo{};
				pipelineInfo.shader = ShaderMap::Get<UI2DGridVSPS>();

				auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

				UI2DGridVSPS::Parameters parameters;
				parameters.Scale = 1.f;
				parameters.InverseViewProjection = glm::inverse(projectionMatrix);

				context.BeginRendering(renderingInfo);

				RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
				{
					context.SetParameters<UI2DGridVSPS>(parameters);
				});

				context.EndRendering();
			});

			UISelectionData& selectionData = renderGraph.AddPass<UISelectionData>("UI Selection Pass",
			[&](RenderGraph::Builder& builder, UISelectionData& data)
			{
				{
					const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(targetImage->GetWidth(), targetImage->GetHeight(), RHI::ImageUsage::AttachmentStorage, "UI Selection");
					data.widgetIDImage = builder.CreateImage(desc);
				}

				{
					const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::D32_SFLOAT>(targetImage->GetWidth(), targetImage->GetHeight(), RHI::ImageUsage::Attachment, "UI Selection Depth");
					data.depthImage = builder.CreateImage(desc);
				}

				builder.ReadResource(renderingData.indexBuffer, RenderGraphResourceState::IndexBuffer);
				builder.ReadResource(renderingData.vertexBuffer, RenderGraphResourceState::VertexBuffer);

				builder.SetHasSideEffect();
			},
			[=](const UISelectionData& data, RenderContext& context) 
			{
				RenderingInfo renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), { data.widgetIDImage, data.depthImage });
				
				RHI::RenderPipelineCreateInfo pipelineInfo{};
				pipelineInfo.shader = ShaderMap::Get<UIWidgetIDVSPS>();
				
				auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

				context.BeginRendering(renderingInfo);
				context.BindPipeline(pipeline);

				context.BindIndexBuffer(renderingData.indexBuffer);
				context.BindVertexBuffers({ renderingData.vertexBuffer }, 0);

				UIWidgetIDVSPS::Parameters parameters;
				parameters.ViewProjection = projectionMatrix;

				context.SetParameters<UIWidgetIDVSPS>(parameters);
				context.DrawIndexed(renderingData.indexCount, 1, 0, 0, 0);
				context.EndRendering();
			});

			renderGraph.EnqueueImageExtraction(selectionData.widgetIDImage, m_widgetIDImage);

			renderGraph.AddPass<UIPassData>("UI Pass",
			[&](RenderGraph::Builder& builder, UIPassData& data) 
			{
				{
					const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::D32_SFLOAT>(targetImage->GetWidth(), targetImage->GetHeight(), RHI::ImageUsage::Attachment, "UI Depth");
					data.depthImage = builder.CreateImage(desc);
				}

				builder.WriteResource(targetImageHandle);
				builder.ReadResource(renderingData.indexBuffer, RenderGraphResourceState::IndexBuffer);
				builder.ReadResource(renderingData.vertexBuffer, RenderGraphResourceState::VertexBuffer);

				builder.SetHasSideEffect();
			},
			[=](const UIPassData& data, RenderContext& context)
			{
				RenderingInfo renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), { targetImageHandle, data.depthImage });
				renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;

				RHI::RenderPipelineCreateInfo pipelineInfo{};
				pipelineInfo.shader = ShaderMap::Get<UIMainVSPS>();

				UIMainVSPS::Parameters parameters;
				parameters.ViewProjection = projectionMatrix;
				parameters.LinearSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle();

				auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

				context.BeginRendering(renderingInfo);
				context.BindPipeline(pipeline);

				context.BindIndexBuffer(renderingData.indexBuffer);
				context.BindVertexBuffers({ renderingData.vertexBuffer }, 0);

				context.SetParameters<UIMainVSPS>(parameters);
				context.DrawIndexed(renderingData.indexCount, 1, 0, 0, 0);
				context.EndRendering();
			});
		}

		RenderGraphResourceHandle outputImage = renderGraph.AddExternalImage(targetImage);
		{
			RenderGraphBarrierInfo barrier{};
			barrier.dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.dstStage = RHI::BarrierStage::PixelShader;

			renderGraph.AddResourceBarrier(outputImage, barrier);
		}

		renderGraph.Compile();
		renderGraph.Execute();
#endif
	}

	bool UISceneRenderer::PrepareForRender(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		VT_PROFILE_FUNCTION();

		// We are going to dynamically size the index and vertex buffers.
		// To do this we first need to calculate the maximum index and vertex count.
		// For now we will only do this for the images, but in the future we need
		// to add this to other types

#if 0
		VertexIndexCounts vertexIndexCounts = CalculateMaxVertexAndIndexCount();

		auto& renderingData = blackboard.Add<UIRenderingData>();

		if (vertexIndexCounts.indexCount == 0 || vertexIndexCounts.vertexCount == 0)
		{
			return false;
		}

		renderingData.indexCount = vertexIndexCounts.indexCount;

		{
			const auto desc = RGUtils::CreateBufferDesc<UIVertex>(vertexIndexCounts.vertexCount, RHI::BufferUsage::VertexBuffer, RHI::MemoryUsage::GPU, "UI Vertex Buffer");
			renderingData.vertexBuffer = renderGraph.CreateBuffer(desc);
		}
	
		{
			const auto desc = RGUtils::CreateBufferDesc<uint32_t>(vertexIndexCounts.indexCount, RHI::BufferUsage::IndexBuffer, RHI::MemoryUsage::GPU, "UI Index Buffer");
			renderingData.indexBuffer = renderGraph.CreateBuffer(desc);
		}
#endif

		// All images
		{
			//StagedBufferUpload<UIVertex> stagedVertexBufferUpload{ vertexIndexCounts.vertexCount };
			//StagedBufferUpload<uint32_t> stagedIndexBufferUpload{ vertexIndexCounts.indexCount };
			//m_scene->ForEachWithComponents<const UITransformComponent, const UIImageComponent, const UIIDComponent>([&](entt::entity handle, const UITransformComponent& transComp, const UIImageComponent& imageComp, const UIIDComponent& idComp)
			//{
			//	auto texture = AssetManager::GetAsset<Texture2D>(imageComp.imageHandle);
			//	if (!texture || !texture->IsValid())
			//	{
			//		texture = Renderer::GetDefaultResources().whiteTexture;
			//	}
			//
			//	const ResourceHandle resourceHandle = texture->GetResourceHandle();
			//
			//	for (uint32_t i = 0; i < s_quadVertexCount; i++)
			//	{
			//		UIVertex& vertex = stagedVertexBufferUpload.AddUploadItem();
			//		vertex.position = (s_quadVertexPositions[i] - glm::vec4{ transComp.alignment, 0.f, 0.f }) * glm::vec4{ transComp.size, 1.f, 1.f } + glm::vec4{ transComp.position.x, -transComp.position.y, 10.f, 0.f };
			//		vertex.texCoords = s_quadVertexTexCoords[i];
			//		vertex.color = glm::vec4{ imageComp.tint, imageComp.alpha };
			//		vertex.imageHandle = resourceHandle;
			//		vertex.id = idComp.id;
			//	}
			//
			//	for (uint32_t i = 0; i < s_quadIndexCount; i++)
			//	{
			//		stagedIndexBufferUpload.AddUploadItem() = s_quadVertexIndices[i];
			//	}
			//});
			//
			//stagedVertexBufferUpload.UploadTo(renderGraph, renderingData.vertexBuffer);
			//stagedIndexBufferUpload.UploadTo(renderGraph, renderingData.indexBuffer);
		}

		return true;
	}

	UISceneRenderer::VertexIndexCounts UISceneRenderer::CalculateMaxVertexAndIndexCount()
	{
		VT_PROFILE_FUNCTION();

		VertexIndexCounts result{};

		// Images
		//{
		//	m_scene->ForEachWithComponents<const UITransformComponent, const UIImageComponent>([&](entt::entity handle, const UITransformComponent& transComp, const UIImageComponent& uiImageComp) 
		//	{
		//		result.vertexCount += s_quadVertexCount;
		//		result.indexCount += s_quadIndexCount;
		//	});
		//}

		return result;
	}
}
