#include "rhipch.h"

#include "RHIModule/Images/ImageUtility.h"
#include "RHIModule/Images/Image.h"
#include "RHIModule/Buffers/Buffer.h"
#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"
#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Core/ResourceStateTracker.h"

namespace Volt::RHI::ImageUtility
{
	DataBuffer ReadbackPixel(RefPtr<RHI::Image> image, uint32_t pixelX, uint32_t pixelY, uint32_t pixelZ)
	{
		const ImageDesc& imageDesc = image->GetDesc();

		const uint64_t formatByteSize = Utility::GetByteSizePerPixelFromFormat(imageDesc.format) * imageDesc.layers;

		BufferDesc stagingDesc{};
		stagingDesc.numElements = 1;
		stagingDesc.elementSize = formatByteSize;
		stagingDesc.usage = BufferUsage::TransferDst;
		stagingDesc.memoryUsage = MemoryUsage::GPUToCPU;
		stagingDesc.debugName = "Staging Alloc";

		RefPtr<RHI::Buffer> stagingBuffer = RHI::Buffer::Create(stagingDesc);

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image, 0);

		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();

		{
			ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcAccess = currentState.access;
			barrier.imageBarrier().srcStage = currentState.stage;
			barrier.imageBarrier().srcLayout = currentState.layout;
			barrier.imageBarrier().dstAccess = BarrierAccess::CopySource;
			barrier.imageBarrier().dstStage = BarrierStage::Copy;
			barrier.imageBarrier().dstLayout = ImageLayout::CopySource;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->CopyImageToBuffer(image, stagingBuffer, 0, 1, 1, 1, pixelX, pixelY, pixelZ, 0);

		{
			ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().dstAccess = currentState.access;
			barrier.imageBarrier().dstStage = currentState.stage;
			barrier.imageBarrier().dstLayout = currentState.layout;
			barrier.imageBarrier().srcAccess = BarrierAccess::CopySource;
			barrier.imageBarrier().srcStage = BarrierStage::Copy;
			barrier.imageBarrier().srcLayout = ImageLayout::CopySource;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();
		CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);

		DataBuffer result;
		result.Allocate(formatByteSize);

		void* mappedPtr = stagingBuffer->Map<void>();
		result.Copy(mappedPtr, formatByteSize);
		stagingBuffer->Unmap();

		return result;
	}
}
