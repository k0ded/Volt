#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/RenderContext.h"

#include <RHIModule/Shader/ShaderCommon.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Buffers/UniformBuffer.h>

#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>
#include <CoreUtilities/Containers/ArrayView.h>
#include <CoreUtilities/DestructorHelper.h>

namespace Volt
{
	namespace RHI
	{
		class ShaderParameterMap;
	}

	class VTRC_API BatchedShaderParameterAllocator
	{
	public:
		BatchedShaderParameterAllocator();
		~BatchedShaderParameterAllocator();

		template<typename T, typename... Args>
		T* Allocate(Args&&... args)
		{
			void* allocation = m_allocator.Allocate(sizeof(T));
			T* ptr = new (allocation) T(std::forward<Args>(args)...);
		
			m_destructors.emplace_back() = DestructorHelper::Create<T>(ptr);
			return ptr;
		}

	private:
		inline static constexpr size_t MaxBatchedShaderParameterSize = 1024;
		FixedSizeLinearAllocator<> m_allocator;
		GlobalMemoryStackVector<DestructorHelper> m_destructors;
	};

	struct BatchedShaderBinding
	{
		BatchedShaderBinding(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType)
			: bindingName(inBindingName), resourceType(inResourceType)
		{ }

		const StringHash bindingName;
		const RHI::ShaderResourceType resourceType;
	};

	struct BatchedBufferShaderBinding : public BatchedShaderBinding
	{
		BatchedBufferShaderBinding(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, IntRef<RHI::BufferView> inBufferView)
			: BatchedShaderBinding(inBindingName, inResourceType), bufferView(inBufferView)
		{ }

		IntRef<RHI::BufferView> bufferView;
	};

	struct BatchedTextureShaderBinding : public BatchedShaderBinding
	{
		BatchedTextureShaderBinding(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, IntRef<RHI::ImageView> inImageView)
			: BatchedShaderBinding(inBindingName, inResourceType), imageView(inImageView)
		{ }

		IntRef<RHI::ImageView> imageView;
	};

	struct BatchedSamplerShaderBinding : public BatchedShaderBinding
	{
		BatchedSamplerShaderBinding(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, IntRef<RHI::SamplerState> inSampler)
			: BatchedShaderBinding(inBindingName, inResourceType), sampler(inSampler)
		{
		}

		IntRef<RHI::SamplerState> sampler;
	};

	struct BatchedShaderParameter
	{
		BatchedShaderParameter(const StringHash inParameterName, const void* inData, const size_t inSize)
			: parameterName(inParameterName), data(inData), size(inSize)
		{ }

		const StringHash parameterName;
		const void* data;
		const size_t size;
	};

	class VTRC_API BatchedShaderParameters
	{
	public:
		inline static constexpr size_t NumMaxShaderBindings = 64;
		inline static constexpr size_t NumMaxShaderParameters = 16;

		void AddBufferParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::BufferView> bufferView);
		void AddTextureParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::ImageView> imageView);
		void AddSamplerParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::SamplerState> sampler);
		void AddShaderParameter(const StringHash parameterName, const void* data, const size_t size);

		VT_INLINE ArrayView<BatchedShaderParameter*> GetShaderParameters() const { return m_parameters; }
		VT_INLINE ArrayView<BatchedShaderBinding*> GetShaderBindings() const { return m_bindings; }

		void PopulateShaderParameterUniformBuffers(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, Vector<RenderContext::PerStageShaderParameters, InlineAllocator<8>>& outShaderParameters);
		void BindToShaderBindings(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings);

	private:
		void BindToShaderBindingsBindlessInternal(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings);
		void BindToShaderBindingsInternal(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings);

		void PopulateShaderParameterUniformBuffersBindless(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, Vector<RenderContext::PerStageShaderParameters, InlineAllocator<8>>& outShaderParameters);

		InlineVector<BatchedShaderBinding*, NumMaxShaderBindings> m_bindings;
		InlineVector<BatchedShaderParameter*, NumMaxShaderParameters> m_parameters;
		BatchedShaderParameterAllocator m_allocator;
	};
}
