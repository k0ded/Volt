#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader2.h>
#include <RHIModule/Descriptors/ResourceHandle.h>

#include <CoreUtilities/Containers/VectorVariants.h>

#include <filesystem>

namespace Volt
{
	class RenderTexture
	{
	public:
		RenderTexture() = default;
		RenderTexture(ResourceHandle handle)
			: m_resourceHandle(handle)
		{ }

		VT_INLINE void SetResource(ResourceHandle handle) { m_resourceHandle = handle; }
		VT_NODISCARD VT_INLINE ResourceHandle GetResource() const { return m_resourceHandle; }
		
		VT_NODISCARD bool IsValid() const
		{
			return m_resourceHandle != Resource::Invalid;
		}

	private:
		ResourceHandle m_resourceHandle = Resource::Invalid;
	};

	class VTR_API RenderMaterial
	{
	public:
		RenderMaterial(const std::string& name);
		RenderMaterial(const std::string& name, RefPtr<RHI::Shader2> shader);

		VT_NODISCARD VT_INLINE const PagedVector<RenderTexture>& GetTextures() const { return m_textures; }

		void SetTexture(uint32_t index, RenderTexture resource);
		void SetTextures(const PagedVector<RenderTexture>& textures);

		bool DoMaterialRequireUpdate() const;
		void ClearStatus();

		VT_NODISCARD VT_INLINE size_t GetHash() const { return m_hash; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return m_name; }
		VT_NODISCARD VT_INLINE RefPtr<RHI::ComputePipeline> GetPipeline() const { return m_pipeline; }

	private:
		friend class MaterialCompiler;

		void Invalidate(const std::filesystem::path& shaderFilepath);
		void GenerateHash();

		PagedVector<RenderTexture> m_textures;

		RefPtr<RHI::ComputePipeline> m_pipeline;
		RefPtr<RHI::Shader2> m_shader;

		std::string m_name;
		size_t m_hash = 0;
		bool m_isDirty = true;
	};
}
