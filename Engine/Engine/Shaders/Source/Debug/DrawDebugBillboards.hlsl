#include "ViewData.hlsli"
#include "BillboardInstanceData.hlsli"
#include "StaticSamplerStates.hlsli"

static const float2 offsets[6] =
{
    { -50.f, 50.f },
    { 50.f, 50.f },
    { 50.f, -50.f },

    { 50.f, -50.f },
    { -50.f, -50.f },
    { -50.f, 50.f }
};

static const float2 uvs[6] =
{
    { 0.f, 0.f },
    { 1.f, 0.f },
    { 1.f, 1.f },

    { 1.f, 1.f },
    { 0.f, 1.f },
    { 0.f, 0.f }
};

StructuredBuffer<BillboardInstanceData> BillboardInstances;

BillboardVSToPS MainVS(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    const BillboardInstanceData billboardInstanceData = BillboardInstances[instanceId];

	BillboardVSToPS result;

    if (billboardInstanceData.isViewSpacePosition)
    {
        result.position = float4(billboardInstanceData.position, 1.f);
    }
    else
    {
        result.position = mul(View.view, float4(billboardInstanceData.position, 1.f));
    }

    result.position.xy += offsets[vertexId] * billboardInstanceData.size.xy;
    result.position = mul(View.projection, result.position);
    result.color = billboardInstanceData.color;
    result.texCoords = uvs[vertexId];
    result.userData = billboardInstanceData.userData;

    return result;
}

struct PSOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
    [[vt::d32f]];
};

Texture2D Texture;

PSOutput MainPS(in BillboardVSToPS input)
{
    const float4 textureColor = Texture.Sample(StaticAnisotropicSamplerClamp, input.texCoords);

	PSOutput output;
	output.color = textureColor * input.color;

	return output;
}