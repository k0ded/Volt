#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class DeviceQueue;
	class TransientHeap;
	class Image;

	struct ImageDesc;
	struct BufferDesc;

	class VTRHI_API GraphicsDevice : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(GraphicsDevice);
		~GraphicsDevice() override = default;

		virtual RefPtr<DeviceQueue> GetDeviceQueue(QueueType queueType) const = 0;

		virtual uint64_t GetMaxRequiredStagingBufferSizeForImage(RawPtr<Image> image) const = 0;
		virtual uint64_t GetRowPitchForWidth(RawPtr<Image> image, uint32_t width) const = 0;
		virtual MemoryRequirement GetImageMemoryRequirement(const ImageDesc& desc) const = 0;
		virtual MemoryRequirement GetBufferMemoryRequirement(const BufferDesc& desc) const = 0;

		static RefPtr<GraphicsDevice> Create(const GraphicsDeviceCreateInfo& deviceInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer);
	
	protected:
		GraphicsDevice();
	};
}
