#include "XeGTAO.hlsli"

RWTexture2D<float> RWDepthMIP0;
RWTexture2D<float> RWDepthMIP1;
RWTexture2D<float> RWDepthMIP2;
RWTexture2D<float> RWDepthMIP3;
RWTexture2D<float> RWDepthMIP4;

Texture2D<float> SourceDepth;
SamplerState PointClampSampler;

ConstantBuffer<GTAOConstants> Constants;

// Engine-specific entry point for the first pass
[numthreads(8, 8, 1)] // <- hard coded to 8x8; each thread computes 2x2 blocks so processing 16x16 block: Dispatch needs to be called with (width + 16-1) / 16, (height + 16-1) / 16
void MainCS(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
    XeGTAO_PrefilterDepths16x16(dispatchThreadID, groupThreadID, Constants, SourceDepth, PointClampSampler, RWDepthMIP0, RWDepthMIP1, RWDepthMIP2, RWDepthMIP3, RWDepthMIP4);
}