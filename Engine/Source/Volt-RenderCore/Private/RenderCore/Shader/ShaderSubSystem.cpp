#include "rcpch.h"

#include "RenderCore/Shader/ShaderSubSystem.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"
#include "RenderCore/Shader/IORequestCompileShader.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>
#include <CoreModule/ConfigManager.h>

#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Shader/ShaderCache.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/RHIFeatures.h>

#include <JobSystem/TaskGraph.h>

#include <FileSystemModule/IOThreads/IOThreads.h>

#include <CoreUtilities/Time/ScopedTimer.h>

VT_DEFINE_LOG_CATEGORY(LogShaderSubSystem);

namespace Volt
{
	static ConsoleVariable<int32_t> s_forceSingleThreadedCompilation(
		"r.Shader.ForceSingleThreadedCompilation",
		0,
		"Whether or not force single threaded compilation of shaders.");

	static ConsoleVariable<int32_t> s_outputShaderDebugInfo(
		"r.Shader.OutputShaderDebugInfo",
		0,
		"Whether or not to output shader debug info."
	);

	static ConsoleVariable<int32_t> s_shaderOptimizationLevel(
		"r.Shader.Optimization",
		0,
		"Shader Optimization level\n"
		"0. Disabled\n"
		"1. Release\n"
		"2. Dist\n"
	);

	static ConsoleVariable<int32_t> s_shaderWarningsAsErrors(
		"r.Shader.WarningsAsErrors",
		1,
		"Whether or not to use warnings as errors."
	);

	VT_REGISTER_SUBSYSTEM(ShaderSubSystem, Minimal, Engine);

	void ShaderSubSystem::Initialize()
	{
		{
			RHI::ShaderCacheCreateInfo info{};
			m_shaderCache = IntRef<RHI::ShaderCache>::Create(info);
		}

		{
			RHI::ShaderCompilerCreateInfo shaderCompilerInfo{};
			shaderCompilerInfo.flags = RHI::ShaderCompilerFlags::None;

			if (s_shaderWarningsAsErrors.GetValue())
			{
				shaderCompilerInfo.flags |= RHI::ShaderCompilerFlags::WarningsAsErrors;
			}

			if (s_outputShaderDebugInfo.GetValue())
			{
				shaderCompilerInfo.flags |= RHI::ShaderCompilerFlags::OutputShaderDebugInfo;
			}

			if (s_shaderOptimizationLevel.GetValue() == 0)
			{
				shaderCompilerInfo.optimizationLevel = RHI::ShaderOptimizationLevel::Disable;
			}
			else if (s_shaderOptimizationLevel.GetValue() == 1)
			{
				shaderCompilerInfo.optimizationLevel = RHI::ShaderOptimizationLevel::Release;
			}
			else if (s_shaderOptimizationLevel.GetValue() == 2)
			{
				shaderCompilerInfo.optimizationLevel = RHI::ShaderOptimizationLevel::Dist;
			}

			shaderCompilerInfo.shaderCache = m_shaderCache;

			const Filesystem::Path engineShaderIncludeDirectory = "Engine/Shaders/Source/Includes";
			const Filesystem::Path engineShaderDirectory = "Engine/Shaders/Source/";
			shaderCompilerInfo.includeDirectories =
			{
				engineShaderIncludeDirectory,
				engineShaderDirectory
			};

			if (RHI::RHICanUseBindless())
			{
				shaderCompilerInfo.initialMacros.emplace_back("BINDLESS_ENABLED=1");
			}

			m_shaderCompiler = RHI::ShaderCompiler::Create(shaderCompilerInfo);
		}

		m_shaderMap = CreateUnique<GlobalShaderMap>();
		m_pipelineStateCache = CreateUnique<PipelineStateCache>();
		LoadRegisteredShaders();
	}

	void ShaderSubSystem::Shutdown()
	{
		m_shaderCompiler = nullptr;
		m_shaderCache = nullptr;
	}

	void ShaderSubSystem::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<ConfigManager>();
	}

	void ShaderSubSystem::LoadRegisteredShaders()
	{
		const auto& registeredShaders = ShaderRegistry::Get().GetRegisteredShaders();

		auto compileFunc = [=](TypeTraits::TypeIndex typeIndex, ShaderRegistry::ShaderRegistrationInfo registrationInfo)
		{
			if (registrationInfo.stageInfos.hasPermutations)
			{
				Vector<RHI::ShaderCreateInfo> createInfos;

				registrationInfo.stageInfos.iteratePermutationsFunc([&](RHI::ShaderPermutationConfig&& permutationConfig, size_t permutationIndex) 
				{
					bool shouldCompilePermutation = true;
					if (registrationInfo.stageInfos.shouldCompilePermutationFunc)
					{
						GlobalShaderPermutationParameters parameters;
						parameters.permutationIndex = permutationIndex;

						shouldCompilePermutation = registrationInfo.stageInfos.shouldCompilePermutationFunc(parameters);
					}

					if (shouldCompilePermutation)
					{
						RHI::ShaderCreateInfo& createInfo = createInfos.emplace_back();
						createInfo.name = registrationInfo.name;
						createInfo.entryPoint = registrationInfo.stageInfos.entryPoint;
						createInfo.sourceFilepath = registrationInfo.stageInfos.filePath;
						createInfo.stage = registrationInfo.stageInfos.shaderStage;
						createInfo.permutationConfig = std::move(permutationConfig);
						createInfo.forceCompile = false;
					}
				});

				IORequestResult<IORequestCompileShader_Multiple> result = IOThreads::SubmitRequest<IORequestCompileShader_Multiple>("Create Shaders", std::move(createInfos));

				if (result.GetResultCode() == IORequestResultCode::Success)
				{
					Map<size_t, IntRef<RHI::Shader>> permutationMap;

					for (const IORequestCompileShader_Multiple::Result& shaderResult : result.GetResult())
					{
						permutationMap[shaderResult.permutationIndex] = shaderResult.shader;
					}

					GlobalShaderMap::RegisterShader(typeIndex, permutationMap);
				}
			}
			else
			{
				bool shouldCompilePermutation = true;

				if (registrationInfo.stageInfos.shouldCompilePermutationFunc)
				{
					GlobalShaderPermutationParameters parameters;
					parameters.permutationIndex = 0;

					shouldCompilePermutation = registrationInfo.stageInfos.shouldCompilePermutationFunc(parameters);
				}

				if (shouldCompilePermutation)
				{
					RHI::ShaderCreateInfo createInfo;
					createInfo.name = registrationInfo.name;
					createInfo.entryPoint = registrationInfo.stageInfos.entryPoint;
					createInfo.sourceFilepath = registrationInfo.stageInfos.filePath;
					createInfo.stage = registrationInfo.stageInfos.shaderStage;
					createInfo.forceCompile = false;

					IORequestResult<IORequestCompileShader> result = IOThreads::SubmitRequest<IORequestCompileShader>("Create Shader", createInfo);

					if (result.GetResultCode() == IORequestResultCode::Success)
					{
						GlobalShaderMap::RegisterShader(typeIndex, result.GetResult());
					}
				}
			}
		};

		TaskGraph taskGraph{ ExecutionPriority::Immediate };
		ScopedTimer timer{};

		for (const auto& [typeIndex, registrationInfo] : registeredShaders)
		{
			if (!s_forceSingleThreadedCompilation.GetValue())
			{
				taskGraph.AddTask("Load and Register Shader", [typeIndex, registrationInfo, compileFunc]()
				{
					compileFunc(typeIndex, registrationInfo);
				});
			}
			else
			{
				compileFunc(typeIndex, registrationInfo);
			}
		}

		if (!s_forceSingleThreadedCompilation.GetValue())
		{
			taskGraph.ExecuteAndWait();
		}
		VT_LOGC(Info, LogRender, "Shader compilation finished in {} seconds!", timer.GetTime<Time::Seconds>());
	}
}
