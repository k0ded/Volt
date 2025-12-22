#pragma once

#include "Volt-Renderer/Config.h"

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
		RenderMaterial(const std::string& name, RefPtr<RHI::Shader> shader);

		VT_NODISCARD VT_INLINE const TexturesMap& GetTextures() const { return m_textures; }

		void SetTexture(uint32_t index, RenderTexture resource);

		bool DoMaterialRequireUpdate() const;
		void UpdateTextures();
		void ClearStatus();

		// Note: This function may be called from any thread during rendering.
		void BindToShaderBindingMap(RHI::ShaderBindingMap& shaderBindingMap, RefPtr<RHI::RenderPipeline> renderPipeline) const;

		VT_NODISCARD VT_INLINE size_t GetHash() const { return m_hash; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return m_name; }
		VT_NODISCARD VT_INLINE RefPtr<RHI::Shader> GetPixelShader() const { return m_pixelShader; }

	private:
		friend class MaterialCompiler;

		void Invalidate(const std::filesystem::path& shaderFilepath);
		void GenerateHash();

		TexturesMap m_textures;
		// Textures used while rendering.
		TexturesMap m_renderableTextures;

		RefPtr<RHI::Shader> m_pixelShader;

		std::string m_name;
		size_t m_hash = 0;
		bool m_isDirty = true;
	};
}
