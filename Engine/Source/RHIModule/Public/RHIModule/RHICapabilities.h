#pragma once

#include "RHIModule/Core/Core.h"

namespace Volt::RHI
{
	struct RHICapabilities
	{
		struct ComputeDimensions
		{
			uint32_t x = 0;
			uint32_t y = 0;
			uint32_t z = 0;
		};

		inline static constexpr uint32_t NumFramesInFlight = 3;

		uint32_t max2DTextureDimensions = 2048;
		uint64_t maxBufferDimensions = (1 << 27);
		uint32_t max3DTextureDimensions = 2048;
		uint32_t maxCubeTextureDimensions = 2048;
		uint32_t maxTextureArrayLayers = 256;
		uint32_t maxTextureSamplers = 16;
		uint64_t maxComputeSharedMemorySize = (1 << 15);
		uint32_t maxWorkGroupInvocations = 1024;
		uint64_t minUniformBufferAlignment = 0;
		uint64_t maxUniformBufferViewSize = 65536;

		struct RayTracing
		{
			bool supportsRayTracing = false;
			bool supportsInlineRaytracing = false;
			bool supportsRaytracingShaders = false;

			uint32_t accelerationStructureAlignment = 0;
			uint32_t scratchBufferAlignment = 0;
			uint32_t shaderTableAlignment = 0;

		} rayTracing;

		uint32_t minimumWaveSize = 0;
		uint32_t maximumWaveSize = 0;

		bool supportsNative16BitOperations = false;
		bool supportsMeshShaders = false;
		bool supportsBindless = false;

		ComputeDimensions maxDispatchThreadGroupsPerDimension;

		// #TODO_Ivar: Move to some other place
		bool useMeshShaders = false;
		bool useBindless = false;
		bool useRayTracing = false;
	};
}
extern VTRHI_API Volt::RHI::RHICapabilities g_rhiCapabilities;
