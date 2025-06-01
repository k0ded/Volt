#include "rcpch.h"
#include "RenderCore/Shader/ShaderMap.h"

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

		inline static const size_t GetShaderBindingTableHash(RefPtr<RHI::RayTracingPipeline> pipeline)
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

	bool ShaderMap::ReloadShaderByName(const std::string& name)
	{
		// #TODO_Ivar: Reimplement

		//if (!s_instance->m_shaderMap.contains(name))
		//{
		//	return false;
		//}
		//
		//auto shader = s_instance->m_shaderMap.at(name);
		//bool reloaded = shader->Reload(true);
		//
		//// #TODO_Ivar: Hack for finding out if it's a compute shader or not
		//
		//if (reloaded)
		//{
		//	if (shader->GetShaderType() == RHI::ShaderType::Compute)
		//	{
		//		for (const auto& [hash, pipeline] : s_instance->m_computePipelineCache)
		//		{
		//			if (pipeline->GetShader() == shader)
		//			{
		//				pipeline->Invalidate();
		//			}
		//		}
		//	}
		//	else if (shader->GetShaderType() == RHI::ShaderType::Rasterization)
		//	{
		//		for (const auto& [hash, pipeline] : s_instance->m_renderPipelineCache)
		//		{
		//			if (pipeline->GetShader() == shader)
		//			{
		//				pipeline->Invalidate();
		//			}
		//		}
		//	}
		//	else
		//	{
		//		for (const auto& [hash, pipeline] : s_instance->m_rayTracingPipelineCache)
		//		{
		//			if (pipeline->IsShaderInPipeline(shader))
		//			{
		//				pipeline->Invalidate();
		//			}
		//		}
		//
		//		for (const auto& [hash, sbt] : s_instance->m_shaderBindingTableCache)
		//		{
		//			if (sbt->IsShaderInTable(shader))
		//			{
		//				sbt->Invalidate();
		//			}
		//		}
		//	}
		//}
		//
		//return reloaded;

		return false;
	}

	void ShaderMap::RegisterShader2(TypeTraits::TypeIndex typeIndex, RefPtr<RHI::Shader2> shader)
	{
		std::scoped_lock lock{ s_instance->m_registerMutex };
		s_instance->m_shaderMap2[typeIndex] = shader;
	}
	RefPtr<RHI::RayTracingPipeline> ShaderMap::GetRayTracingPipeline(const RHI::RayTracingPipelineCreateInfo& pipelineInfo)
	{
		std::scoped_lock lock{ s_instance->m_rayTracingCacheMutex };
		const size_t hash = Utility::GetRayTracingPipelineHash(pipelineInfo);

		if (s_instance->m_rayTracingPipelineCache.contains(hash))
		{
			auto pipeline = s_instance->m_rayTracingPipelineCache.at(hash);
			VT_ENSURE(pipeline->IsValid());

			return pipeline;
		}

		RefPtr<RHI::RayTracingPipeline> pipeline = RHI::RayTracingPipeline::Create(pipelineInfo);
		s_instance->m_rayTracingPipelineCache[hash] = pipeline;

		VT_ENSURE(pipeline->IsValid());
		return pipeline;
	}

	RefPtr<RHI::ShaderBindingTable> ShaderMap::GetShaderBindingTable(RefPtr<RHI::RayTracingPipeline> pipeline)
	{
		std::scoped_lock lock{ s_instance->m_shaderBindingTableMutex };
		const size_t hash = Utility::GetShaderBindingTableHash(pipeline);

		if (s_instance->m_shaderBindingTableCache.contains(hash))
		{
			auto sbt = s_instance->m_shaderBindingTableCache.at(hash);
			return sbt;
		}

		RefPtr<RHI::ShaderBindingTable> sbt = RHI::ShaderBindingTable::Create(pipeline);
		s_instance->m_shaderBindingTableCache[hash] = sbt;

		return sbt;
	}
}
