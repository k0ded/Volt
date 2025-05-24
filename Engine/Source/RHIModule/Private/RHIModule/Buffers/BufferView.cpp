#include "rhipch.h"

#include "RHIModule/Buffers/BufferView.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI 
{
	RefPtr<BufferView> BufferView::Create(const BufferViewSpecification& specification)
	{
		return RHIModule::GetInstance().CreateBufferView(specification);
	}
}
