#include "rcpch.h"
#include "RenderCore/Shader/ShaderMap.h"
#include "RenderCore/Shader/PipelineStateCache.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/ComparisonHelpers.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	namespace Utility
	{
		inline const size_t GetRayTracingPipelineHash(const RHI::RayTracingPipelineCreateInfo& pipelineInfo)
		{
			size_t hash = 0;
			for (const auto& shader : pipelineInfo.rayGenTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			for (const auto& shader : pipelineInfo.missTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			for (const auto& shader : pipelineInfo.closestHitTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			for (const auto& shader : pipelineInfo.anyHitTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			for (const auto& shader : pipelineInfo.intersectionTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			for (const auto& shader : pipelineInfo.callableTable)
			{
				hash = Math::HashCombine(hash, std::hash<std::string_view>()(shader->GetName()));
			}

			return hash;
		}

		inline static const size_t GetShaderBindingTableHash(IntRef<RHI::RayTracingPipeline> pipeline)
		{
			return pipeline.GetHash();
		}
	}

	ShaderMap::ShaderMap()
	{
		VT_ASSERT(s_instance == nullptr);
		s_instance = this;
	}

	ShaderMap::~ShaderMap()
	{
		m_rayTracingPipelineCache.clear();
		m_shaderBindingTableCache.clear();

		s_instance = nullptr;
	}

	void ShaderMap::ReloadAll()
	{

	}

	bool ShaderMap::ReloadAllWithReferenceToFile(const std::filesystem::path& filepath)
	{
		const bool isSourceFile = filepath.extension() == L".hlsl";

		Vector<IntRef<RHI::Shader>> touchedShaders;

		// Find all shaders that have any reference to the file.
		if (isSourceFile)
		{
			for (const auto& [typeIndex, shaderBucket] : s_instance->m_shaderMap)
			{
				if (shaderBucket.hasPermutations)
				{
					for (const auto& [permutationHash, shader] : shaderBucket.permutationMap)
					{
						std::filesystem::path absoluteSourcePath = std::filesystem::absolute(shader->GetShaderSourceInfo().sourceEntry.filepath);
						if (absoluteSourcePath == filepath)
						{
							touchedShaders.emplace_back(shader);
						}
					}
				}
				else
				{
					std::filesystem::path absoluteSourcePath = std::filesystem::absolute(shaderBucket.baseShader->GetShaderSourceInfo().sourceEntry.filepath);
					if (absoluteSourcePath == filepath)
					{
						touchedShaders.emplace_back(shaderBucket.baseShader);
					}
				}
			}
		}
		else
		{
			for (const auto& [typeIndex, shaderBucket] : s_instance->m_shaderMap)
			{
				if (shaderBucket.hasPermutations)
				{
					for (const auto& [permutationHash, shader] : shaderBucket.permutationMap)
					{
						for (const auto& includeDependency : shader->GetShaderIncludeDependencies())
						{
							std::filesystem::path absoluteDependencyPath = std::filesystem::absolute(includeDependency);

							if (absoluteDependencyPath == filepath)
							{
								touchedShaders.emplace_back(shader);
								break;
							}
						}
					}
				}
				else
				{
					for (const auto& includeDependency : shaderBucket.baseShader->GetShaderIncludeDependencies())
					{
						std::filesystem::path absoluteDependencyPath = std::filesystem::absolute(includeDependency);

						if (absoluteDependencyPath == filepath)
						{
							touchedShaders.emplace_back(shaderBucket.baseShader);
							break;
						}
					}
				}
			}
		}

		// Invalidate all pipelines that reference this shader.
		for (const auto& shader : touchedShaders)
		{
			shader->Reload(true);
			PipelineStateCache::InvalidatePipelinesWithReferenceToShader(shader);
		}

		return true;
	}

	void ShaderMap::RegisterShader(TypeTraits::TypeIndex typeIndex, IntRef<RHI::Shader> shader, bool hasPermutations)
	{
		std::scoped_lock lock{ s_instance->m_registerMutex };

		ShaderBucket& shaderBucket = s_instance->m_shaderMap[typeIndex];
		shaderBucket.hasPermutations = hasPermutations;
		shaderBucket.baseShader = shader;
	}
	  
	IntRef<RHI::RayTracingPipeline> ShaderMap::GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo)
	{
		std::scoped_lock lock{ s_instance->m_rayTracingCacheMutex };
		const size_t hash = Utility::GetRayTracingPipelineHash(pipelineInfo);

		if (s_instance->m_rayTracingPipelineCache.contains(hash))
		{
			auto pipeline = s_instance->m_rayTracingPipelineCache.at(hash);
			VT_ENSURE(pipeline->IsValid());

			return pipeline;
		}

		IntRef<RHI::RayTracingPipeline> pipeline = RHI::RayTracingPipeline::Create(pipelineInfo);
		s_instance->m_rayTracingPipelineCache[hash] = pipeline;

		VT_ENSURE(pipeline->IsValid());
		return pipeline;
	}

	IntRef<RHI::ShaderBindingTable> ShaderMap::GetShaderBindingTable(IntRef<RHI::RayTracingPipeline> pipeline)
	{
		std::scoped_lock lock{ s_instance->m_shaderBindingTableMutex };
		const size_t hash = Utility::GetShaderBindingTableHash(pipeline);

		if (s_instance->m_shaderBindingTableCache.contains(hash))
		{
			auto sbt = s_instance->m_shaderBindingTableCache.at(hash);
			return sbt;
		}

		IntRef<RHI::ShaderBindingTable> sbt = RHI::ShaderBindingTable::Create(pipeline);
		s_instance->m_shaderBindingTableCache[hash] = sbt;

		return sbt;
	}

	IntRef<RHI::Shader> ShaderMap::GetInternal(TypeTraits::TypeIndex typeIndex, size_t permutationIndex, bool hasPermutationDefined)
	{
		VT_ENSURE(m_shaderMap.contains(typeIndex));
	
		const ShaderBucket& shaderBucket = m_shaderMap.at(typeIndex);

		VT_ENSURE_MSG((!hasPermutationDefined && !shaderBucket.hasPermutations) || (hasPermutationDefined && shaderBucket.hasPermutations), "Shaders with permutations must get it using it's permutation vector!");

		if (shaderBucket.hasPermutations)
		{
			if (shaderBucket.permutationMap.contains(permutationIndex))
			{
				return shaderBucket.permutationMap.at(permutationIndex);
			}
			else
			{
				return nullptr;
			}
		}
		else
		{
			return shaderBucket.baseShader;
		}
	}

	IntRef<RHI::Shader> ShaderMap::CompileShaderPermutation(TypeTraits::TypeIndex typeIndex, size_t permutationIndex, RHI::ShaderPermutationConfig&& permutationConfig)
	{
		const ShaderBucket& shaderBucket = m_shaderMap.at(typeIndex);
		const RHI::ShaderSourceInfo& sourceInfo = shaderBucket.baseShader->GetShaderSourceInfo();

		RHI::ShaderCreateInfo createInfo;
		createInfo.name = shaderBucket.baseShader->GetName();
		createInfo.entryPoint = sourceInfo.sourceEntry.entryPoint;
		createInfo.sourceFilepath = sourceInfo.sourceEntry.filepath;
		createInfo.stage = sourceInfo.sourceEntry.shaderStage;
		createInfo.permutationConfig = std::move(permutationConfig);
		createInfo.forceCompile = false;

		IntRef<RHI::Shader> shader;
		{
			VT_PROFILE_SCOPE("Compile shader permutation");
			shader = RHI::Shader::Create(createInfo);
		}

		m_shaderMap.at(typeIndex).permutationMap[permutationIndex] = shader;
		return shader;
	}
}
