#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Core/RHIResource.h>
#include <RHIModule/Descriptors/BindlessDescriptorTable.h>
#include <RHIModule/Descriptors/ResourceHandle.h>

namespace Volt
{
	namespace RHI
	{
		class StorageBuffer;
		class SamplerState;
	}

	class VTRC_API BindlessResourcesManager
	{
	public:
		BindlessResourcesManager();
		~BindlessResourcesManager();

		ResourceHandle RegisterBuffer(RawPtr<RHI::StorageBuffer> storageBuffer);
		ResourceHandle RegisterImageView(RawPtr<RHI::ImageView> image);
		ResourceHandle RegisterSamplerState(RawPtr<RHI::SamplerState> samplerState);

		void UnregisterResource(ResourceHandle handle);
		void MarkResourceAsDirty(ResourceHandle handle);

		void UnregisterSamplerState(ResourceHandle handle);
		void MarkSamplerStateAsDirty(ResourceHandle handle);

		void Update();
		void PrepareForRender();

		bool IsResourceValid(ResourceHandle handle);

		VT_NODISCARD VT_INLINE RefPtr<RHI::BindlessDescriptorTable> GetDescriptorTable() const { return m_bindlessDescriptorTable; }
		VT_NODISCARD VT_INLINE static BindlessResourcesManager& Get() { return *s_instance; }

	private:
		inline static BindlessResourcesManager* s_instance = nullptr;
		RefPtr<RHI::BindlessDescriptorTable> m_bindlessDescriptorTable;
	};
}
