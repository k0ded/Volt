#pragma once

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/SceneRendererExtension.h"
#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/RenderingTechniques/TAANoise.h"
#include "Volt-Renderer/GlobalIllumination/GlobalIlluminationRenderer.h"
#include "Volt-Renderer/MeshPassProcessor.h"

#include <RenderCore/RenderGraph/RenderGraphDebugger.h>
#include <RenderCore/Resources/GrowingGPUBuffer.h>

#include <JobSystem/Job.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Delegates/DelegateHandle.h>

namespace Volt
{
	namespace RHI
	{
		class Image2D;
		class CommandBuffer;
		class Shader;
		class RenderPipeline;
		class ComputePipeline;

		class UniformBufferSet;
		class Buffer;

		class SamplerState;

		class DescriptorTable;
	}

	class Mesh;
	class Camera;
	class Scene;
	class RenderScene;
	class RenderContext;

	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;
	struct RenderLightData;

	struct TranslucencyCompositePS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TranslucencyCompositePS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Accumulation)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Revealage)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	struct SceneRendererCreateInfo
	{
		std::string debugName;
		glm::uvec2 initialResolution = { 1280, 720 };
		bool drawDebug = false;

		Ref<RenderScene> renderScene;
	};

	class VTR_API SceneRenderer : public EventListener
	{
	public:
		enum class VisualizationMode : uint8_t
		{
			None = 0,
			BaseColor = 1,
			Metallic = 2,
			Roughness = 3,
			SceneColor = 4,
			SceneDepth = 5,
			WorldNormal = 6,
			GeometryNormals = 7,
			AmbientOcclusion = 8,
			Velocity = 9,
			UV = 10,
			GeometryTangents = 11
		};

		enum class AntiAliasingMethod : uint8_t
		{
			None,
			FXAA,
			TAA
		};

		SceneRenderer(const SceneRendererCreateInfo& specification);
		~SceneRenderer() override;

		void OnRenderEditor(Ref<Camera> camera, float timestep);

		void Resize(const uint32_t width, const uint32_t height);

		inline void SetVisualizationMode(VisualizationMode visualizationMode) { m_visualizationMode = visualizationMode; }
		inline VisualizationMode GetVisualizationMode2() const { return m_visualizationMode; }

		inline const RenderGraphDebugger& GetRenderGraphDebugger() const { return m_renderGraphDebugger; }

		IntRef<RHI::Image> GetFinalImage();

		void Enable();

		const uint64_t GetFrameTotalGPUAllocationSize() const;

		template<typename T, typename... Args>
		Ref<T> AddExtension(SceneRendererExtensionStage stage, Args&&... args);

	private:
		using SceneRendererExtensionMap = Map<SceneRendererExtensionStage, Vector<Ref<SceneRendererExtension>>>;

		void OnRender(Ref<Camera> camera, float timestep);

		///// Render Passes /////
		void AddDefaultTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddEnvironmentTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);
		void AddBasePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);
		void AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);
		void AddShadingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef directionalShadowMap, RGUniformBufferRef directionalShadowUniformBuffer, RGTextureRef indirectLightTexture);
		void AddTranslucencyPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef directionalShadowMap, RGUniformBufferRef directionalShadowUniformBuffer);
		void AddTranslucencyCompositePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);
		void AddPostProcessingPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef outputTexture);
		void AddTonemappingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef outputTexture);
		/////////////////////////

		void CreateMainRenderTarget(const uint32_t width, const uint32_t height);
		void AddMeshPassProcessors();

		RGUniformBufferRef CreateViewUniformBuffer(RenderGraph& renderGraph, Ref<Camera> camera);

		bool ShouldApplyJitter() const;
		bool IsMeshPassVisualizationMode() const;

		bool OnPostFrameUpdateEvent(AppPostFrameUpdateEvent& event);
		RGTextureRef ExecuteSceneRendererExtensions(SceneRendererExtensionStage stage, RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef prevOutputImage);

		bool m_enabled = false;

		IntRef<RHI::Image> m_outputImage;
		IntRef<RHI::Image> m_previousColorImage;
		IntRef<RHI::Image> m_averageLuminanceImage;

		Ref<Mesh> m_skyboxMesh;

		bool m_shouldResize = false;

		uint32_t m_width = 1280;
		uint32_t m_height = 720;

		uint32_t m_resizeWidth = 1280;
		uint32_t m_resizeHeight = 1280;
			
		uint32_t m_frameIndex = 0;

		glm::mat4 m_prevViewProjection = 1.f;
		glm::vec2 m_currentJitter = 0.f;
		glm::vec2 m_prevJitter = 0.f;

		AntiAliasingMethod m_antiAliasingMethod = AntiAliasingMethod::None;
		VisualizationMode m_visualizationMode = VisualizationMode::None;

		PreviousFrameData m_previousFrameData;
		JobCounterRef m_renderGraphExecutionCounter = nullptr;

		RenderGraphDebugger m_renderGraphDebugger;
		SceneRendererCreateInfo m_createInfo;

		std::atomic<uint64_t> m_frameTotalGPUAllocation;

		///// TEMP /////
		VisibilityVisualization m_visibilityVisualization = VisibilityVisualization::TriangleID;
		////////////////
		
		Ref<RenderScene> m_renderScene;
		TAANoise m_taaNoise;

		GlobalIlluminationRenderer m_globalIlluminationRenderer;

		// Extensions
		SceneRendererExtensionMap m_sceneRendererExtensions;

		// Mesh passes
		MeshPassProcessorRegistry m_meshPassProcessorRegistry;
		DelegateHandle m_renderPrimitiveAddedDelegateHandle = 0;
		DelegateHandle m_renderPrimitiveRemovedDelegateHandle = 0;

		class DepthPrePassMeshProcessor* m_depthPrePassMeshProcessor = nullptr;
		class BasePassMeshProcessor* m_basePassMeshProcessor = nullptr;
		class CascadedShadowMapMeshProcessor* m_cascadedShadowMapMeshProcessor = nullptr;
		class TranslucencyMeshPassProcessor* m_translucencyMeshPassProcessor = nullptr;
	};

	template<typename T, typename... Args>
	Ref<T> SceneRenderer::AddExtension(SceneRendererExtensionStage stage, Args&&... args)
	{
		static_assert(std::is_base_of_v<SceneRendererExtension, T>);

		Ref<T> instance = CreateRef<T>(m_renderScene, std::forward<Args>(args)...);
		instance->OnRegistered(m_meshPassProcessorRegistry);

		m_sceneRendererExtensions[stage].emplace_back(instance);
		return instance;
	}
}
