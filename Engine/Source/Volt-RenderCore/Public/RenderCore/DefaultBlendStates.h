#pragma once

#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt::DefaultBlendStates
{
	inline RHI::AttachmentBlendState Add()
	{
		RHI::AttachmentBlendState result;
		result.enabled = true;
		result.srcColorBlend = RHI::AttachmentBlendFactor::One;
		result.dstColorBlend = RHI::AttachmentBlendFactor::One;
		result.colorBlendOp = RHI::AttachmentBlendOp::Add;
		result.srcAlphaBlend = RHI::AttachmentBlendFactor::One;
		result.dstAlphaBlend = RHI::AttachmentBlendFactor::One;
		result.alphaBlendOp = RHI::AttachmentBlendOp::Add;

		return result;
	}

	inline RHI::AttachmentBlendState Alpha()
	{
		RHI::AttachmentBlendState result;
		result.enabled = true;
		result.srcColorBlend = RHI::AttachmentBlendFactor::SrcAlpha;
		result.dstColorBlend = RHI::AttachmentBlendFactor::OneMinusSrcAlpha;
		result.colorBlendOp = RHI::AttachmentBlendOp::Add;
		result.srcAlphaBlend = RHI::AttachmentBlendFactor::OneMinusDstAlpha;
		result.dstAlphaBlend = RHI::AttachmentBlendFactor::One;
		result.alphaBlendOp = RHI::AttachmentBlendOp::Add;

		return result;
	}

	inline RHI::AttachmentBlendState ZeroSrcColor()
	{
		RHI::AttachmentBlendState result;
		result.enabled = true;
		result.srcColorBlend = RHI::AttachmentBlendFactor::Zero;
		result.dstColorBlend = RHI::AttachmentBlendFactor::SrcColor;
		result.colorBlendOp = RHI::AttachmentBlendOp::Add;
		result.srcAlphaBlend = RHI::AttachmentBlendFactor::One;
		result.dstAlphaBlend = RHI::AttachmentBlendFactor::Zero;
		result.alphaBlendOp = RHI::AttachmentBlendOp::Add;

		return result;
	}

	inline RHI::AttachmentBlendState OneMinusSrcColor()
	{
		RHI::AttachmentBlendState result;
		result.enabled = true;
		result.srcColorBlend = RHI::AttachmentBlendFactor::Zero;
		result.dstColorBlend = RHI::AttachmentBlendFactor::OneMinusSrcColor;
		result.colorBlendOp = RHI::AttachmentBlendOp::Add;
		result.srcAlphaBlend = RHI::AttachmentBlendFactor::Zero;
		result.dstAlphaBlend = RHI::AttachmentBlendFactor::OneMinusSrcColor;
		result.alphaBlendOp = RHI::AttachmentBlendOp::Add;

		return result;
	}

	inline RHI::AttachmentBlendState OneMinusSrcAlpha()
	{
		RHI::AttachmentBlendState result;
		result.enabled = true;
		result.srcColorBlend = RHI::AttachmentBlendFactor::SrcAlpha;
		result.dstColorBlend = RHI::AttachmentBlendFactor::OneMinusSrcAlpha;
		result.colorBlendOp = RHI::AttachmentBlendOp::Add;
		result.srcAlphaBlend = RHI::AttachmentBlendFactor::SrcAlpha;
		result.dstAlphaBlend = RHI::AttachmentBlendFactor::OneMinusSrcAlpha;
		result.alphaBlendOp = RHI::AttachmentBlendOp::Add;

		return result;
	}

}
