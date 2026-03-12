#include "rhipch.h"

#include "RHIModule/Buffers/BufferView.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI 
{
	RefPtr<BufferView> BufferView::Create(const BufferViewDesc& specification, RawPtr<Buffer> buffer)
	{
		return RHIModule::GetInstance().CreateBufferView(specification, buffer);
	}

	RefPtr<BufferView> BufferView::Create(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer)
	{
		return RHIModule::GetInstance().CreateBufferView(specification, buffer);
	}
}
