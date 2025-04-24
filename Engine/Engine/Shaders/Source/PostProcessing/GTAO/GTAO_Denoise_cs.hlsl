
#include "Defines.hlsli"
#include "XeGTAO.hlsli"
#include "Resources.hlsli"

vt::RWTex2D<uint> RWFinalAOTerm;
vt::Tex2D<uint> AOTerm;
vt::Tex2D<float> Edges;
vt::TextureSampler PointClampSampler;

GTAOConstants Constants;

[numthreads(8, 8, 1)]
void main(const uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixCoordBase = dispatchThreadID * uint2(2, 1); // we're computing 2 horizontal pixels at a time (performance optimization)
    
    Texture2D<uint> aoTerm = AOTerm.Get();
    Texture2D<float> edges = Edges.Get();
    
    RWTexture2D<uint> finalAOTerm = RWFinalAOTerm.Get();
    SamplerState samplerState = PointClampSampler.Get();
    
    // g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
    XeGTAO_Denoise(pixCoordBase, Constants, aoTerm, edges, samplerState, finalAOTerm, false);
}