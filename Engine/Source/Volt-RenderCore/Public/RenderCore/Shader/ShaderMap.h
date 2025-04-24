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

		static RefPtr<RHI::RenderPipeline> GetRenderPipeline(const RHI::RenderPipelineCreateInfo& pipelineInfo);
		static RefPtr<RHI::RayTracingPipeline> GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo);
		static RefPtr<RHI::ShaderBindingTable> GetShaderBindingTable(RefPtr<RHI::RayTracingPipeline> pipeline);

		template<typename T>
		static RefPtr<RHI::Shader> Get()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(s_instance->m_shaderMap.contains(typeIndex));

			return s_instance->m_shaderMap.at(typeIndex);
		}

		template<typename T>
		static RefPtr<RHI::ComputePipeline> GetComputePipeline(bool useGlobalResouces = true)
		{
			return GetComputePipeline(Get<T>(), useGlobalResouces);
		}

	private:
		inline static ShaderMap* s_instance = nullptr;

		static RefPtr<RHI::ComputePipeline> GetComputePipeline(RefPtr<RHI::Shader> shader, bool useGlobalResouces = true);

		vt::map<TypeTraits::TypeIndex, RefPtr<RHI::Shader>> m_shaderMap;
		vt::map<size_t, RefPtr<RHI::ComputePipeline>> m_computePipelineCache;
		vt::map<size_t, RefPtr<RHI::RenderPipeline>> m_renderPipelineCache;

		vt::map<size_t, RefPtr<RHI::RayTracingPipeline>> m_rayTracingPipelineCache;
		vt::map<size_t, RefPtr<RHI::ShaderBindingTable>> m_shaderBindingTableCache;


		std::mutex m_registerMutex;
		std::mutex m_computeCacheMutex;
		std::mutex m_renderCacheMutex;
		std::mutex m_rayTracingCacheMutex;
		std::mutex m_shaderBindingTableMutex;
	};
}
