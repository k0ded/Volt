#include "XeGTAO.hlsli"

RWTexture2D<uint> RWFinalAOTerm;
Texture2D<uint> AOTerm;
Texture2D<float> Edges;

ConstantBuffer<GTAOConstants> Constants;

SamplerState PointClampSampler;

[numthreads(8, 8, 1)]
void MainCS(const uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixCoordBase = dispatchThreadID * uint2(2, 1); // we're computing 2 horizontal pixels at a time (performance optimization)
    
    // g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
    XeGTAO_Denoise(pixCoordBase, Constants, AOTerm, Edges, PointClampSampler, RWFinalAOTerm, false);
}