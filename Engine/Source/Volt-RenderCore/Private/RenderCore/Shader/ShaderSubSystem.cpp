#include "rcpch.h"

#include "RenderCore/Shader/ShaderSubSystem.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Shader/ShaderCache.h>
#include <RHIModule/Shader/Shader.h>

#include <JobSystem/TaskGraph.h>

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

	static ConsoleVariable<std::string> s_shaderDebugInfoPath(
		"r.Shader.ShaderDebugInfoPath",
		"Engine/Shaders/Debug/",
		"Where to output shader debug info."
	);

	VT_REGISTER_SUBSYSTEM(ShaderSubSystem, Minimal, Engine);

	void ShaderSubSystem::Initialize()
	{
		{
			RHI::ShaderCacheCreateInfo info{};
			info.cacheDirectory = "Engine/Shaders/Cache";

			m_shaderCache = RefPtr<RHI::ShaderCache>::Create(info);
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
			shaderCompilerInfo.shaderDebugInfoPath = s_shaderDebugInfoPath.GetValue();

			const std::filesystem::path engineShaderIncludeDirectory = "Engine/Shaders/Source/Includes";
			const std::filesystem::path engineShaderDirectory = "Engine/Shaders/Source/";
			shaderCompilerInfo.includeDirectories =
			{
				engineShaderIncludeDirectory,
				engineShaderDirectory
			};

			m_shaderCompiler = RHI::ShaderCompiler::Create(shaderCompilerInfo);
		}

		m_shaderMap = CreateScope<ShaderMap>();
		m_pipelineStateCache = CreateScope<PipelineStateCache>();
		LoadRegisteredShaders();
	}

	void ShaderSubSystem::Shutdown()
	{
		m_shaderCompiler = nullptr;
		m_shaderCache = nullptr;
	}

	void ShaderSubSystem::LoadRegisteredShaders()
	{
		const auto& registeredShaders = ShaderRegistry::Get().GetRegisteredShaders();

		auto compileFunc = [=](TypeTraits::TypeIndex typeIndex, ShaderRegistry::ShaderRegistrationInfo registrationInfo)
		{
			RHI::ShaderCreateInfo createInfo;
			createInfo.name = registrationInfo.name;
			createInfo.entryPoint = registrationInfo.stageInfos.entryPoint;
			createInfo.sourceFilepath = registrationInfo.stageInfos.filePath;
			createInfo.stage = registrationInfo.stageInfos.shaderStage;
			createInfo.forceCompile = false;

			RefPtr<RHI::Shader> shader;
			{
				VT_PROFILE_SCOPE("Create Shader");
				shader = RHI::Shader::Create(createInfo);
			}
			ShaderMap::RegisterShader(typeIndex, shader, registrationInfo.stageInfos.hasPermutations);
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
