#include "rcpch.h"

#include "RenderCore/Shader/ShaderSubSystem.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Shader/ShaderCache.h>
#include <RHIModule/Shader/Shader.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/Time/ScopedTimer.h>

VT_DEFINE_LOG_CATEGORY(LogShaderSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ShaderSubSystem, Minimal, Engine, 0);

	void ShaderSubSystem::Initialize()
	{
		{
			RHI::ShaderCacheCreateInfo info{};
			info.cacheDirectory = "Engine/Shaders/Cache";

			m_shaderCache = RefPtr<RHI::ShaderCache>::Create(info);
		}

		{
			RHI::ShaderCompilerCreateInfo shaderCompilerInfo{};
			shaderCompilerInfo.flags = RHI::ShaderCompilerFlags::WarningsAsErrors;
			shaderCompilerInfo.shaderCache = m_shaderCache;

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

		TaskGraph taskGraph{};
		ScopedTimer timer{};

		for (const auto& [typeIndex, registrationInfo] : registeredShaders)
		{
			taskGraph.AddTask("Load and Register Shader", [=]()
			{
				RHI::ShaderCreateInfo createInfo;
				createInfo.name = registrationInfo.name;
				createInfo.entryPoint = registrationInfo.stageInfos.entryPoint;
				createInfo.sourceFilepath = registrationInfo.stageInfos.filePath;
				createInfo.stage = registrationInfo.stageInfos.shaderStage;

				RefPtr<RHI::Shader> shader;
				{
					VT_PROFILE_SCOPE("Create Shader");
					shader = RHI::Shader::Create(createInfo);
				}
				ShaderMap::RegisterShader(typeIndex, shader);
			});
		}

		taskGraph.ExecuteAndWait();
		VT_LOGC(Info, LogRender, "Shader compilation finished in {} seconds!", timer.GetTime<Time::Seconds>());
	}
}
