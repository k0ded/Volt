#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialCompiler.h"

#include <Volt-Platforms/Platform.h>

#include <JobSystem/JobSystem.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(MaterialCompilerSubSystem, Default, PostEngine);

	void MaterialCompilerSubSystem::Initialize()
	{
	}

	void MaterialCompilerSubSystem::Shutdown()
	{
	}

	void MaterialCompilerSubSystem::RequestMaterialCompilation(AssetReference<MaterialAsset> materialAsset)
	{
		JobRef compileJob = JobSystem::CreateJob("Compile Material", ExecutionPriority::Latent, [materialAsset = std::move(materialAsset)]()
		{
			MaterialCompiler compiler;
			compiler.CompileMaterial(materialAsset);
		});

		JobSystem::RunJob(compileJob);
	}
}
