#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Pipelines/RayTracingPipeline.h>
#include <RHIModule/RayTracing/ShaderBindingTable.h>

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Locks/SpinMutex.h>

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
		static bool ReloadAllWithReferenceToFile(const Filesystem::Path& filepath);

		static void RegisterShader(TypeTraits::TypeIndex typeIndex, IntRef<RHI::Shader> shader, bool hasPermutations);

		static IntRef<RHI::RayTracingPipeline> GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo);
		static IntRef<RHI::ShaderBindingTable> GetShaderBindingTable(IntRef<RHI::RayTracingPipeline> pipeline);

		template<typename T>
		static IntRef<RHI::Shader> Get()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			return s_instance->GetInternal(typeIndex, 0, false);
		}

		template<typename T>
		static IntRef<RHI::Shader> Get(const T::PermutationVector& permutationVector)
		{
			permutationVector.Validate();
			const size_t permutationIndex = permutationVector.GetPermutationIndex();

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			IntRef<RHI::Shader> shader = s_instance->GetInternal(typeIndex, permutationIndex, true);
		
			if (!shader)
			{

				RHI::ShaderPermutationConfig permutationConfig;
				permutationVector.ResolvePermutations(permutationConfig);

				shader = s_instance->CompileShaderPermutation(typeIndex, permutationIndex, std::move(permutationConfig));
			}

			return shader;
		}

		static IntRef<RHI::Shader> Get(TypeTraits::TypeIndex typeIndex)
		{
			return s_instance->GetInternal(typeIndex, 0, false);
		}

	private:
		struct ShaderBucket
		{
			Map<size_t, IntRef<RHI::Shader>> permutationMap;

			IntRef<RHI::Shader> baseShader;
			bool hasPermutations = false;
		};
		
		inline static ShaderMap* s_instance = nullptr;

		IntRef<RHI::Shader> GetInternal(TypeTraits::TypeIndex typeIndex, size_t permutationIndex, bool hasPermutationDefined);
		IntRef<RHI::Shader> CompileShaderPermutation(TypeTraits::TypeIndex typeIndex, size_t permutationIndex, RHI::ShaderPermutationConfig&& permutationConfig);

		Map<TypeTraits::TypeIndex, ShaderBucket> m_shaderMap;

		Map<size_t, IntRef<RHI::RayTracingPipeline>> m_rayTracingPipelineCache;
		Map<size_t, IntRef<RHI::ShaderBindingTable>> m_shaderBindingTableCache;

		SpinMutex m_registerMutex;
		std::mutex m_rayTracingCacheMutex;
		std::mutex m_shaderBindingTableMutex;
	};
}
