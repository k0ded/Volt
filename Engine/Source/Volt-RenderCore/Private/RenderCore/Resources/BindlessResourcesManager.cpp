#include "rcpch.h"
#include "RenderCore/Resources/BindlessResourcesManager.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/RHIModule.h>

#include <RHIModule/RHIFeatures.h>

namespace Volt
{
	BindlessResourcesManager::BindlessResourcesManager()
	{
		VT_ASSERT_MSG(!s_instance, "Instance should be null!");
		s_instance = this;

		constexpr uint32_t framesInFlight = 3;
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable = RHI::BindlessDescriptorTable::Create(framesInFlight);
		}
	}

	BindlessResourcesManager::~BindlessResourcesManager()
	{
		m_bindlessDescriptorTable = nullptr;
		s_instance = nullptr;
	}

	ResourceHandle BindlessResourcesManager::RegisterBuffer(RawPtr<RHI::StorageBuffer> storageBuffer)
	{
		if (RHI::RHICanUseBindless())
		{
			return m_bindlessDescriptorTable->RegisterBuffer(storageBuffer);
		}

		return Resource::Invalid;
	}

	ResourceHandle BindlessResourcesManager::RegisterImageView(RawPtr<RHI::ImageView> imageView)
	{
		if (RHI::RHICanUseBindless())
		{
			return m_bindlessDescriptorTable->RegisterImageView(imageView);
		}

		return Resource::Invalid;
}

	ResourceHandle BindlessResourcesManager::RegisterSamplerState(RawPtr<RHI::SamplerState> samplerState)
	{
		if (RHI::RHICanUseBindless())
		{
			return m_bindlessDescriptorTable->RegisterSamplerState(samplerState);
		}

		return Resource::Invalid;
	}

	void BindlessResourcesManager::UnregisterResource(ResourceHandle handle)
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->UnregisterResource(handle);
		}
	}

	void BindlessResourcesManager::MarkResourceAsDirty(ResourceHandle handle)
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->MarkResourceAsDirty(handle);
		}
	}

	void BindlessResourcesManager::UnregisterSamplerState(ResourceHandle handle)
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->UnregisterSamplerState(handle);
		}
	}

	void BindlessResourcesManager::MarkSamplerStateAsDirty(ResourceHandle handle)
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->MarkSamplerStateAsDirty(handle);
		}
	}

	void BindlessResourcesManager::Update()
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->Update();
		}
	}

	void BindlessResourcesManager::PrepareForRender()
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessDescriptorTable->PrepareForRender();
		}
	}

	bool BindlessResourcesManager::IsResourceValid(ResourceHandle handle)
	{
		if (RHI::RHICanUseBindless())
		{
			return m_bindlessDescriptorTable->IsResourceValid(handle);
		}

		return false;
	}
}
