#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Images/Image.h"
#include "RHIModule/Memory/TransientHeap.h"

/*
* This is a specialization of Image, which is supposed to be used for frame transient images.
* It does not own it's memory, but is instead assigned it.
* It does own it's image handle.
*/

namespace Volt::RHI
{
	class TransientImage : public Image
	{
	public: 
		~TransientImage() override = default;

		virtual void BindMemory(RefPtr<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset) = 0;

		VTRHI_API static RefPtr<TransientImage> Create(const ImageDesc& desc);

	protected:
		TransientImage() = default;
	};
}
