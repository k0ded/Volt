#include "Defines.hlsli"
#include "XeGTAO.hlsli"
#include "Resources.hlsli"

vt::RWTex2D<float> RWDepthMIP0;
vt::RWTex2D<float> RWDepthMIP1;
vt::RWTex2D<float> RWDepthMIP2;
vt::RWTex2D<float> RWDepthMIP3;
vt::RWTex2D<float> RWDepthMIP4;

vt::Tex2D<float> SourceDepth;
vt::TextureSampler PointClampSampler;

GTAOConstants Constants;

// Engine-specific entry point for the first pass
[numthreads(8, 8, 1)] // <- hard coded to 8x8; each thread computes 2x2 blocks so processing 16x16 block: Dispatch needs to be called with (width + 16-1) / 16, (height + 16-1) / 16
void main(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
    Texture2D<float> sourceDepth = SourceDepth.Get();
    RWTexture2D<float> depthMip0 = RWDepthMIP0.Get();
    RWTexture2D<float> depthMip1 = RWDepthMIP1.Get();
    RWTexture2D<float> depthMip2 = RWDepthMIP2.Get();
    RWTexture2D<float> depthMip3 = RWDepthMIP3.Get();
    RWTexture2D<float> depthMip4 = RWDepthMIP4.Get();
    
    XeGTAO_PrefilterDepths16x16(dispatchThreadID, groupThreadID, Constants, sourceDepth, PointClampSampler.Get(), depthMip0, depthMip1, depthMip2, depthMip3, depthMip4);
}