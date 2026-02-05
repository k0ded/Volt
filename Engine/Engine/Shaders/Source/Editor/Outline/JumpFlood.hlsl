#include "Utility/FullscreenTriangleVertex.hlsli"

Texture2D<float4> InputColor;
SamplerState PointSampler;
float2 RenderSize;

float ScreenDistance(float2 v, float2 texelSize)
{
    float ratio = texelSize.x / texelSize.y;
    v.x /= ratio;

    return dot(v, v);
}

float4 JumpFloodInitPS(FullscreenTriangleVertex input) : SV_Target0
{
    float4 color = InputColor.Sample(PointSampler, input.uv);
    float2 texelSize = float2(1.f / RenderSize.x, 1.f / RenderSize.y);

    float4 result;
    result.xy = float2(100.f, 100.f);
    result.z = ScreenDistance(result.xy, texelSize);
    result.w = color.a > 0.5f ? 1.f : 0.f;

    return result;    
}

struct VSToPS
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
    float2 texelSize : TEXELSIZE;
    float2 UV[9] : UV0;
};

static const float4 m_jumpFloodPositions[] =
{
    float4(-1.f, -1.f, 0.f, 1.f),
    float4(-1.f, 3.f, 0.f, 1.f),
    float4(3.f, -1.f, 0.f, 1.f),
};

static const float2 m_jumpFloodUVS[] =
{
    float2(0.f, 1.f),
    float2(0.f, -1.f),
    float2(2.f, 1.f)
};

float2 TexelSize;
int Step;

VSToPS JumpFloodPassVS(const uint vertexIndex : SV_VertexID)
{
    VSToPS result;
    result.texCoords = m_jumpFloodUVS[vertexIndex];
    result.texelSize = TexelSize;
    
    float2 dx = float2(TexelSize.x, 0.f) * float(Step);
    float2 dy = float2(0.f, TexelSize.y) * float(Step);

    result.UV[0] = result.texCoords;

    // Setup UVs in a 3x3 grid
    result.UV[1] = result.texCoords + dx;
    result.UV[2] = result.texCoords - dx;
    result.UV[3] = result.texCoords + dy;
    result.UV[4] = result.texCoords - dy;
    result.UV[5] = result.texCoords + dx + dy;
    result.UV[6] = result.texCoords + dx - dy;
    result.UV[7] = result.texCoords - dx + dy;
    result.UV[8] = result.texCoords - dx - dy;

    result.position = m_jumpFloodPositions[vertexIndex];
    return result;
}

void BoundsCheck(inout float2 xy, float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        xy = 1000.f;
    }
}

float4 JumpFloodPassPS(VSToPS input) : SV_Target0
{
    float4 pixel = InputColor.Sample(PointSampler, input.UV[0]);

    for (uint j = 1; j <= 8; ++j)
    {
        float4 n = InputColor.Sample(PointSampler, input.UV[j]);
        if (n.w != pixel.w)
        {
            n.xyz = 0.f;
        }

        n.xy += input.UV[j] - input.UV[0];

        BoundsCheck(n.xy, input.UV[j]);

        float dist = ScreenDistance(n.xy, input.texelSize);
        if (dist < pixel.z)
        {
            pixel.xyz = float3(n.xy, dist);
        }
    }

    return pixel;
}