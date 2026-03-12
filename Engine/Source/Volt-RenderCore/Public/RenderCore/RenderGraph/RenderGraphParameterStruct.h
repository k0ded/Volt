#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

namespace Volt
{
	class ShaderParameterMetadataDescription;

	class RenderGraphParameterDesc
	{
	public:
		VTRC_API RenderGraphParameterDesc(
			uint8_t* dataPtr, 
			StringHash parameterNameHash, 
			size_t size,
			ShaderParameterType shaderParameterType, 
			RGResourceAccess resourceAccessType
		);

		template<typename T>
		T GetAs();

		VT_INLINE ShaderParameterType GetType() const { return m_shaderParameterType; }
		VT_INLINE RGResourceAccess GetAccess() const { return m_resourceAccessType; }
		VT_INLINE StringHash GetParameterNameHash() const { return m_parameterNameHash; }
		VT_INLINE size_t GetSize() const { return m_size; }
		VT_INLINE uint8_t* GetData() const { return m_dataPtr; }

	private:
		uint8_t* m_dataPtr;
		StringHash m_parameterNameHash;
		size_t m_size;
		ShaderParameterType m_shaderParameterType;
		RGResourceAccess m_resourceAccessType;
	};

	class RenderGraphParameterStruct
	{
	public:
		template<typename T>
		RenderGraphParameterStruct(const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata)
			: m_shaderParameters(reinterpret_cast<const uint8_t*>(shaderParameters)),
			m_shaderParameterMetadata(shaderParameterMetadata)
		{}

		template<typename FuncType>
		void EnumerateParameters(FuncType&& func);

	private:
		const uint8_t* m_shaderParameters;
		const ShaderParameterMetadataDescription* m_shaderParameterMetadata;
	};
}

#include "RenderCore/RenderGraph/RenderGraphParameterStruct.inl"
