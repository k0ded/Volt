#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompiler.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-MaterialGraph/MaterialGraph.h>

#include <Volt-Renderer/RenderMaterial.h>

#include <Volt-Core/Project/ProjectManager.h>

namespace Volt
{
	void MaterialCompiler::CompileMaterial(Ref<MaterialAsset> materialAsset)
	{
		constexpr const char* REPLACE_STRING = "GENERATED_SHADER";
		constexpr const char* BASE_OUTPUT_PATH = "Generated\\Materials";
		constexpr const char* BASE_SHADER_PATH = "Engine\\Shaders\\Source\\Generated\\GenerateGBuffer_cs.hlsl";

		constexpr size_t REPLACE_STRING_SIZE = 16;

		const auto baseShaderPath = ProjectManager::GetEngineDirectory() / BASE_SHADER_PATH;
		const std::string compilationResult = materialAsset->GetMaterialGraph()->GetMosaicGraph().Compile();

		// #TODO_Ivar: Temporary barrier
		if (compilationResult.empty())
		{
			return;
		}

		// Write shader file
		std::ifstream input(baseShaderPath, std::ios::in | std::ios::binary);
		VT_ASSERT_MSG(input.is_open(), "Could not open file!");

		std::string resultShader;

		input.seekg(0, std::ios::end);
		resultShader.resize(input.tellg());
		input.seekg(0, std::ios::beg);
		input.read(&resultShader[0], resultShader.size());

		input.close();

		const size_t replaceOffset = resultShader.find(REPLACE_STRING);
		resultShader.replace(replaceOffset, REPLACE_STRING_SIZE, compilationResult);

		const std::filesystem::path outShaderPath = ProjectManager::GetProjectDirectory() / BASE_OUTPUT_PATH / std::filesystem::path(materialAsset->assetName + "-" + materialAsset->GetMaterialGraph()->GetMaterialGUID().ToString() + ".hlsl");
		if (!std::filesystem::exists(outShaderPath.parent_path()))
		{
			std::filesystem::create_directories(outShaderPath.parent_path());
		}

		std::ofstream output(outShaderPath);
		VT_ASSERT_MSG(output.is_open(), "Could not open file!");

		output.write(resultShader.c_str(), resultShader.size());
		output.close();

		// Create new pipeline based on compiled shader
		materialAsset->GetRenderMaterial()->Invalidate(outShaderPath);
	}
}
