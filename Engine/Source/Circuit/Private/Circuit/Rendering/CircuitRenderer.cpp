#include "circuitpch.h"

#include "Rendering/CircuitRenderer.h"

#include <Volt-Application/Application.h>
#include <Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/ShaderTypes.h>

#include <RenderCore/RenderGraph/Resources/RenderGraphBuffer.h>
#include <RenderCore/Shader/GlobalShaderMap.h>

#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/SamplerStateCache.h>
#include <RenderCore/DefaultBlendStates.h>

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/DeviceQueue.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/RHICapabilities.h>

#include <Circuit/Window/CircuitWindow.h>
#include <Circuit/CircuitManager.h>

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

#include <LogModule/Log.h>
 
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Math/Math.h>

using namespace Volt;

namespace Circuit
{
	Circuit::CircuitRenderer::CircuitRenderer(CircuitWindow& targetCircuitWindow, IntRef<Volt::RHI::ResourceTable> resourceTable)
		: m_targetCircuitWindow(targetCircuitWindow), 
		m_targetWindow(Volt::WindowManager_New::Get().GetWindow(targetCircuitWindow.GetWindowHandle())),
		m_resourceTable(resourceTable)
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

		// Need to make sure everything is submitted before we continue, to ensure that present happens after.
		renderGraph.ExecuteImmediate();
	}

	struct CountUIElementsCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CountUIElementsCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<UICommand>, R_Commands)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RW_PerTileCommandCount)
			SHADER_PARAMETER(uint2, NumTiles)
			SHADER_PARAMETER(uint, NumCommands)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(CountUIElementsCS, "Engine/Shaders/Source/Editor/CountUIElementsCS.hlsl", "CountUIElementsCS", Compute);

	struct CullUIElementsCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CullUIElementsCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<int>, RW_CulledUIElements)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<UICommand>, R_Commands)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, R_PerTileCommandOffset)
			SHADER_PARAMETER(uint2, NumTiles)
			SHADER_PARAMETER(uint, NumCommands)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(CullUIElementsCS, "Engine/Shaders/Source/Editor/CullUIElementsCS.hlsl", "CullUIElementsCS", Compute);

	struct CircuitPrimitivesPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CircuitPrimitivesPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<UICommand>, R_Commands)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<int>, R_CulledUIElements)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, R_PerTileCommandOffset)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, R_PerTileCommandCount)
			SHADER_PARAMETER(uint, CommandCount)
			SHADER_PARAMETER(uint2, NumTiles)
			SHADER_PARAMETER_RESOURCE_TABLE(ResourceTable)

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

		m_resourceTable->Update(static_cast<uint32_t>(Volt::Application::Get().GetFrameIndex()));

		const uint32_t swapchainWidth = m_targetWindow.GetSwapchain().GetWidth();
		const uint32_t swapchainHeight = m_targetWindow.GetSwapchain().GetHeight();

		ArrayView<Circuit::CircuitDrawCommand> commands = m_targetCircuitWindow.GetDrawCommands();

		if (commands.empty())
		{
			return;
		}

		const uint32_t numCommands = static_cast<uint32_t>(commands.size());

		RGBufferDesc cmdsBufferDesc = RGBufferDesc::CreateMappableBufferDesc<Circuit::CircuitDrawCommand>(numCommands, RHI::BufferUsage::StorageBuffer, "UI Commands");
		RGBufferRef cmdsBuffer = renderGraph.CreateBuffer(cmdsBufferDesc);

		RGTextureRef renderTarget = renderGraph.RegisterExternalTexture(m_targetWindow.GetSwapchain().GetCurrentImage());

		AddMappedBufferUploadCopyData(renderGraph, cmdsBuffer, commands.data(), commands.byte_size());

		constexpr uint32_t NumMaxUICommands = 1024;
		const glm::uvec2 numTiles = { Math::DivideRoundUp(swapchainWidth, 16u), Math::DivideRoundUp(swapchainHeight, 16u) };
		const uint32_t numTotalTiles = numTiles.x * numTiles.y;
		const uint32_t numMaxCommandIndices = numTotalTiles * NumMaxUICommands;

		RGBufferRef culledCommandIndices = renderGraph.CreateBuffer(RGBufferDesc::CreateStructuredBufferDesc<int32_t>(numMaxCommandIndices, "UI.CulledCommandIndices"));
		RGBufferRef perTileCommandCount = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(numTotalTiles, "UI.PerTileCommandCount"));
		RGBufferRef perTileCommandOffset = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(numTotalTiles, "UI.PerTileCommandOffset"));

		{
			CountUIElementsCS::Parameters* passParameters = renderGraph.AllocParameters<CountUIElementsCS::Parameters>();
			passParameters->R_Commands = renderGraph.CreateSRV(cmdsBuffer);
			passParameters->RW_PerTileCommandCount = renderGraph.CreateUAV(perTileCommandCount, RHI::PixelFormat::R32_UINT);
			passParameters->NumTiles = numTiles;
			passParameters->NumCommands = numCommands;
		
			auto shader = GlobalShaderMap::Get<CountUIElementsCS>();
			ComputeShaderUtils::AddPass<CountUIElementsCS>(renderGraph,
				"CountUIElementsCS",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ numTiles, 1 });
		}

		PrefixSumTechnique prefixSum(renderGraph);
		prefixSum.Execute(perTileCommandCount, perTileCommandOffset, numTotalTiles);

		{
			CullUIElementsCS::Parameters* passParameters = renderGraph.AllocParameters<CullUIElementsCS::Parameters>();
			passParameters->RW_CulledUIElements = renderGraph.CreateUAV(culledCommandIndices);
			passParameters->R_Commands = renderGraph.CreateSRV(cmdsBuffer);
			passParameters->R_PerTileCommandOffset = renderGraph.CreateSRV(perTileCommandOffset, RHI::PixelFormat::R32_UINT);
			passParameters->NumTiles = numTiles;
			passParameters->NumCommands = numCommands;
		
			auto shader = GlobalShaderMap::Get<CullUIElementsCS>();
			ComputeShaderUtils::AddPass<CullUIElementsCS>(renderGraph,
				"CullUIElementsCS",
				shader,
				passParameters,
				RenderGraphPassFlags::None,
				{ numTiles, 1 });
		}

		CircuitPrimitivesParameters* passParameters = renderGraph.AllocParameters<CircuitPrimitivesParameters>();
		passParameters->PS.CommandCount = numCommands;
		passParameters->PS.NumTiles = numTiles;
		passParameters->PS.R_Commands = renderGraph.CreateSRV(cmdsBuffer);
		passParameters->PS.R_CulledUIElements = renderGraph.CreateSRV(culledCommandIndices);
		passParameters->PS.R_PerTileCommandOffset = renderGraph.CreateSRV(perTileCommandOffset, RHI::PixelFormat::R32_UINT);
		passParameters->PS.R_PerTileCommandCount = renderGraph.CreateSRV(perTileCommandCount, RHI::PixelFormat::R32_UINT);
		passParameters->PS.ResourceTable = m_resourceTable;
		passParameters->PS.renderTargets.renderTargets[0] = renderTarget;

		auto vertexShader = GlobalShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = GlobalShaderMap::Get<CircuitPrimitivesPS>();

		renderGraph.AddPass("Test UI",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, vertexShader, pixelShader, swapchainWidth, swapchainHeight](RenderContext& context)
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = {vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.renderTargets = passParameters->PS.renderTargets;
			//pipelineState.attachmentBlendStates[0] = Volt::DefaultBlendStates::Alpha();

			RenderingInfo renderingInfo = context.CreateRenderingInfo(swapchainWidth, swapchainHeight, passParameters->PS.renderTargets);

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.SetParameters<CircuitPrimitivesPS>(pixelShader, &passParameters->PS);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}
}
