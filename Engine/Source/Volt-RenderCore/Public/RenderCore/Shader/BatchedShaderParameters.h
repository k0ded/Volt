#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Shader/ShaderCommon.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Allocators/LinearAllocator.h>
#include <CoreUtilities/Allocators//InlineAllocator.h>
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
		LinearAllocator<MaxBatchedShaderParameterSize> m_allocator;
		Vector<DestructorHelper> m_destructors;
	};

	struct BatchedShaderParameter
	{
		BatchedShaderParameter(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType)
			: bindingName(inBindingName), resourceType(inResourceType)
		{ }

		const StringHash bindingName;
		const RHI::ShaderResourceType resourceType;
	};

	struct BatchedBufferShaderParameter : public BatchedShaderParameter
	{
		BatchedBufferShaderParameter(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, RefPtr<RHI::BufferView> inBufferView)
			: BatchedShaderParameter(inBindingName, inResourceType), bufferView(inBufferView)
		{ }

		RefPtr<RHI::BufferView> bufferView;
	};

	struct BatchedTextureShaderParameter : public BatchedShaderParameter
	{
		BatchedTextureShaderParameter(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, RefPtr<RHI::ImageView> inImageView)
			: BatchedShaderParameter(inBindingName, inResourceType), imageView(inImageView)
		{ }

		RefPtr<RHI::ImageView> imageView;
	};

	struct BatchedSamplerShaderParameter : public BatchedShaderParameter
	{
		BatchedSamplerShaderParameter(const StringHash inBindingName, const RHI::ShaderResourceType inResourceType, RefPtr<RHI::SamplerState> inSampler)
			: BatchedShaderParameter(inBindingName, inResourceType), sampler(inSampler)
		{
		}

		RefPtr<RHI::SamplerState> sampler;
	};

	class VTRC_API BatchedShaderParameters
	{
	public:
		void AddBufferParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::BufferView> bufferView);
		void AddTextureParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::ImageView> imageView);
		void AddSamplerParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::SamplerState> sampler);
		void BindParametersToDescriptorTable(const Vector<RHI::ShaderParameterMap>& shaderParameterMaps, RefPtr<RHI::DescriptorTable> descriptorTable) const;

	private:
		inline static constexpr size_t NumMaxShaderParameters = 64;

		Vector<BatchedShaderParameter*, InlineAllocator<NumMaxShaderParameters>> m_parameters;
		BatchedShaderParameterAllocator m_allocator;
	};
}
