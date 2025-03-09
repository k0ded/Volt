#include "rcpch.h"

#include "RenderCore/Shader/ShaderSubSystem.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <Volt-Core/Project/ProjectManager.h>

#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Shader/ShaderCache.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/Time/ScopedTimer.h>

VT_DEFINE_LOG_CATEGORY(LogShaderSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ShaderSubSystem, Engine, 0);

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

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
			shaderCompilerInfo.flags |= RHI::ShaderCompilerFlags::EnableShaderValidator;
#endif

			shaderCompilerInfo.includeDirectories =
			{
				ProjectManager::GetEngineShaderIncludeDirectory(),
				ProjectManager::GetEngineShaderDirectory(),
				ProjectManager::GetAssetsDirectory()
			};

			m_shaderCompiler = RHI::ShaderCompiler::Create(shaderCompilerInfo);
		}

		m_shaderMap = CreateScope<ShaderMap>();
		LoadRegisteredShaders();
	}

	void ShaderSubSystem::Shutdown()
	{
		m_shaderCompiler = nullptr;
		m_shaderCache = nullptr;
	}

	void ShaderSubSystem::LoadRegisteredShaders()
	{
		const auto& registeredShaders = GetShaderRegistry().GetRegisteredShaders();

		TaskGraph taskGraph{};
		ScopedTimer timer{};

		for (const auto& [typeIndex, registrationInfo] : registeredShaders)
		{
			taskGraph.AddTask([=]() 
			{
				RHI::ShaderSpecification specification;
				specification.name = registrationInfo.name;

				for (const auto& stageInfo : registrationInfo.stageInfos)
				{
					auto& sourceEntry = specification.sourceEntries.emplace_back();
					sourceEntry.entryPoint = stageInfo.entryPoint;
					sourceEntry.filePath = stageInfo.filePath;
					sourceEntry.shaderStage = stageInfo.shaderStage;
				}

				specification.forceCompile = false;

				RefPtr<RHI::Shader> shader = RHI::Shader::Create(specification);
				ShaderMap::RegisterShader(typeIndex, shader);
			});
		}

		taskGraph.ExecuteAndWait();
		VT_LOGC(Info, LogRender, "Shader compilation finished in {} seconds!", timer.GetTime<Time::Seconds>());
	}
}
