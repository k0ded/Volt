#include "circuitpch.h"

#include "Rendering/CircuitRenderer.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/ShaderTypes.h>

#include <RenderCore/RenderGraph/Resources/RenderGraphBuffer.h>

#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/SamplerStateCache.h>

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/DeviceQueue.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <Circuit/Window/CircuitWindow.h>
#include <Circuit/CircuitManager.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <LogModule/Log.h>

#include <RenderCore/Shader/ShaderMap.h>

using namespace Volt;

namespace Circuit
{
	Circuit::CircuitRenderer::CircuitRenderer(CircuitWindow& targetCircuitWindow)
		: m_targetCircuitWindow(targetCircuitWindow), m_targetWindow(Volt::WindowManager::Get().GetWindow(targetCircuitWindow.GetWindowHandle()))
	{
		m_width = 0;
		m_height = 0;
	}

	CircuitRenderer::~CircuitRenderer()
	{
	}

	void CircuitRenderer::OnRender()
	{
		VT_PROFILE_FUNCTION();

		if (m_targetCircuitWindow.GetSize().x < 0 ||
			m_targetCircuitWindow.GetSize().y < 0)
		{
			VT_LOG(Error, "Window size must be non-zero");
			return;
		}

		const uint32_t swapchainWidth = m_targetWindow.GetSwapchain().GetWidth();
		const uint32_t swapchainHeight = m_targetWindow.GetSwapchain().GetHeight();

		if (m_width != swapchainWidth ||
			m_height != swapchainHeight)
		{
			m_width = swapchainWidth;
			m_height = swapchainHeight;
		}

		RenderGraphBlackboard rgBlackboard{};
		RenderGraph renderGraph{};

		AddCircuitPrimitivesPass(renderGraph, rgBlackboard);

		renderGraph.Compile();
		renderGraph.Execute();
	}

	struct CircuitPrimitivesPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CircuitPrimitivesPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<UICommand>, Commands)
			SHADER_PARAMETER(uint, CommandCount)
			SHADER_PARAMETER(uint2, RenderSize)

			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(CircuitPrimitivesPS, "Engine/Shaders/Source/Editor/SDFUI.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(CircuitPrimitivesParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(CircuitPrimitivesPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void CircuitRenderer::AddCircuitPrimitivesPass(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard)
	{
		VT_PROFILE_FUNCTION();

		const uint32_t swapchainWidth = m_targetWindow.GetSwapchain().GetWidth();
		const uint32_t swapchainHeight = m_targetWindow.GetSwapchain().GetHeight();

		std::vector<Circuit::CircuitDrawCommand> cmds = m_targetCircuitWindow.GetDrawCommands();

		if (cmds.empty())
		{
			return;
		}

		RGBufferDesc cmdsBufferDesc = RGBufferDesc::CreateMappableBufferDesc<Circuit::CircuitDrawCommand>(cmds.size(), RHI::BufferUsage::StorageBuffer, "UI Commands");
		RGBufferRef cmdsBuffer = renderGraph.CreateBuffer(cmdsBufferDesc);

		RGTextureRef renderTarget = renderGraph.RegisterExternalTexture(m_targetWindow.GetSwapchain().GetCurrentImage());

		AddMappedBufferUploadCopyData(renderGraph, cmdsBuffer, cmds.data(), cmds.size() * sizeof(Circuit::CircuitDrawCommand));

		CircuitPrimitivesParameters* passParameters = renderGraph.AllocParameters<CircuitPrimitivesParameters>();
		passParameters->PS.CommandCount = static_cast<uint>(cmds.size());
		passParameters->PS.RenderSize = uint2{ swapchainWidth, swapchainHeight };
		passParameters->PS.Commands = renderGraph.CreateSRV(cmdsBuffer);
		passParameters->PS.renderTargets.renderTargets[0] = renderTarget;

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<CircuitPrimitivesPS>();

		renderGraph.AddPass("Test UI",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, vertexShader, pixelShader](RenderContext& context)
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = {vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.renderTargets = passParameters->PS.renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(passParameters->PS.RenderSize.x, passParameters->PS.RenderSize.y, passParameters->PS.renderTargets);

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.SetParameters<CircuitPrimitivesPS>(pixelShader, &passParameters->PS);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}
}
