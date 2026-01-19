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
		static bool ReloadAllWithReferenceToFile(const std::filesystem::path& filepath);

		static void RegisterShader(TypeTraits::TypeIndex typeIndex, RefPtr<RHI::Shader> shader, bool hasPermutations);

		static RefPtr<RHI::RayTracingPipeline> GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo);
		static RefPtr<RHI::ShaderBindingTable> GetShaderBindingTable(RefPtr<RHI::RayTracingPipeline> pipeline);

		template<typename T>
		static RefPtr<RHI::Shader> Get()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			return s_instance->GetInternal(typeIndex, 0);
		}

		template<typename T>
		static RefPtr<RHI::Shader> Get(const T::PermutationVector& permutationVector)
		{
			permutationVector.Validate();
			const size_t permutationHash = permutationVector.GetHash();

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			RefPtr<RHI::Shader> shader = s_instance->GetInternal(typeIndex, permutationHash);
		
			if (!shader)
			{

				RHI::ShaderPermutationConfig permutationConfig;
				permutationVector.ResolvePermutations(permutationConfig);

				shader = s_instance->CompileShaderPermutation(typeIndex, permutationHash, std::move(permutationConfig));
			}
		}

	private:
		struct ShaderBucket
		{
			Map<size_t, RefPtr<RHI::Shader>> permutationMap;

			RefPtr<RHI::Shader> baseShader;
			bool hasPermutations = false;
		};
		
		inline static ShaderMap* s_instance = nullptr;

		RefPtr<RHI::Shader> GetInternal(TypeTraits::TypeIndex typeIndex, size_t permutationHash);
		RefPtr<RHI::Shader> CompileShaderPermutation(TypeTraits::TypeIndex typeIndex, size_t permutationHash, RHI::ShaderPermutationConfig&& permutationConfig);

		Map<TypeTraits::TypeIndex, ShaderBucket> m_shaderMap;

		Map<size_t, RefPtr<RHI::RayTracingPipeline>> m_rayTracingPipelineCache;
		Map<size_t, RefPtr<RHI::ShaderBindingTable>> m_shaderBindingTableCache;

		std::mutex m_registerMutex;
		std::mutex m_rayTracingCacheMutex;
		std::mutex m_shaderBindingTableMutex;
	};
}
