#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class RenderGraph;
	class RenderGraphBlackboard;
	struct RenderView;

	BEGIN_SHADER_PARAMETER_STRUCT(WorldRadianceCacheParameters)
		SHADER_PARAMETER(float, WorldRadianceCacheLodDistance)
		SHADER_PARAMETER(float, WorldRadianceCacheCellSize)
		SHADER_PARAMETER(uint32_t, WorldRadianceCacheCellLifetime)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(SpatialHashTableParameters)
		SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWSpatialHashTableChecksum)
		SHADER_PARAMETER(uint, SpatialHashTableSize)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(IrradianceVolumeParameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(IrradianceVolumeConstants, IrradianceVolumeConstantData)
		SHADER_PARAMETER(float, IrradianceVolumeBaseSpacing)
		SHADER_PARAMETER(uint32_t, IrradianceVolumeResolution)
		SHADER_PARAMETER(uint32_t, IrradianceVolumeNumCascades)
		SHADER_PARAMETER(uint32_t, IrradianceVolumeProbeResolution)
		SHADER_PARAMETER(uint32_t, IrradianceVolumeProbeAtlasResolution)
	END_SHADER_PARAMETER_STRUCT()

	class GlobalIlluminationRenderer
	{
	public:
		struct IrradianceVolumeConstants
		{
			inline static constexpr uint32_t NumMaxCascades = 10;

			int4 cascadeScrollOffset[NumMaxCascades];
			float4 cascadeMinCornerAndSpacing[NumMaxCascades];
		};

		struct Output
		{
			RGTextureRef indirectLight = nullptr;
		};

		GlobalIlluminationRenderer();

		Output Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);

		void Visualize(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);

	private:
		RGUniformBufferRef SetupIrradianceVolumeUniformBuffer(RenderGraph& renderGraph, const RenderView& view);
		WorldRadianceCacheParameters GetWorldRadianceCacheParameters();
		IrradianceVolumeParameters GetIrradianceVolumeParameters(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);
		SpatialHashTableParameters GetSpatialHashTableParameters(RenderGraph& renderGraph);

		RefPtr<RHI::StorageBuffer> m_spatialHashTableChecksumBuffer;
		RefPtr<RHI::StorageBuffer> m_worldRadianceCacheCellCache;
		RefPtr<RHI::StorageBuffer> m_worldRadianceCacheCellInfo;

		RefPtr<RHI::Image> m_irradianceVolumeProbeRadianceAtlas;
		RefPtr<RHI::Image> m_irradianceVolumeProbeVisibilityAtlas;
		RefPtr<RHI::StorageBuffer> m_irradianceVolumeProbeOffsets;
		RefPtr<RHI::StorageBuffer> m_irradianceVolumeProbeStatus;

		RefPtr<RHI::Image> m_prevIndirectLight;
	
		glm::vec3 m_prevCameraPosition = 0.f;

		IrradianceVolumeConstants m_irradianceVolumeConstants;
	};
}
