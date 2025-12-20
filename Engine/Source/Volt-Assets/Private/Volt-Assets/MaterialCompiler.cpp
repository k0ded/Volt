#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompiler.h"
#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <Volt-Renderer/RenderMaterial.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Texture/Texture2D.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogMaterialCompiler);

namespace Volt
{
	std::string ReadBaseFile()
	{
		constexpr const char* BaseShaderPath = "Shaders\\Source\\Template\\GenerateGBufferPixel.hlsl";
		const auto baseShaderPath = ProjectManager::GetEngineAssetsDirectory() / BaseShaderPath;

		std::ifstream input(baseShaderPath, std::ios::in | std::ios::binary);
		VT_ASSERT_MSG(input.is_open(), "Could not open file!");

		std::string resultShader;

		input.seekg(0, std::ios::end);
		resultShader.resize(input.tellg());
		input.seekg(0, std::ios::beg);
		input.read(&resultShader[0], resultShader.size());

		input.close();

		return resultShader;
	}

	inline void InsertTextureDeclarations(std::string& shaderString, const Mosaic::MosaicShaderWriter& shaderWriter)
	{
		VT_PROFILE_FUNCTION();

		constexpr const char* TextureDeclarationTag = "$(TextureDeclarations)";

		const Vector<Mosaic::MosaicShaderWriter::TextureDeclaration>& textureDeclarations = shaderWriter.GetTextureDeclarations();

		std::stringstream textureDeclarationStringStream;
		for (const Mosaic::MosaicShaderWriter::TextureDeclaration& texture : textureDeclarations)
		{
			textureDeclarationStringStream << "Texture2D " << texture.name << ";\n";
		}

		auto textureDeclarationTagOffset = shaderString.find(TextureDeclarationTag);
		const size_t tagLength = strlen(TextureDeclarationTag);

		shaderString.replace(textureDeclarationTagOffset, tagLength, textureDeclarationStringStream.str());
	}

	inline void InsertMaterialEvaluation(std::string& shaderString, const Mosaic::MosaicShaderWriter& shaderWriter)
	{
		VT_PROFILE_FUNCTION();

		constexpr const char* EvaluateMaterialTag = "$(EvaluateMaterial)";
		
		auto evaluateMaterialTagOffset = shaderString.find(EvaluateMaterialTag);
		const size_t tagLength = strlen(EvaluateMaterialTag);

		shaderString.replace(evaluateMaterialTagOffset, tagLength, shaderWriter.GetAsString());
	}

	void MaterialCompiler::CompileMaterial(AssetReference<MaterialAsset> materialAsset)
	{
		VT_PROFILE_FUNCTION();

		ScopedAssetReferenceLock materialAssetLock{ materialAsset };

		VT_LOGC(Trace, LogMaterialCompiler, "Started compilation of material {}", materialAsset->GetAssetName());

		ScopedTimer timer;

		constexpr const char* BaseOutputPath = "Generated\\Materials";

		const Mosaic::MosaicShaderWriter compilationResult = materialAsset->GetMaterialGraph()->GetMosaicGraph().Compile();

		// #TODO_Ivar: Temporary barrier
		if (compilationResult.GetAsString().empty())
		{
			return;
		}

		// Write shader file
		std::string shaderString = ReadBaseFile();
		InsertTextureDeclarations(shaderString, compilationResult);
		InsertMaterialEvaluation(shaderString, compilationResult);

		const std::filesystem::path outShaderPath = ProjectManager::GetProjectDirectory() / BaseOutputPath / std::filesystem::path(std::string(materialAsset->GetAssetName()) + "-" + materialAsset->GetMaterialGraph()->GetMaterialGUID().ToString() + ".hlsl");
		if (!std::filesystem::exists(outShaderPath.parent_path()))
		{
			std::filesystem::create_directories(outShaderPath.parent_path());
		}

		{
			std::ofstream output(outShaderPath);
			VT_ASSERT_MSG(output.is_open(), "Could not open file!");

			output.write(shaderString.c_str(), shaderString.size());
			output.close();
		}

		// Add textures
		const Vector<Mosaic::MosaicShaderWriter::TextureDeclaration>& textureDeclarations = compilationResult.GetTextureDeclarations();

		for (const Mosaic::MosaicShaderWriter::TextureDeclaration& texture : textureDeclarations)
		{
			materialAsset->GetRenderMaterial()->AddTexture(texture.index, texture.name);
		}

		// Set textures
		// #TODO_Ivar: This is a temporary way of settings the textures.
		const auto& nodes = materialAsset->GetMaterialGraph()->GetMosaicGraph().GetUnderlyingGraph().GetNodes();
		for (const auto& node : nodes)
		{
			if (node.nodeData->GetGUID() == MosaicNodes::SampleTextureNode::GetStaticGUID())
			{
				Ref<MosaicNodes::SampleTextureNode> sampleTextureNode = std::reinterpret_pointer_cast<MosaicNodes::SampleTextureNode>(node.nodeData);
				const auto textureInfo = sampleTextureNode->GetTextureInfo();
			
				RefPtr<RHI::Image> image;

				AssetReference<Texture2D> texture = g_assetManager->GetAssetImmediately<Texture2D>(textureInfo.textureHandle);
				if (texture && texture->IsValid())
				{
					image = texture->GetImage();
				}
				else
				{
					image = Renderer::GetDefaultResources().white1x1;
				}
				

				materialAsset->GetRenderMaterial()->SetTexture(textureInfo.textureIndex, RenderTexture(image));
			}
		}

		// Create new pipeline based on compiled shader
		materialAsset->GetRenderMaterial()->Invalidate(outShaderPath);

		if (MaterialCompilerSubSystem* compilerSubSystem = SubSystemManager::GetSubSystem<MaterialCompilerSubSystem>(); compilerSubSystem != nullptr)
		{
			compilerSubSystem->GetMaterialCompiledDelegate().ExecuteIfBound(materialAsset->GetAssetHandle());
		}

		VT_LOGC(Trace, LogMaterialCompiler, "Compiled material {} in {} seconds!", materialAsset->GetAssetName(), timer.GetTime<Time::Seconds>());
	}
}
