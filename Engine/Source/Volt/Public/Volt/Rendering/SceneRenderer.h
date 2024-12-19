#pragma once

#include "Volt/Core/Base.h"

#include "Volt/Rendering/SceneRendererStructs.h"
#include "Volt/Rendering/RendererStructs.h"
#include "Volt/Rendering/RenderingTechniques/GIBS.h"
#include "Volt/Rendering/RenderingTechniques/TAATechnique.h"

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

	struct SceneRendererSpecification
	{
		std::string debugName;
		glm::uvec2 initialResolution = { 1280, 720 };
		Ref<Scene> scene;
	};

	class SceneRenderer
	{
	public:
		enum class ShadingMode : uint32_t
		{
			Shaded = 0,
			Albedo = 1,
			Normals = 2,
			Metalness = 3,
			Roughness = 4,
			Emissive = 5,
			AO = 6,

			PathTracing = 7
		};

		enum class VisualizationMode : uint32_t
		{
			None = 0,
			VisualizeCascades = 1,
			VisualizeLightComplexity = 2,
			VisualizeMeshSDF = 3
		};

		enum class AntiAliasingMethod : uint8_t
		{
			FXAA,
			TAA
		};

		SceneRenderer(const SceneRendererSpecification& specification);
		~SceneRenderer();

		void OnRenderEditor(Ref<Camera> camera, float timestep);

		void Resize(const uint32_t width, const uint32_t height);
		inline void SetShadingMode(ShadingMode shadingMode) { m_shadingMode = shadingMode; }
		inline ShadingMode GetShadingMode() const { return m_shadingMode; }

		inline void SetVisualizationMode(VisualizationMode visMode) { m_visualizationMode = visMode; }
		inline VisualizationMode GetVisualizationMode() const { return m_visualizationMode; }

		RefPtr<RHI::Image> GetFinalImage();
		RefPtr<RHI::Image> GetObjectIDImage();

		// #TODO_Ivar: TEMP, Should not be public!
		void Invalidate();

		void Enable();

		const uint64_t GetFrameTotalGPUAllocationSize() const;

	private:
		void OnRender(Ref<Camera> camera, float timestep);

		void BuildMeshPass(RenderGraph::Builder& builder, RenderGraphBlackboard& blackboard);
		void SetupMeshPassConstants(RenderContext& context, const RenderGraphBlackboard& blackboard);

		void SetupFrameData(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);

		///// Passes //////
		void UploadUniformBuffers(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);
		void UploadLightBuffers(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

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

		void AddTonemapPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle srcImage);

		void AddVisualizeSDFPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage);
		void AddVisualizeBricksPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage);

		void AddPathTracingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage);

		void CreateMainRenderTarget(const uint32_t width, const uint32_t height);

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

		ShadingMode m_shadingMode = ShadingMode::Shaded;
		VisualizationMode m_visualizationMode = VisualizationMode::None;
		AntiAliasingMethod m_antiAliasingMethod = AntiAliasingMethod::TAA;

		PreviousFrameData m_previousFrameData;

		RHI::CommandBufferSet m_commandBufferSet;

		std::atomic<uint64_t> m_frameTotalGPUAllocation;

		///// TEMP /////
		VisibilityVisualization m_visibilityVisualization = VisibilityVisualization::TriangleID;
		////////////////
		
		GIBS m_gibs;
		TAANoise m_taaNoise;

		Ref<Scene> m_scene;
		SceneEnvironment m_sceneEnvironment;
	};
}
