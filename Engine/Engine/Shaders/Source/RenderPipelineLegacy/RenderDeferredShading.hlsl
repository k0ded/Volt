#include "ViewData.hlsli"

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWSceneColor;

Texture2D<float4> Albedo;
Texture2D<float4> Normals;
Texture2D<float2> Material;
Texture2D<float> SceneDepth;

[numthreads(8, 8, 1)]
void MainCS(uint2 threadId : SV_DispatchThreadID)
{
    if (any(threadId >= View.renderSize))
    {
        return;
    }

    const float pixelDepth = SceneDepth.Load(int3(threadId, 0));
    const float4 albedo = Albedo.Load(int3(threadId, 0));
    if (pixelDepth > 0.f && albedo.a >= 0.5f)
    {
        RWSceneColor[threadId] = float4(albedo.rgb, 1.f);
    }
}