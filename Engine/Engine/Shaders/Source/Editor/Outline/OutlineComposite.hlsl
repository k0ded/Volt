VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWOutputColor;
Texture2D<float4> JumpFloodResult;

float3 OutlineColor;
uint2 RenderSize;

[numthreads(8, 8, 1)]
void OutlineCompositeCS(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    if (all(dispatchThreadID < RenderSize))
    {
        float4 pixel = JumpFloodResult.Load(int3(dispatchThreadID, 0));
        float dist = sqrt(pixel.z);
        float alpha = smoothstep(0.004f, 0.002f, dist);
        if (alpha > 0.f)
        {
            RWOutputColor[dispatchThreadID] = float4(OutlineColor, 1.f);
        }
    }
}