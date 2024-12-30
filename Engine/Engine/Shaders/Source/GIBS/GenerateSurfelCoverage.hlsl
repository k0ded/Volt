#include "Resources.hlsli"
#include "Utility.hlsli"
#include "Atomics.hlsli"

#include "GIBSCommon.hlsli"

struct GenerateSurfelCoverageConstants
{
    vt::RWTex2D<float> surfelCoverage;
    vt::RWTex2D<float4> debugTexture;
    vt::TypedBuffer<Surfel> surfels;
    vt::TypedBuffer<uint> surfelsAllocator;

    vt::TypedBuffer<uint> surfelCellIndirections;
    vt::TypedBuffer<uint> surfelCellCounts;
    vt::TypedBuffer<uint> surfelCellStartOffsets;

    vt::UniformBuffer<ViewData> viewData;
    vt::Tex2D<float> depthTexture;
    vt::Tex2D<float3> normalsTexture;
};

[numthreads(16, 16, 1)]
void GenerateSurfelCoverage(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const GenerateSurfelCoverageConstants constants = GetConstants<GenerateSurfelCoverageConstants>();
    const ViewData viewData = constants.viewData.Load();

    if (dispatchThreadID.x > viewData.renderSize.x || dispatchThreadID.y > viewData.renderSize.y)
    {
        return;
    }

    float2 screenUV = (float2)dispatchThreadID * viewData.invRenderSize;
    screenUV.y = 1.f - screenUV.y;

    const float pixelDepth = constants.depthTexture.Load(int3(dispatchThreadID, 0));

    if (pixelDepth == 0.f)
    {
        return;
    }

    const float3 pixelWorldPosition = ReconstructWorldPosition(viewData, screenUV, pixelDepth);
    const float3 pixelNormal = normalize(constants.normalsTexture.Load(int3(dispatchThreadID, 0)).xyz * 2.f - 1.f);

    const uint surfelCellIndex = GetCellIndexFromWorldPosition(pixelWorldPosition);

    const uint surfelCount = constants.surfelCellCounts.Load(surfelCellIndex);
    const uint cellStartOffset = constants.surfelCellStartOffsets.Load(surfelCellIndex);

    constants.debugTexture.Store(dispatchThreadID, float4(pixelWorldPosition, 1.f));

    float pixelCoverage = 0.f;
    
    for (uint i = 0; i < surfelCount; i++)
    {
        const uint surfelIndex = constants.surfelCellIndirections.Load(cellStartOffset + i);

        Surfel surfel = constants.surfels.Load(surfelIndex);
        float distanceToSurfel = Distance2(surfel.worldPosition, pixelWorldPosition);

        if (distanceToSurfel < surfel.radius * surfel.radius)
        {
            float backfacing = dot(surfel.normal, pixelNormal);
            if (backfacing > 0.f)
            {
                float distanceSqrt = sqrt(distanceToSurfel);
                float contribution = 1.f;

                contribution *= saturate(backfacing);
                contribution *= saturate(1.f - distanceSqrt / surfel.radius);
                contribution *= smoothstep(0, 1, contribution);

                pixelCoverage += contribution;
            }
        }
    }

    constants.surfelCoverage.Store(dispatchThreadID, pixelCoverage);
}