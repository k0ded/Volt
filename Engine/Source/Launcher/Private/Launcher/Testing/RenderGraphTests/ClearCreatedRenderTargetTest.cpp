#include "Testing/RenderGraphTests/ClearCreatedRenderTargetTest.h"

#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>

using namespace Volt;

BEGIN_SHADER_PARAMETER_STRUCT2(ClearParameters)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<uint>, texture)
END_SHADER_PARAMETER_STRUCT2()

RG_ClearCreatedRenderTargetTest::RG_ClearCreatedRenderTargetTest()
{
}

RG_ClearCreatedRenderTargetTest::~RG_ClearCreatedRenderTargetTest()
{
}

bool RG_ClearCreatedRenderTargetTest::RunTest()
{
	RenderGraph2 renderGraph{ m_commandBuffer };

	RGTextureRef testTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1280, 720, RHI::ImageUsage::AttachmentStorage, "TestTexture"));

	ClearParameters* clearParameters = renderGraph.AllocParameters<ClearParameters>();
	clearParameters->texture = renderGraph.CreateUAV(testTexture);

	renderGraph.AddPass("Clear Pass",
		RenderGraphPassFlags::NeverCull | RenderGraphPassFlags::Compute,
		clearParameters,
		[clearParameters](RenderContext2& context) 
	{
		context.ClearUAV(clearParameters->texture, glm::vec4{ 0.f, 0.f, 0.f, 1.f });
	});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
