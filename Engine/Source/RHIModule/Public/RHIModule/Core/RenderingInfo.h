#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Images/ImageView.h"

namespace Volt::RHI
{
	struct AttachmentInfo
	{
		union
		{
			float float32[4];
			int32_t int32[4];
			uint32_t uint32[4];

		} clearColor;

		IntRef<ImageView> view;
		ClearMode clearMode;

		inline void SetClearColor(float r, float g, float b, float a)
		{
			clearColor.float32[0] = r;
			clearColor.float32[1] = g;
			clearColor.float32[2] = b;
			clearColor.float32[3] = a;
		}

		inline void SetClearColor(int32_t r, int32_t g, int32_t b, int32_t a)
		{
			clearColor.int32[0] = r;
			clearColor.int32[1] = g;
			clearColor.int32[2] = b;
			clearColor.int32[3] = a;
		}

		inline void SetClearColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
		{
			clearColor.uint32[0] = r;
			clearColor.uint32[1] = g;
			clearColor.uint32[2] = b;
			clearColor.uint32[3] = a;
		}
	};

	struct RenderingInfo
	{
		InlineVector<AttachmentInfo, MAX_COLOR_ATTACHMENT_COUNT> colorAttachments;
		AttachmentInfo depthAttachmentInfo{};

		Rect2D renderArea{};
		uint32_t layerCount = 1;
	};

	struct RenderingAttachmentDeclaration
	{
		InlineVector<PixelFormat, MAX_COLOR_ATTACHMENT_COUNT> colorAttachmentFormats;
		PixelFormat depthAttachmentFormat = PixelFormat::UNDEFINED;
	};
}
