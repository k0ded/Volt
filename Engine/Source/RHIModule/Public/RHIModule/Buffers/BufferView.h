#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	class RHIResource;

	struct BufferViewDesc
	{
		size_t offset = 0;
		size_t size = std::numeric_limits<size_t>::max();

		// Used for texel buffers.
		RHI::PixelFormat bufferFormat = RHI::PixelFormat::UNDEFINED;

		// #TODO_Ivar: Move out from the desc.
		RHIResource* bufferResource = nullptr;
	};

	class VTRHI_API BufferView : public ArenaRHIInterface
	{
	public:
		~BufferView() override = default;

		static RefPtr<BufferView> Create(const BufferViewDesc& specification);
		virtual const uint64_t GetDeviceAddress() const = 0;
		virtual bool IsTexelBufferView() const = 0;

	protected:
		BufferView() = default;
	};
}
