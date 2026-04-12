#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Material/MaterialShaderMap.h"
#include "Volt-Renderer/Material/CompiledMaterialShaders.h"
#include "Volt-Renderer/Material/MaterialShader.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
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
		RenderTexture(IntRef<RHI::Image> image)
			: m_image(image)
		{ }

		VT_INLINE void SetResource(IntRef<RHI::Image> image) { m_image = image; }
		VT_NODISCARD VT_INLINE IntRef<RHI::Image> GetResource() const { return m_image; }
		
		VT_NODISCARD bool IsValid() const
		{
			return m_image != nullptr;
		}

	private:
		IntRef<RHI::Image> m_image;
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

		RenderMaterial(const String& name);

		void SetTexture(uint32_t index, RenderTexture resource);

		bool DoMaterialRequireUpdate() const;
		void UpdateTextures();
		void ClearStatus();

		// Note: This function may be called from any thread during rendering.
		void BindToShaderBindingMap(RHI::ShaderBindingMap& shaderBindingMap, IntRef<RHI::RenderPipeline> renderPipeline) const;

		VT_INLINE void SetMaterialBlendMode(MaterialBlendMode materialBlendMode) { m_materialBlendMode = materialBlendMode; }
		VT_INLINE void SetIsDoubleSided(bool isDoubleSided) { m_isDoubleSided = isDoubleSided; }

		VT_NODISCARD VT_INLINE const TexturesMap& GetTextures() const { return m_textures; }
		VT_NODISCARD VT_INLINE size_t GetHash() const { return m_hash; }
		VT_NODISCARD VT_INLINE const String& GetName() const { return m_name; }
		VT_NODISCARD VT_INLINE MaterialBlendMode GetMaterialBlendMode() const { return m_materialBlendMode; }
		VT_NODISCARD VT_INLINE bool GetIsDoubleSided() const { return m_isDoubleSided; }

		template<typename T>
		VT_NODISCARD IntRef<RHI::Shader> GetPixelShader() 
		{ 
			return m_shaderMap.GetShader<T>(); 
		}

		template<typename T>
		VT_NODISCARD IntRef<RHI::Shader> GetPixelShader(const typename T::PermutationVector& permutationVector)
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

		String m_name;
		size_t m_hash = 0;
		bool m_isDirty = true;
	};
}
