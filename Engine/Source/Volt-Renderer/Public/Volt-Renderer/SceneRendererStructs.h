#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>
#include <RenderCore/RenderGraph/RenderGraph.h>

namespace Volt
{
	class Camera;
	class RenderContext;
	class RenderGraphPassResources;
	class RenderGraph;
	class RenderGraph::Builder;

	enum class VisibilityVisualization
	{
		None = 0,
		ObjectID = 1,
		TriangleID = 2
	};

	struct PreviousFrameData
	{
		glm::mat4 viewProjection = glm::identity<glm::mat4>();
		glm::vec2 jitter = 0.f;
	};

	struct ExternalImagesData
	{
		RenderGraphImageHandle black1x1Cube;
		RenderGraphImageHandle white1x1;
		RenderGraphImageHandle DFGLuT;
	};

	struct EnvironmentTexturesData
	{
		RenderGraphImageHandle radiance;
		RenderGraphImageHandle irradiance;
	};

	struct UniformBuffersData
	{
		RenderGraphUniformBufferHandle viewDataBuffer;
		RenderGraphUniformBufferHandle directionalLightShadowDataBuffer;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(GPUSceneData)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<GPUMesh>, meshesBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<GPUMeshSDF>, sdfMeshesBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<GPUMaterial>, materialsBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<PrimitiveDrawData>, primitiveDrawDataBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<PrimitiveDrawData>, prevPrimitiveDrawDataBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<float4x4>, bonesBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<LightDrawData>, lightsBuffer)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, validPrimitiveDrawDatasBuffer)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(MeshShaderCommonParameters)
		SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
		SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<MeshTaskCommand>, TaskCommands)
	END_SHADER_PARAMETER_STRUCT()

	void BuildGPUSceneData(RenderGraph::Builder& builder, const GPUSceneData& data);

	struct LightBuffersData
	{
		RenderGraphBufferHandle pointLightsBuffer;
		RenderGraphBufferHandle spotLightsBuffer;
	};

	struct DepthPrePass
	{
		RenderGraphImageHandle depth;
		RenderGraphImageHandle normals;
		RenderGraphImageHandle velocity;
	};

	struct DirectionalShadowData
	{
		RenderGraphImageHandle shadowTexture;
		glm::uvec2 renderSize;
	};

	struct VisibilityBufferData
	{
		RenderGraphImageHandle visibility;
	};

	struct MaterialCountData
	{
		RenderGraphBufferHandle materialCountBuffer;
		RenderGraphBufferHandle materialStartBuffer;
	};

	struct MaterialPixelsData
	{
		RenderGraphBufferHandle pixelCollectionBuffer;
		RenderGraphBufferHandle currentMaterialCountBuffer;
	};

	struct MaterialIndirectArgsData
	{
		RenderGraphBufferHandle materialIndirectArgsBuffer;
	};

	struct GBufferData
	{
		RenderGraphImageHandle albedo;
		RenderGraphImageHandle normals;
		RenderGraphImageHandle material;
		RenderGraphImageHandle emissive;
	};

	struct ShadingOutputData
	{
		RenderGraphImageHandle colorOutput;
	};

	struct FinalOutput
	{
		RenderGraphImageHandle colorOutput;
	};

	struct FXAAOutputData
	{
		RenderGraphImageHandle output;
	};

	struct FinalCopyData
	{
		RenderGraphImageHandle output;
	};

	struct GTAOOutput
	{
		RenderGraphImageHandle tempImage;
		RenderGraphImageHandle outputImage;
	};

	struct GTAOSettings
	{
		float radius = 50.f;
		float radiusMultiplier = 1.457f;
		float falloffRange = 0.615f;
		float finalValuePower = 2.2f;
	};

	struct TestUIData
	{
		RenderGraphImageHandle outputTextureHandle;
		RenderGraphImageHandle uiCommandsBufferHandle;
	};
}
