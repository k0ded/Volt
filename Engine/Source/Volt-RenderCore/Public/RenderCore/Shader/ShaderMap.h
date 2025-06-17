#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Pipelines/RayTracingPipeline.h>
#include <RHIModule/RayTracing/ShaderBindingTable.h>

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/TypeTraits/TypeIndex.h>

#include <string>

namespace Volt
{
	namespace RHI
	{
		struct RenderPipelineCreateInfo;
		struct RayTracingPipelineCreateInfo;
	}

	class VTRC_API ShaderMap
	{
	public:
		ShaderMap();
		~ShaderMap();

		static void ReloadAll();
		static bool ReloadShaderByName(const std::string& name);

		static void RegisterShader(TypeTraits::TypeIndex typeIndex, RefPtr<RHI::Shader> shader);

		static RefPtr<RHI::RayTracingPipeline> GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo);
		static RefPtr<RHI::ShaderBindingTable> GetShaderBindingTable(RefPtr<RHI::RayTracingPipeline> pipeline);

		template<typename T>
		static RefPtr<RHI::Shader> Get()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(s_instance->m_shaderMap2.contains(typeIndex));

			return s_instance->m_shaderMap2.at(typeIndex);
		}

	private:
		inline static ShaderMap* s_instance = nullptr;

		Map<TypeTraits::TypeIndex, RefPtr<RHI::Shader>> m_shaderMap2;

		Map<size_t, RefPtr<RHI::RayTracingPipeline>> m_rayTracingPipelineCache;
		Map<size_t, RefPtr<RHI::ShaderBindingTable>> m_shaderBindingTableCache;

		std::mutex m_registerMutex;
		std::mutex m_rayTracingCacheMutex;
		std::mutex m_shaderBindingTableMutex;
	};
}
