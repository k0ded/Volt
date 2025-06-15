#include "Lights/Lights.hlsli"

#include "PBR/PBR.hlsli"
#include "Utility/Utility.hlsli"

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWSceneColor;

Texture2D<float4> GBufferAlbedo;
Texture2D<float4> GBufferNormal;
Texture2D<float2> GBufferMaterial;
Texture2D<float> SceneDepth;

[numthreads(8, 8, 1)]
void MainCS(uint2 threadId : SV_DispatchThreadID)
{
    if (any(threadId >= View.renderSize))
    {
        return;
    }

    const float pixelDepth = SceneDepth.Load(int3(threadId, 0));
    const float4 albedo = GBufferAlbedo.Load(int3(threadId, 0));
    if (pixelDepth > 0.f && albedo.a >= 0.5f)
    {
        const float2 texCoords = float2(float(threadId.x) * View.invRenderSize.x, 1.f - float(threadId.y) * View.invRenderSize.y);
        const float3 worldPosition = ReconstructWorldPosition(texCoords, pixelDepth);
        const float3 dirToCamera = normalize(View.cameraPosition.xyz - worldPosition);

        const float3 normal = GBufferNormal.Load(int3(threadId, 0)).xyz * 2.f - 1.f;
        const float2 material = GBufferMaterial.Load(int3(threadId, 0));

        PBRInput pbrInput;
        pbrInput.albedo = albedo;
        pbrInput.normal = normal;
        pbrInput.roughness = material.x;
        pbrInput.metallic = material.y;
        pbrInput.emissive = 0.f;
        pbrInput.worldPosition = worldPosition;
        pbrInput.ao = 0.f;
        pbrInput.tileId = threadId.xy / LIGHT_CULLING_TILE_SIZE;

        float3 outputColor = EvaluatePBR(pbrInput);

        RWSceneColor[threadId] = float4(outputColor, 1.f);
    }
}