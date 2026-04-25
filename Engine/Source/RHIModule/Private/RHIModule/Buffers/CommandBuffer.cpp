#include "rhipch.h"

#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<CommandBuffer> CommandBuffer::Create(QueueType queueType) 
	{
		return RHIModule::GetInstance().CreateCommandBuffer(queueType);
	}

	IntRef<CommandBuffer> CommandBuffer::Create()
	{
		return RHIModule::GetInstance().CreateCommandBuffer(QueueType::Graphics);
	}

	IntRef<CommandBuffer> CommandBuffer::CreateSecondary(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration)
	{
		return RHIModule::GetInstance().CreateSecondaryCommandBuffer(renderingAttachmentDeclaration);
	}
}
