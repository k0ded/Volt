#pragma once

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RenderingTechniques/DDGI.h"
#include "Volt-Renderer/RenderingTechniques/TAATechnique.h"
#include "Volt-Renderer/RenderingTechniques/VolumetricFogTechnique.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/RenderGraphDebugger.h>

// #TODO_Ivar: Maybe remove from here
#include <RenderCore/RenderGraph/RenderGraph.h>

#include <RHIModule/Buffers/CommandBufferSet.h>

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
		class StorageBuffer;

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

	class RenderGraph::Builder;

	struct SceneRendererCreateInfo
	{
		std::string debugName;
		glm::uvec2 initialResolution = { 1280, 720 };

		Ref<RenderScene> renderScene;
	};

	class VTR_API SceneRenderer
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
			FXAA,
			TAA
		};

		SceneRenderer(const SceneRendererCreateInfo& specification);
		~SceneRenderer();

		void OnRenderEditor(Ref<Camera> camera, float timestep);

		void Resize(const uint32_t width, const uint32_t height);

		inline void SetVisualizationMode(VisualizationMode visualizationMode) { m_visualizationMode = visualizationMode; }
		inline VisualizationMode GetVisualizationMode2() const { return m_visualizationMode; }

		inline const RenderGraphDebugger& GetRenderGraphDebugger() const { return m_renderGraphDebugger; }

		RefPtr<RHI::Image> GetFinalImage();
		RefPtr<RHI::Image> GetObjectIDImage();

		// #TODO_Ivar: TEMP, Should not be public!
		void Invalidate();

		void Enable();

		const uint64_t GetFrameTotalGPUAllocationSize() const;

	private:
		void OnRender(Ref<Camera> camera, float timestep);

		void BuildMeshPass(RenderGraph::Builder& builder, RenderGraphBlackboard& blackboard);

		void SetupFrameData(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);

		///// Passes //////
		void UploadUniformBuffers(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);

		void AddExternalResources(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void ExecuteGBufferGenerationPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void ExecutePostProcessingPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, float timestep);

		void AddMainCullingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddObjectIDPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddGTAOPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);
		void AddVisibilityBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void AddClearGBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void AddGenerateMaterialCountsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddCollectMaterialPixelsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddGenerateMaterialIndirectArgsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void RenderMaterials(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddGenerateGBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const uint32_t materialId);
		void AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void AddShadingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void AddFXAAPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle srcImage);

		void AddTonemappingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle srcImage);

		void AddVisualizationPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage);

		void AddPathTracingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage);

		void CreateMainRenderTarget(const uint32_t width, const uint32_t height);

		bool ShouldApplyJitter() const;
		bool IsMeshPassVisualizationMode() const;

		bool m_enabled = false;

		RefPtr<RHI::Image> m_outputImage;
		RefPtr<RHI::Image> m_objectIDImage;
		RefPtr<RHI::Image> m_previousColorImage;
		RefPtr<RHI::Image> m_averageLuminanceImage;

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

		AntiAliasingMethod m_antiAliasingMethod = AntiAliasingMethod::TAA;
		VisualizationMode m_visualizationMode = VisualizationMode::None;

		PreviousFrameData m_previousFrameData;

		RHI::CommandBufferSet m_commandBufferSet;
		RenderGraphDebugger m_renderGraphDebugger;

		std::atomic<uint64_t> m_frameTotalGPUAllocation;

		///// TEMP /////
		VisibilityVisualization m_visibilityVisualization = VisibilityVisualization::TriangleID;
		////////////////
		
		DDGI m_ddgi;
		TAANoise m_taaNoise;
		VolumetricFogTechnique m_volumetricFog;

		Ref<RenderScene> m_renderScene;
		Renderer::EnvironmentTextures m_sceneEnvironment;
	};
}
