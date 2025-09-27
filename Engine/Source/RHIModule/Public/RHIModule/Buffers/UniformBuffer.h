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

	class VTRHI_API UniformBuffer : public RHIResource
	{ 
	public:
		~UniformBuffer() override = default;

		virtual RefPtr<BufferView> GetView(const BufferViewDesc& desc = {}) = 0;
		virtual const uint32_t GetSize() const = 0;
		virtual void SetData(const void* data, const uint32_t size) = 0;
		virtual void Unmap() = 0;

		// Note: The index parameter is used to map at a certain index offset in the uniform buffer
		template<typename T>
		inline T* Map(const uint32_t index = 0);

		template<typename T>
		inline void SetData(const T& data);

		// Note: Count is used to create a buffer of the correct size if using offsets into the uniform buffer
		static RefPtr<UniformBuffer> Create(const UniformBufferDesc& uniformBufferDesc, const void* initialData = nullptr);

	protected:
		virtual void* MapInternal(const uint32_t index) = 0;

		UniformBuffer() = default;
	};

	template<typename T>
	inline T* UniformBuffer::Map(const uint32_t index)
	{
		return reinterpret_cast<T*>(MapInternal(index));
	}

	template<typename T>
	inline void UniformBuffer::SetData(const T& data)
	{
		SetData(&data, sizeof(T));
	}
}
