#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Shader/Shader2.h"

#include <CoreUtilities/Containers/Array.h>

namespace Volt::RHI
{
	class Shader;

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
		RefPtr<Shader> shader;
		Vector<RefPtr<Shader2>> shaders;

		Topology topology = Topology::TriangleList;
		CullMode cullMode = CullMode::Back;
		FillMode fillMode = FillMode::Solid;
		DepthMode depthMode = DepthMode::ReadWrite;
		CompareOperator depthCompareOperator = CompareOperator::GreaterEqual;
		bool enablePrimitiveRestart = false;

		Array<AttachmentBlendState, MAX_ATTACHMENT_COUNT> attachmentBlendStates;
		std::string name;
	};

	class VTRHI_API RenderPipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual RefPtr<Shader> GetShader() const = 0;
		virtual bool IsValid() const = 0;
		virtual size_t GetHash() const = 0;
		virtual const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const = 0;
		virtual const Vector<ShaderParameterMap>& GetShaderParameterMaps() const = 0;

		static RefPtr<RenderPipeline> Create(const RenderPipelineCreateInfo& createInfo);
		static RefPtr<RenderPipeline> Create2(const RenderPipelineCreateInfo& createInfo);

	protected:
		RenderPipeline() = default;
		virtual ~RenderPipeline() = default;
	};
}
