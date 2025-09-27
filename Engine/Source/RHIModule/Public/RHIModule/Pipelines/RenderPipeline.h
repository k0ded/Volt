#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Shader/Shader.h"

#include <CoreUtilities/Containers/Array.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt::RHI
{
	struct AttachmentBlendState
	{
		bool enabled = false;
		AttachmentBlendFactor srcColorBlend = AttachmentBlendFactor::One;
		AttachmentBlendFactor dstColorBlend = AttachmentBlendFactor::One;
		AttachmentBlendOp colorBlendOp = AttachmentBlendOp::Add;
		AttachmentBlendFactor srcAlphaBlend = AttachmentBlendFactor::One;
		AttachmentBlendFactor dstAlphaBlend = AttachmentBlendFactor::One;
		AttachmentBlendOp alphaBlendOp = AttachmentBlendOp::Add;
	};

	struct RenderPipelineCreateInfo
	{
		Vector<RefPtr<Shader>> shaders;

		Topology topology = Topology::TriangleList;
		CullMode cullMode = CullMode::Back;
		FillMode fillMode = FillMode::Solid;
		DepthMode depthMode = DepthMode::ReadWrite;
		CompareOperator depthCompareOperator = CompareOperator::GreaterEqual;
		bool enablePrimitiveRestart = false;
		bool enableDepthClamp = false;
		float depthBiasConstantFactor = 0.f;
		float depthBiasClamp = 0.f;
		float depthBiasSlopeFactor = 0.f;

		Array<AttachmentBlendState, MAX_COLOR_ATTACHMENT_COUNT> attachmentBlendStates;
		std::string name;
	};

	class VTRHI_API RenderPipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsValid() const = 0;
		virtual size_t GetHash() const = 0;
		virtual const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const = 0;
		virtual ArrayView<ShaderParameterMap> GetShaderParameterMaps() const = 0;
		virtual const Vector<RefPtr<Shader>>& GetShaders() const = 0;

		static RefPtr<RenderPipeline> Create(const RenderPipelineCreateInfo& createInfo);

	protected:
		RenderPipeline() = default;
		virtual ~RenderPipeline() = default;
	};
}
