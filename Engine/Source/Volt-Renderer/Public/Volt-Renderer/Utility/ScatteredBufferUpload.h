#pragma once

#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph2/RenderGraphUtils.h>
#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

#include <atomic>

namespace Volt
{
	struct ScatterUploadCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(ScatterUploadCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWDstBuffer)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, SrcBuffer)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, ScatterIndices)
			SHADER_PARAMETER(uint32_t, TypeSizeInUINT)
			SHADER_PARAMETER(uint32_t, CopyCount)
		END_SHADER_PARAMETER_STRUCT()
	};

	namespace RHI
	{
		class StorageBuffer;
	}

	template<typename T>
	concept IsTrivial = std::is_trivially_copyable<T>::value && sizeof(T) % 4 == 0;

	template<IsTrivial T>
	class ScatteredBufferUpload
	{
	public:
		ScatteredBufferUpload(const size_t uploadCount);

		T& AddUploadItem(size_t bufferIndex);
		void UploadTo(RenderGraph2& renderGraph, RefPtr<RHI::StorageBuffer> dstBuffer);

	private:
		void UploadToInternal(RenderGraph2& renderGraph, RefPtr<RHI::StorageBuffer> dstBuffer);

		Vector<T> m_data;
		Vector<uint32_t> m_dataIndices;
		std::atomic<uint32_t> m_currentIndex = 0;
	};

	template<IsTrivial T>
	inline ScatteredBufferUpload<T>::ScatteredBufferUpload(const size_t uploadCount)
	{
		m_data.resize(uploadCount);
		m_dataIndices.resize(uploadCount);
	}

	template<IsTrivial T>
	inline T& ScatteredBufferUpload<T>::AddUploadItem(size_t bufferIndex)
	{
		const uint32_t index = m_currentIndex++;

		m_dataIndices[index] = static_cast<uint32_t>(bufferIndex);
		return m_data[index];
	}

	template<IsTrivial T>
	inline void ScatteredBufferUpload<T>::UploadTo(RenderGraph2& renderGraph, RefPtr<RHI::StorageBuffer> dstBuffer)
	{
		UploadToInternal(renderGraph, dstBuffer);
	}

	template<IsTrivial T>
	inline void ScatteredBufferUpload<T>::UploadToInternal(RenderGraph2& renderGraph, RefPtr<RHI::StorageBuffer> rhiDstBuffer)
	{
		if (m_currentIndex == 0)
		{
			return;
		}

		constexpr uint32_t sizeInUINT = static_cast<uint32_t>(sizeof(T) / sizeof(uint32_t));

		RGBufferRef srcBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<T>(m_data.size(), RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "Src Data"));
		RGBufferRef indicesBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(m_data.size(), RHI::BufferUsage::TexelBuffer, RHI::MemoryUsage::CPUToGPU, "Scatter Indices"));
		RGBufferRef dstBuffer = renderGraph.RegisterExternalBuffer(rhiDstBuffer);

		AddMappedBufferUpload(renderGraph, srcBuffer, m_data.data(), sizeof(T) * m_data.size());
		AddMappedBufferUpload(renderGraph, indicesBuffer, m_dataIndices.data(), sizeof(uint32_t) * m_dataIndices.size());

		ScatterUploadCS::Parameters* passParameters = renderGraph.AllocParameters<ScatterUploadCS::Parameters>();
		passParameters->RWDstBuffer = renderGraph.CreateUAV(dstBuffer);
		passParameters->SrcBuffer = renderGraph.CreateSRV(srcBuffer);
		passParameters->ScatterIndices = renderGraph.CreateSRV(indicesBuffer);
		passParameters->TypeSizeInUINT = static_cast<uint32_t>(sizeInUINT);
		passParameters->CopyCount = static_cast<uint32_t>(m_data.size());

		const uint32_t groupSize = Math::DivideRoundUp(static_cast<uint32_t>(sizeInUINT * passParameters->CopyCount), 64u);

		auto shader = ShaderMap::Get2<ScatterUploadCS>();
		ComputeShaderUtils::AddPass<ScatterUploadCS>(renderGraph,
			"Scatter Buffer Upload",
			shader,
			passParameters,
			{ groupSize, 1, 1 });
	}
}
