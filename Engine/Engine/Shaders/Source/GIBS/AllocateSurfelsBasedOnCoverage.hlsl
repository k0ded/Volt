#include "Resources.hlsli"
#include "Utility.hlsli"
#include "Atomics.hlsli"

#include "GIBSCommon.hlsli"

struct Constants
{
    vt::Tex2D<float> surfelCoverage;
    vt::RWTypedBuffer<Surfel> surfels;
    vt::RWTypedBuffer<uint> surfelsAllocator;

    vt::UniformBuffer<ViewData> viewData;
    vt::Tex2D<float> depthTexture;
    vt::Tex2D<float4> normalsTexture;

    uint maxSurfelCount;
    float cameraFov;
};

static const float PI = 3.14159265359f;

float CalculateSurfelRadius(float distance, float fovY, float2 renderTargetSize, float area)
{
    return distance * tan(sqrt(area / PI) * fovY / max(renderTargetSize.x, renderTargetSize.y));
}

[numthreads(1, 1, 1)]
void AllocateSurfelsBasedOnCoverage(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    const ViewData viewData = constants.viewData.Load();

    uint2 basePixelOffset = dispatchThreadID * 16;

    uint2 minCoveragePixel;
    uint currentMinCoverage = 1000000;

    [unroll]
    for (uint x = 0; x < 16; x++)
    {
        [unroll]
        for (uint y = 0; y < 16; y++)
        {
            uint2 currentPixelOffset = basePixelOffset + uint2(x, y);
            if (currentPixelOffset.x > viewData.renderSize.x || currentPixelOffset.y > viewData.renderSize.y)
            {
                continue;
            }

            float pixelCoverage = constants.surfelCoverage.Load(int3(currentPixelOffset, 0));

            if (pixelCoverage < currentMinCoverage)
            {
                currentMinCoverage = pixelCoverage;
                minCoveragePixel = currentPixelOffset;
            }
        }
    }

    if (currentMinCoverage > 2.f)
    {
        return;
    }

    if (constants.surfelsAllocator.Load(0) < constants.maxSurfelCount)
    {
        uint surfelIndex;
	    InterlockedAdd(constants.surfelsAllocator, 0, 1, surfelIndex);

        float2 screenUV = (float2)minCoveragePixel * viewData.invRenderSize;
        screenUV.y = 1.f - screenUV.y;
        const float pixelDepth = constants.depthTexture.Load(int3(minCoveragePixel, 0));
        const float3 pixelWorldPosition = ReconstructWorldPosition(viewData, screenUV, pixelDepth);
        const float3 pixelNormal = normalize(constants.normalsTexture.Load(int3(minCoveragePixel, 0)).xyz * 2.f - 1.f);
    
        float distanceToCamera = length(viewData.cameraPosition.xyz - pixelWorldPosition);

        Surfel newSurfel;
        newSurfel.radius = CalculateSurfelRadius(distanceToCamera, constants.cameraFov, viewData.renderSize, 200.f);
        newSurfel.worldPosition = pixelWorldPosition;
        newSurfel.normal = pixelNormal;

        constants.surfels.Store(surfelIndex, newSurfel);
    }
}