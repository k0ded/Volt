#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	class Buffer;
	class UniformBuffer;

	struct BufferViewDesc
	{
		uint64_t offset = 0;
		uint64_t size = std::numeric_limits<uint64_t>::max();

		// Used for texel buffers.
		RHI::PixelFormat bufferFormat = RHI::PixelFormat::UNDEFINED;
	};

	class VTRHI_API BufferView : public ArenaRHIInterface
	{
	public:
		~BufferView() override = default;

		static IntRef<BufferView> Create(const BufferViewDesc& desc, RawPtr<Buffer> buffer);
		static IntRef<BufferView> Create(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer);
		virtual const uint64_t GetDeviceAddress() const = 0;
		virtual bool IsTexelBufferView() const = 0;
		virtual const BufferViewDesc& GetDesc() const = 0;

	protected:
		BufferView() = default;
	};
}
