#pragma once

#include "RHIModule/Core/RHIResource.h"
#include "RHIModule/Buffers/BufferView.h"

namespace Volt::RHI
{
	class BufferView;

	struct UniformBufferDesc
	{
		uint32_t size;
		std::string debugName;
	};

	class UniformBuffer : public RHIResource
	{ 
	public:
		~UniformBuffer() override = default;

		virtual IntRef<BufferView> GetView(const BufferViewDesc& desc = {}) = 0;
		virtual uint64_t GetSize() const = 0;
		virtual void Unmap() = 0;

		template<typename T> inline T* Map();

		VTRHI_API static IntRef<UniformBuffer> Create(const UniformBufferDesc& uniformBufferDesc, const void* initialData = nullptr);

	protected:
		virtual void* MapInternal() = 0;

		UniformBuffer() = default;
	};

	template<typename T>
	inline T* UniformBuffer::Map()
	{
		return reinterpret_cast<T*>(MapInternal());
	}
}
