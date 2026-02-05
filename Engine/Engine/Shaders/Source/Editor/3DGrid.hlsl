#include "ViewData.hlsli"

static const float4 m_positions[] =
{
    float4(-1.f, -1.f, 0.f, 1.f),
    float4(-1.f, 3.f, 0.f, 1.f),
    float4(3.f, -1.f, 0.f, 1.f),
};

struct VSToPS
{
    float4 position : SV_Position;
    float3 nearPoint : NEARPOINT;
    float3 farPoint : FARPOINT;
};

float4x4 NonReversedInverseProjection;

float3 UnprojectPoint(float x, float y, float z)
{
    float4 unprojectedPoint = mul(View.inverseView, mul(NonReversedInverseProjection, float4(x, y, z, 1.f)));
    return unprojectedPoint.xyz / unprojectedPoint.w;
} 

VSToPS GridVS(const uint vertexIndex : SV_VertexID)
{
    VSToPS result;
    result.position = m_positions[vertexIndex];
    result.nearPoint = UnprojectPoint(result.position.x, result.position.y, 0.f);
    result.farPoint = UnprojectPoint(result.position.x, result.position.y, 1.f);

    return result;
}

struct PSOutput
{
    float4 output : SV_Target;
    float depth : SV_Depth;
};

float4 EvaluateGrid(float3 position, float scale)
{
    const float2 coords = position.xz * scale;
    const float2 derivative = fwidth(coords);
    const float2 grid = abs(frac(coords - 0.5f) - 0.5f) / derivative;

    const float lin = min(grid.x, grid.y);
    const float minZ = min(derivative.y, 1.f) / scale;
    const float minX = min(derivative.x, 1.f) / scale;

    float4 color = float4(0.2f, 0.2f, 0.2f, 1.f - min(lin, 1.f));
    
    if (position.x > -1.f * minX && position.x < 1.f * minX)
    {
        color.rgb = float3(0.f, 0.f, 1.f);
    }

    if (position.z > -1.f * minZ && position.z < 1.f * minZ)
    {
        color.rgb = float3(1.f, 0.f, 0.f);
    }

    return color;
}

float ComputeDepth(float3 position)
{
    float4 clipPos = mul(View.viewProjection, float4(position, 1.f));
    return clipPos.z / clipPos.w;
}

PSOutput GridPS(VSToPS input)
{
    const float t = -input.nearPoint.y / (input.farPoint.y - input.nearPoint.y);
    const float3 position = input.nearPoint + t * (input.farPoint - input.nearPoint);

    const float linearDepth = mul(View.view, float4(position, 1.f)).z / View.farPlane;
    const float fade = max(0.f, (0.5f - linearDepth));

    PSOutput result;
    result.output = EvaluateGrid(position, 0.01f) * float(t > 0.f);
    result.output.a *= fade;
    result.depth = ComputeDepth(position);

    return result;
}