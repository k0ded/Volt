#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialCompiler.h"

#include <Volt-Platforms/Platform.h>
#include <Volt-Core/Console/ConsoleVariableRegistry.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <JobSystem/JobSystem.h>

#include <CoreUtilities/FileIO/FileUtility.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_forceSingleThreadedCompilation(
		"r.MaterialCompiler.ForceSingleThreadedCompilation",
		0,
		"Whether or not force single threaded compilation of materials.");

	VT_REGISTER_SUBSYSTEM(MaterialCompilerSubSystem, Default, PostEngine);

	void MaterialCompilerSubSystem::Initialize()
	{
		ReadMaterialShaderFileContents();
	}

	void MaterialCompilerSubSystem::Shutdown()
	{
	}

	void MaterialCompilerSubSystem::RequestMaterialCompilation(AssetReference<MaterialAsset> materialAsset)
	{
		if (!s_forceSingleThreadedCompilation.GetValue())
		{
			JobRef compileJob = JobSystem::CreateJob("Compile Material", ExecutionPriority::Latent, [materialAsset = std::move(materialAsset)]()
			{
				MaterialCompiler compiler;
				compiler.CompileMaterial(materialAsset);
			}, FiberStackSize::KB32);

			JobSystem::RunJob(compileJob);
		}
		else
		{
			MaterialCompiler compiler;
			compiler.CompileMaterial(materialAsset);
		}
	}

	void MaterialCompilerSubSystem::ReadMaterialShaderFileContents()
	{
		const std::filesystem::path materialShaderFilepath = ProjectManager::GetEngineAssetsDirectory() / "Shaders" / "Source" / "Material" / "MaterialShader.hlsli";
		VT_MAYBE_UNUSED bool readFile = FileUtility::ReadStringFromFile(materialShaderFilepath, m_materialShaderFileContents);
		VT_ENSURE(readFile);
	}
}
