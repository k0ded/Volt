#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompiler.h"
#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <Volt-Renderer/Material/RenderMaterial.h>
#include <Volt-Renderer/Material/CompiledMaterialShaders.h>
#include <Volt-Renderer/Material/MaterialShaderRegistry.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Texture/Texture2D.h>

#include <Volt-Core/Project/ProjectManager.h>
#include <Volt-FileSystem/FileUtility.h>

#include <AssetSystem/AssetManager.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogMaterialCompiler);

namespace Volt
{
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

	struct IncludeDirective
	{
		std::filesystem::path filepath;
		size_t begin;
		size_t end;
	};

	inline Vector<IncludeDirective> FindIncludeDirectives(std::string_view shaderString)
	{
		Vector<IncludeDirective> includeDirectives;

		size_t offset = shaderString.find("#include", 0);
		while (offset != std::string_view::npos)
		{
			// Move to first "
			size_t firstQuote = shaderString.find_first_of('"', offset);

			IncludeDirective& includeDirective = includeDirectives.emplace_back();
			includeDirective.begin = offset;
			includeDirective.end = shaderString.find_first_of('"', firstQuote + 1);

			std::string_view filepathString = shaderString.substr(firstQuote + 1, includeDirective.end - firstQuote - 1);
		
			includeDirective.filepath = std::filesystem::path(filepathString);

			offset = shaderString.find("#include", includeDirective.end);
		}

		return includeDirectives;
	}

	void MaterialCompiler::CompileMaterial(AssetReference<MaterialAsset> materialAsset)
	{
		VT_PROFILE_FUNCTION();

		VT_LOGC(Trace, LogMaterialCompiler, "Started compilation of material {}", materialAsset->GetAssetName());

		ScopedTimer timer;

		MaterialCompilerSubSystem* compilerSubSystem = SubSystemManager::GetSubSystem<MaterialCompilerSubSystem>();
		VT_ENSURE_MSG(compilerSubSystem != nullptr, "The MaterialCompilerSubSystem must exist!");

		const Mosaic::MosaicShaderWriter compilationResult = materialAsset->GetMaterialGraph()->GetMosaicGraph().Compile();

		// #TODO_Ivar: Temporary barrier
		if (compilationResult.GetAsString().empty())
		{
			return;
		}

		const std::filesystem::path materialShaderFilepath = "Material/MaterialShader.hlsli";
		const std::string& materialShaderFileContents = compilerSubSystem->GetMaterialShaderFileContents();

		CompiledMaterialShaders result;

		// Compile for each material shader type.
		for (const auto& [typeIndex, registeredShader] : MaterialShaderRegistry::Get().GetRegisteredShaders())
		{
			const std::filesystem::path absoluteFilepath = ProjectManager::GetEngineRootDirectory() / registeredShader.baseFilepath;

			std::string materialShaderString;
			FileUtility::ReadStringFromFile(absoluteFilepath, materialShaderString);

			// Remove the MaterialShader.hlsli include if it exists.
			Vector<IncludeDirective> includeDirectives = FindIncludeDirectives(materialShaderString);
		
			bool replaced = false;
			for (const IncludeDirective& includeDirective : includeDirectives)
			{
				if (includeDirective.filepath == materialShaderFilepath)
				{
					materialShaderString.replace(includeDirective.begin, includeDirective.end - includeDirective.begin + 1, materialShaderFileContents);
					replaced = true;
					break;
				}
			}

			// Include didn't exist in shader, we'll insert it at the top.
			if (!replaced)
			{
				materialShaderString.insert(0, materialShaderFileContents);
			}

			// Insert material specific code.
			InsertTextureDeclarations(materialShaderString, compilationResult);
			InsertMaterialEvaluation(materialShaderString, compilationResult);

			result.Add(typeIndex, std::move(materialShaderString), registeredShader.entryPoint);
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

				IntRef<RHI::Image> image;

				if (textureInfo.textureHandle != Asset::Null())
				{
					AssetReference<Texture2D> texture = g_assetManager->GetAssetImmediately<Texture2D>(textureInfo.textureHandle);
					if (texture && texture->IsValid())
					{
						image = texture->GetImage();
					}
				}

				if (image == nullptr)
				{
					image = Renderer::GetDefaultResources().white1x1;
				}

				materialAsset->GetRenderMaterial()->SetTexture(textureInfo.textureIndex, RenderTexture(image));
			}
		}

		materialAsset->GetRenderMaterial()->Invalidate(std::move(result));
		compilerSubSystem->GetMaterialCompiledDelegate().ExecuteIfBound(materialAsset->GetAssetHandle());

		VT_LOGC(Trace, LogMaterialCompiler, "Compiled material {} in {} seconds!", materialAsset->GetAssetName(), timer.GetTime<Time::Seconds>());
	}
}
