#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Shader/ShaderCommon.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{
	class VTRC_API BatchedShaderParameterAllocator
	{
	public:
		template<typename T, typename... Args>
		T* Allocate(Args&&... args)
		{
			void* allocation = m_allocator.Allocate(sizeof(T));
			return new (allocation) T(std::forward<Args>(args)...);
		}

	private:
		inline static constexpr size_t MaxBatchedShaderParameterSize = 1024;
		LinearAllocator<MaxBatchedShaderParameterSize> m_allocator;
	};

	struct BatchedShaderParameter
	{
		BatchedShaderParameter(const RHI::ShaderResourceBinding* inResourceBinding)
			: resourceBinding(inResourceBinding)
		{ }

		const RHI::ShaderResourceBinding* const resourceBinding;
	};

	struct BatchedBufferShaderParameter : public BatchedShaderParameter
	{
		BatchedBufferShaderParameter(const RHI::ShaderResourceBinding* inResourceBinding, RefPtr<RHI::BufferView> inBufferView)
			: BatchedShaderParameter(inResourceBinding), bufferView(inBufferView)
		{ }

		RefPtr<RHI::BufferView> bufferView;
	};

	struct BatchedTextureShaderParameter : public BatchedShaderParameter
	{
		BatchedTextureShaderParameter(const RHI::ShaderResourceBinding* inResourceBinding, RefPtr<RHI::ImageView> inImageView)
			: BatchedShaderParameter(inResourceBinding), imageView(inImageView)
		{ }

		RefPtr<RHI::ImageView> imageView;
	};

	class VTRC_API BatchedShaderParameters
	{
	public:
		void AddBufferParameter(const RHI::ShaderResourceBinding* resourceBinding, RefPtr<RHI::BufferView> bufferView);
		void AddTextureParameter(const RHI::ShaderResourceBinding* resourceBinding, RefPtr<RHI::ImageView> imageView);
		void BindParametersToDescriptorTable(RefPtr<RHI::DescriptorTable> descriptorTable) const;

	private:
		// #TODO_Ivar: Switch to inline allocator
		PagedVector<BatchedShaderParameter*> m_parameters;
		BatchedShaderParameterAllocator m_allocator;
	};
}
