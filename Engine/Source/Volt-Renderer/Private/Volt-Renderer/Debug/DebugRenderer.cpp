#include "vrpch.h"

#include "Volt-Renderer/Debug/DebugRenderer.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/ShaderMap.h>

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
	REGISTER_SHADER(DrawDebugLinesVS, "Engine/Shaders/Source/Debug/DrawDebugLines.hlsl", "MainVS", Vertex);

	struct DrawDebugLinesPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DrawDebugLinesPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DrawDebugLinesPS, "Engine/Shaders/Source/Debug/DrawDebugLines.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(DrawDebugLinesParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DrawDebugLinesVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(DrawDebugLinesPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	DebugRenderer::DebugRenderer()
	{
		constexpr size_t NumMaxLines = 1'000'000;

		m_lineVerticesAllocator.Reserve(sizeof(LineVertex) * NumMaxLines);

		IniitalizeLineSphere();
	}

	void DebugRenderer::DrawLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color)
	{
		constexpr size_t AllocSize = sizeof(LineVertex) * 2ull;
		LineVertex* vertex = reinterpret_cast<LineVertex*>(m_lineVerticesAllocator.Allocate(AllocSize));
	
		vertex->position = v0;
		vertex->color = color;
		vertex++;

		vertex->position = v1;
		vertex->color = color;
	}

	void DebugRenderer::DrawLineSphere(const glm::vec3& center, float radius, const glm::vec4& color)
	{
		for (size_t i = 1; i < m_lineSphere.circleXZ.size(); i++)
		{
			DrawLine(
				center + m_lineSphere.circleXZ.at(i - 1) * radius,
				center + m_lineSphere.circleXZ.at(i) * radius,
				color
			);
		}

		DrawLine(
			center + m_lineSphere.circleXZ.back() * radius,
			center + m_lineSphere.circleXZ.front() * radius,
			color
		);

		for (size_t i = 1; i < m_lineSphere.circleXY.size(); i++)
		{
			DrawLine(
				center + m_lineSphere.circleXY.at(i - 1) * radius,
				center + m_lineSphere.circleXY.at(i) * radius,
				color
			);
		}

		DrawLine(
			center + m_lineSphere.circleXY.back() * radius,
			center + m_lineSphere.circleXY.front() * radius,
			color
		);

		for (size_t i = 1; i < m_lineSphere.circleYZ.size(); i++)
		{
			DrawLine(
				center + m_lineSphere.circleYZ.at(i - 1) * radius,
				center + m_lineSphere.circleYZ.at(i) * radius,
				color
			);
		}

		DrawLine(
			center + m_lineSphere.circleYZ.back() * radius,
			center + m_lineSphere.circleYZ.front() * radius,
			color
		);
	}

	void DebugRenderer::Render(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture)
	{
		RenderDebugLines(renderGraph, renderView, dstTexture, depthTexture);
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
		AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(linesVertexBuffer), m_lineVerticesAllocator.GetData(), m_lineVerticesAllocator.GetAllocatedSize());
	
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
	}
}
