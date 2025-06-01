#include "Testing/RenderGraphTests/ClearCreatedRenderTargetTest.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>

using namespace Volt;

BEGIN_SHADER_PARAMETER_STRUCT(ClearParameters)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<uint>, texture)
END_SHADER_PARAMETER_STRUCT()

RG_ClearCreatedRenderTargetTest::RG_ClearCreatedRenderTargetTest()
{
}

RG_ClearCreatedRenderTargetTest::~RG_ClearCreatedRenderTargetTest()
{
}

bool RG_ClearCreatedRenderTargetTest::RunTest()
{
	RenderGraph renderGraph{ m_commandBuffer };

	RGTextureRef testTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1280, 720, RHI::ImageUsage::AttachmentStorage, "TestTexture"));

	ClearParameters* clearParameters = renderGraph.AllocParameters<ClearParameters>();
	clearParameters->texture = renderGraph.CreateUAV(testTexture);

	renderGraph.AddPass("Clear Pass",
		RenderGraphPassFlags::NeverCull | RenderGraphPassFlags::Compute,
		clearParameters,
		[clearParameters](RenderContext& context) 
	{
		context.ClearUAV(clearParameters->texture, glm::vec4{ 0.f, 0.f, 0.f, 1.f });
	});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
