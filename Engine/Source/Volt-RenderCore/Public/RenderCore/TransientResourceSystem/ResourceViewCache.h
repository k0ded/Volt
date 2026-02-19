#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"

#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/Image.h>

#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Buffers/Buffer.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class RGRHIBufferResource;

	class TransientBufferViewCache
	{
	public:
		TransientBufferViewCache(RGRHIBufferResource* buffer);
		RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc);

	private:
		struct ViewPair
		{
			size_t hash;
			RefPtr<RHI::BufferView> view;
		};

		RGRHIBufferResource* m_buffer;
		InlineVector<ViewPair, 1> m_views;
	};

	class TransientImageViewCache
	{
	public:
		TransientImageViewCache(RGRHITextureResource* buffer);
		RefPtr<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc);

	private:
		struct ViewPair
		{
			size_t hash;
			RefPtr<RHI::ImageView> view;
		};

		RGRHITextureResource* m_texture;
		InlineVector<ViewPair, 1> m_views;
	};

	class TransientUniformBufferViewCache
	{
	public:
		TransientUniformBufferViewCache(RGRHIUniformBufferResource* buffer);
		RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc);

	private:
		struct ViewPair
		{
			size_t hash;
			RefPtr<RHI::BufferView> view;
		};

		RGRHIUniformBufferResource* m_buffer;
		InlineVector<ViewPair, 1> m_views;
	};
}
