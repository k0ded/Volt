#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Material/MaterialShaderMap.h"
#include "Volt-Renderer/Material/CompiledMaterialShaders.h"
#include "Volt-Renderer/Material/MaterialShader.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Descriptors/ResourceHandle.h>
#include <RHIModule/Images/Image.h>

#include <filesystem>

namespace Volt
{
	namespace RHI
	{
		class ShaderBindingMap;
	}

	class RenderTexture
	{
	public:
		RenderTexture() = default;
		RenderTexture(RefPtr<RHI::Image> image)
			: m_image(image)
		{ }

		VT_INLINE void SetResource(RefPtr<RHI::Image> image) { m_image = image; }
		VT_NODISCARD VT_INLINE RefPtr<RHI::Image> GetResource() const { return m_image; }
		
		VT_NODISCARD bool IsValid() const
		{
			return m_image != nullptr;
		}

	private:
		RefPtr<RHI::Image> m_image;
	};

	class VTR_API RenderMaterial
	{
	public:
		struct TextureInfo
		{
			StringHash bindingName;
			RenderTexture texture;
		};

		using TexturesMap = Map<uint32_t, TextureInfo>;

		RenderMaterial(const std::string& name);

		void SetTexture(uint32_t index, RenderTexture resource);

		bool DoMaterialRequireUpdate() const;
		void UpdateTextures();
		void ClearStatus();

		// Note: This function may be called from any thread during rendering.
		void BindToShaderBindingMap(RHI::ShaderBindingMap& shaderBindingMap, RefPtr<RHI::RenderPipeline> renderPipeline) const;

		VT_INLINE void SetMaterialBlendMode(MaterialBlendMode materialBlendMode) { m_materialBlendMode = materialBlendMode; }
		VT_INLINE void SetIsDoubleSided(bool isDoubleSided) { m_isDoubleSided = isDoubleSided; }

		VT_NODISCARD VT_INLINE const TexturesMap& GetTextures() const { return m_textures; }
		VT_NODISCARD VT_INLINE size_t GetHash() const { return m_hash; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return m_name; }
		VT_NODISCARD VT_INLINE MaterialBlendMode GetMaterialBlendMode() const { return m_materialBlendMode; }
		VT_NODISCARD VT_INLINE bool GetIsDoubleSided() const { return m_isDoubleSided; }

		template<typename T>
		VT_NODISCARD RefPtr<RHI::Shader> GetPixelShader() 
		{ 
			return m_shaderMap.GetShader<T>(); 
		}

		template<typename T>
		VT_NODISCARD RefPtr<RHI::Shader> GetPixelShader(const typename T::PermutationVector& permutationVector)
		{
			return m_shaderMap.GetShader<T>(permutationVector); 
		}

	private:
		friend class MaterialCompiler;

		void Invalidate(CompiledMaterialShaders&& compiledMaterialShaders);
		void GenerateHash();

		TexturesMap m_textures;
		// Textures used while rendering.
		TexturesMap m_renderableTextures;

		MaterialBlendMode m_materialBlendMode = MaterialBlendMode::Opaque;
		bool m_isDoubleSided = false;
		MaterialShaderMap m_shaderMap;

		std::string m_name;
		size_t m_hash = 0;
		bool m_isDirty = true;
	};
}
