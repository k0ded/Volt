#include "vrpch.h"

#include "Volt-Renderer/Debug/DebugRenderer.h"
#include "Volt-Renderer/Renderer.h"

#include <Volt-Core/Algorithms.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/DefaultBlendStates.h>

#include <RHIModule/Globals.h>

#include <CoreUtilities/Allocators/FrameStackAllocator.h>

#include "RenderView.h"

namespace Volt
{
	struct DrawDebugLinesVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DrawDebugLinesVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			RG_BUFFER_ACCESS(VertexBuffer, RGResourceAccess::VertexBuffer)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(DrawDebugLinesVS, "Engine/Shaders/Source/Debug/DrawDebugLines.hlsl", "MainVS", Vertex);

	struct DrawDebugLinesPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DrawDebugLinesPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(DrawDebugLinesPS, "Engine/Shaders/Source/Debug/DrawDebugLines.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(DrawDebugLinesParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DrawDebugLinesVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(DrawDebugLinesPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	struct DrawDebugBillboardsVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DrawDebugBillboardsVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<BillboardInstanceData>, BillboardInstances)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(DrawDebugBillboardsVS, "Engine/Shaders/Source/Debug/DrawDebugBillboards.hlsl", "MainVS", Vertex);

	struct DrawDebugBillboardsPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DrawDebugBillboardsPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(DrawDebugBillboardsPS, "Engine/Shaders/Source/Debug/DrawDebugBillboards.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(DrawDebugBillboardsParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DrawDebugBillboardsVS::Parameters, VS)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	DebugRenderer::DebugRenderer()
	{
		IniitalizeLineSphere();
	}

	void DebugRenderer::DrawLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color)
	{
		VT_PROFILE_FUNCTION();

		constexpr size_t AllocSize = sizeof(LineVertex) * 2ull;
		LineVertex* vertex = reinterpret_cast<LineVertex*>(m_lineVerticesAllocator.Allocate(AllocSize));
	
		DrawLineWithVertices(vertex, vertex + 1, v0, v1, color);
	}

	void DebugRenderer::DrawLineSphere(const glm::vec3& center, float radius, const glm::vec4& color)
	{
		VT_PROFILE_FUNCTION();

		const size_t numRequiredVertices = GetNumLinesPerLineSphere() * 2;

		LineVertex* baseVertex = ReserveLineVertices(numRequiredVertices);

		for (size_t i = 1; i < m_lineSphere.circleXZ.size(); i++)
		{
			DrawLineWithVertices(
				baseVertex++, baseVertex++,
				center + m_lineSphere.circleXZ.at(i - 1) * radius,
				center + m_lineSphere.circleXZ.at(i) * radius,
				color
			);
		}

		DrawLineWithVertices(
			baseVertex++, baseVertex++,
			center + m_lineSphere.circleXZ.back() * radius,
			center + m_lineSphere.circleXZ.front() * radius,
			color
		);

		for (size_t i = 1; i < m_lineSphere.circleXY.size(); i++)
		{
			DrawLineWithVertices(
				baseVertex++, baseVertex++,
				center + m_lineSphere.circleXY.at(i - 1) * radius,
				center + m_lineSphere.circleXY.at(i) * radius,
				color
			);
		}

		DrawLineWithVertices(
			baseVertex++, baseVertex++,
			center + m_lineSphere.circleXY.back() * radius,
			center + m_lineSphere.circleXY.front() * radius,
			color
		);

		for (size_t i = 1; i < m_lineSphere.circleYZ.size(); i++)
		{
			DrawLineWithVertices(
				baseVertex++, baseVertex++,
				center + m_lineSphere.circleYZ.at(i - 1) * radius,
				center + m_lineSphere.circleYZ.at(i) * radius,
				color
			);
		}

		DrawLineWithVertices(
			baseVertex++, baseVertex++,
			center + m_lineSphere.circleYZ.back() * radius,
			center + m_lineSphere.circleYZ.front() * radius,
			color
		);
	}

	void DebugRenderer::Render(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture)
	{
		RenderDebugLines(renderGraph, renderView, dstTexture, depthTexture);
		RenderDebugBillboards(renderGraph, renderView, dstTexture, depthTexture);
	}

	void DebugRenderer::IniitalizeLineSphere()
	{
		m_lineSphere.circleXZ.reserve(36);
		m_lineSphere.circleXY.reserve(36);
		m_lineSphere.circleYZ.reserve(36);

		for (float i = 0; i <= 360.f; i += 10.f)
		{
			const float x = glm::cos(glm::radians(i));
			const float z = glm::sin(glm::radians(i));

			m_lineSphere.circleXZ.emplace_back(x, 0.f, z);
		}

		for (float i = 0; i <= 360; i += 10.f)
		{
			const float x = glm::cos(glm::radians(i));
			const float z = glm::sin(glm::radians(i));

			m_lineSphere.circleXY.emplace_back(x, z, 0.f);
		}

		for (float i = 0; i <= 360; i += 10.f)
		{
			const float x = glm::cos(glm::radians(i));
			const float z = glm::sin(glm::radians(i));

			m_lineSphere.circleYZ.emplace_back(0.f, z, x);
		}
	}

	void DebugRenderer::RenderDebugLines(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture)
	{
		const size_t numLineVertices = m_lineVerticesAllocator.GetAllocatedSize() / sizeof(LineVertex);

		if (numLineVertices == 0)
		{
			return;
		}

		RGBufferDesc desc = RGBufferDesc::CreateStructuredBufferDesc<LineVertex>(numLineVertices, "DebugRenderer.LineVertexBuffer");
		desc.usage |= RHI::BufferUsage::VertexBuffer;
		desc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		
		RGBufferRef linesVertexBuffer = renderGraph.CreateBuffer(desc);
		AddMappedBufferUploadCopyData(renderGraph, renderGraph.CreateUAV(linesVertexBuffer), m_lineVerticesAllocator.GetData(), m_lineVerticesAllocator.GetAllocatedSize());
	
		DrawDebugLinesParameters* passParameters = renderGraph.AllocParameters<DrawDebugLinesParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.VertexBuffer = linesVertexBuffer;
		passParameters->PS.renderTargets.renderTargets[0] = dstTexture;
		passParameters->PS.renderTargets.depthTarget = depthTexture;

		auto vertexShader = ShaderMap::Get<DrawDebugLinesVS>();
		auto pixelShader = ShaderMap::Get<DrawDebugLinesPS>();

		renderGraph.AddPass("Draw Debug Lines",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, vertexShader, pixelShader, view, numLineVertices](RenderContext& context)
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::Back;
			pipelineInfo.topology = RHI::Topology::LineList;

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			context.BindPipeline(pipeline);
			context.BindVertexBuffers({ passParameters->VS.VertexBuffer }, 0);
			context.SetParameters<DrawDebugLinesVS>(vertexShader, &passParameters->VS);
			context.SetParameters<DrawDebugLinesPS>(pixelShader, &passParameters->PS);
			context.Draw(static_cast<uint32_t>(numLineVertices), 1, 0, 0);
			context.EndRendering();
		});
	}

	void DebugRenderer::Reset()
	{
		m_lineVerticesAllocator.Reset();
		m_billboardInstanceAllocator.Reset();
	}

	LineVertex* DebugRenderer::ReserveLineVertices(size_t count)
	{
		return reinterpret_cast<LineVertex*>(m_lineVerticesAllocator.Allocate(sizeof(LineVertex) * count));
	}

	void DebugRenderer::DrawLineWithVertices(LineVertex* vertex0, LineVertex* vertex1, const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color)
	{
		vertex0->position = v0;
		vertex0->color = color;

		vertex1->position = v1;
		vertex1->color = color;
	}

	void DebugRenderer::DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, uint32_t userData)
	{
		VT_PROFILE_FUNCTION();

		BillboardDrawCommand* billboardDrawCommand = reinterpret_cast<BillboardDrawCommand*>(m_billboardInstanceAllocator.Allocate(sizeof(BillboardDrawCommand)));
		billboardDrawCommand->position = position;
		billboardDrawCommand->size = size;
		billboardDrawCommand->color = color;
		billboardDrawCommand->texture = nullptr;
		billboardDrawCommand->userData = userData;
	}

	void DebugRenderer::DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, RefPtr<RHI::Image> texture, uint32_t userData)
	{
		VT_PROFILE_FUNCTION();

		BillboardDrawCommand* billboardDrawCommand = reinterpret_cast<BillboardDrawCommand*>(m_billboardInstanceAllocator.Allocate(sizeof(BillboardDrawCommand)));
		billboardDrawCommand->position = position;
		billboardDrawCommand->size = size;
		billboardDrawCommand->color = color;
		billboardDrawCommand->texture = texture.GetRaw();
		billboardDrawCommand->userData = userData;
	}

	void DebugRenderer::ReserveLines(size_t count)
	{
		m_lineVerticesAllocator.Reserve(sizeof(LineVertex) * count * 2);
	}

	void DebugRenderer::ReserveBillboards(size_t count)
	{
		m_billboardInstanceAllocator.Reserve(sizeof(BillboardDrawCommand) * count);
	}

	size_t DebugRenderer::GetNumLinesPerLineSphere() const
	{
		const size_t numLinesPerSphere =
			(m_lineSphere.circleXZ.size() - 1 +
			1 +
			m_lineSphere.circleXY.size() - 1 +
			1 +
			m_lineSphere.circleYZ.size() - 1 +
			1);
	
		return numLinesPerSphere;
	}

	void DebugRenderer::RenderDebugBillboards(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture)
	{
		auto vertexShader = ShaderMap::Get<DrawDebugBillboardsVS>();
		auto pixelShader = ShaderMap::Get<DrawDebugBillboardsPS>();

		ShaderParameterRenderTargetBindings renderTargets;
		renderTargets.renderTargets[0] = dstTexture;
		renderTargets.depthTarget = depthTexture;

		RenderBillboards(renderGraph, pixelShader, view, renderTargets, false);
	}

	RGBufferRef DebugRenderer::PrepareBillboardInstancesForRendering(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		const size_t numBillboardCommands = m_billboardInstanceAllocator.GetAllocatedSize() / sizeof(BillboardDrawCommand);
	
		if (numBillboardCommands == 0)
		{
			return nullptr;
		}

		// Copy billboard draw commands into intermediate structure to allow sorting.
		Vector<BillboardDrawCommand, FrameStackAllocator::Mark> billboardDrawCommands;
		billboardDrawCommands.resize_uninitialized(numBillboardCommands);

		memcpy_s(billboardDrawCommands.data(), billboardDrawCommands.byte_size(), m_billboardInstanceAllocator.GetData(), m_billboardInstanceAllocator.GetAllocatedSize());

		std::sort(billboardDrawCommands.begin(), billboardDrawCommands.end(), [](const BillboardDrawCommand& lhs, const BillboardDrawCommand& rhs) 
		{
			return lhs.texture < rhs.texture;
		});

		// Now we can find the instancing ranges.
		m_billboardInstancingRanges.clear();

		BillboardInstancingRange* activeRange = nullptr;
		for (size_t i = 0; i < billboardDrawCommands.size(); ++i)
		{
			if (i == 0 || activeRange->texture != billboardDrawCommands.at(i).texture)
			{
				activeRange = &m_billboardInstancingRanges.emplace_back();
				activeRange->begin = static_cast<uint32_t>(i);
				activeRange->count = 1;
				activeRange->texture = billboardDrawCommands.at(i).texture;

				if (activeRange->texture == nullptr)
				{
					activeRange->texture = Volt::Renderer::GetDefaultResources().white1x1.GetRaw();
				}
			}
			else
			{
				activeRange->count++;
			}
		}

		// Now we can copy the draw commands into instances
		const size_t billboardInstancesSize = billboardDrawCommands.size() * sizeof(BillboardInstance);

		BillboardInstance* billboardInstances = reinterpret_cast<BillboardInstance*>(renderGraph.AllocData(billboardInstancesSize));

		Algo::ForEachParalellBlocking([&billboardInstances, &billboardDrawCommands](uint32_t threadIdx, uint32_t elementIdx)
		{
			const BillboardDrawCommand& billboardDrawCommand = billboardDrawCommands.at(elementIdx);

			billboardInstances[elementIdx].position = billboardDrawCommand.position;
			billboardInstances[elementIdx].size = billboardDrawCommand.size;
			billboardInstances[elementIdx].color = billboardDrawCommand.color;
			billboardInstances[elementIdx].userData = billboardDrawCommand.userData;

		}, static_cast<uint32_t>(numBillboardCommands), 128);

		RGBufferDesc uploadDesc = RGBufferDesc::CreateStructuredBufferDesc<BillboardInstance>(numBillboardCommands, "DebugRenderer.BillboardInstances");
		uploadDesc.memoryUsage |= RHI::MemoryUsage::CPUToGPU;

		RGBufferRef billboardInstancesBuffer = renderGraph.CreateBuffer(uploadDesc);
		AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(billboardInstancesBuffer), billboardInstances, billboardInstancesSize);

		return billboardInstancesBuffer;
	}

	void DebugRenderer::RenderBillboards(RenderGraph& renderGraph, RefPtr<RHI::Shader> pixelShader, const RenderView& view, const ShaderParameterRenderTargetBindings& renderTargets, bool shouldClear)
	{
		RGBufferRef billboardInstances = PrepareBillboardInstancesForRendering(renderGraph);

		// No billboards to render.
		if (billboardInstances == nullptr)
		{
			return;
		}

		DrawDebugBillboardsParameters* passParameters = renderGraph.AllocParameters<DrawDebugBillboardsParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.BillboardInstances = renderGraph.CreateSRV(billboardInstances);
		passParameters->renderTargets = renderTargets;

		auto vertexShader = ShaderMap::Get<DrawDebugBillboardsVS>();

		renderGraph.AddPass("Draw Debug Billboards",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, vertexShader, pixelShader, view, instancingRanges = m_billboardInstancingRanges, shouldClear](RenderContext& context)
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::Back;
			pipelineInfo.topology = RHI::Topology::TriangleList;
			pipelineInfo.attachmentBlendStates[0] = Volt::DefaultBlendStates::Alpha();

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);

			if (!shouldClear)
			{
				for (RHI::AttachmentInfo& colorAttachment : renderingInfo.renderingInfo.colorAttachments)
				{
					colorAttachment.clearMode = RHI::ClearMode::Load;
				}

				renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;
			}

			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			context.BeginRendering(renderingInfo);

			RefPtr<RHI::CommandBuffer> commandBuffer = context.GetRHICommandBuffer();

			commandBuffer->BindPipeline(pipeline);

			ArrayView<RHI::ShaderParameterMap> shaderParametersMaps = pipeline->GetShaderParameterMaps();
			const RHI::ShaderResourceBinding* textureResourceBinding = pipeline->GetResourceBindingFromName("Texture"_sh, RHI::ShaderStage::Pixel);

			for (const BillboardInstancingRange& instancingRange : instancingRanges)
			{
				VT_PROFILE_SCOPE("Instancing Range");

				InlineVector<RenderContext::PerStageShaderParameters, 8> perShaderStageParameters = context.SetupPipelineData(pipeline);

				RHI::ShaderBindingMap shaderBindings = RHI::ShaderBindingMap::InitializeFromPipeline(pipeline);
				batchedShaderParameters.BindShaderBindings(shaderParametersMaps, shaderBindings);
				batchedShaderParameters.PopulateShaderParameterUniformBuffers(shaderParametersMaps, perShaderStageParameters);

				for (auto& shaderParameters : perShaderStageParameters)
				{
					shaderBindings.SetUniformBufferWithSizeAndOffset(shaderParameters.shaderStage, RHI::Globals::SHADER_GLOBALS_BINDING, shaderParameters.uniformBufferSRV->GetRHIView(), shaderParameters.size, shaderParameters.offset);
				}

				if (textureResourceBinding)
				{
					shaderBindings.SetTextureSRV(RHI::ShaderStage::Pixel, textureResourceBinding->binding, instancingRange.texture->GetView());
				}

				commandBuffer->BindShaderBindings(shaderBindings);
				commandBuffer->Draw(6, instancingRange.count, 0, instancingRange.begin);
			}

			context.EndRendering();
		});
	}
}
