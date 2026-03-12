#include "rhipch.h"

#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<CommandBuffer> CommandBuffer::Create(QueueType queueType)
	{
		return RHIModule::GetInstance().CreateCommandBuffer(queueType);
	}

	RefPtr<CommandBuffer> CommandBuffer::Create()
	{
		return RHIModule::GetInstance().CreateCommandBuffer(QueueType::Graphics);
	}

	RefPtr<CommandBuffer> CommandBuffer::CreateSecondary(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration)
	{
		return RHIModule::GetInstance().CreateSecondaryCommandBuffer(renderingAttachmentDeclaration);
	}
}
