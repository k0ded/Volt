#include "Utility.hlsli"
#include "GPUScene.hlsli"

struct BrickInfo
{
    float3 min;
    float3 max;
};

RWStructuredBuffer<GPUSDFBrick> RWBricks;
RWTexture3D<float> RWBrickTexture;

Buffer<float> BrickData;
StructuredBuffer<BrickInfo> BrickInfoData;

uint BrickTextureSize;

[numthreads(512, 1, 1)]
void MainCS(uint groupThreadId : SV_GroupThreadID, uint groupId : SV_GroupID)
{
    const uint brickTextureSizeInBricks = BrickTextureSize / 8;

    uint3 targetBrickCoord = Get3DCoordFrom1DIndex(groupId, brickTextureSizeInBricks, brickTextureSizeInBricks) * 8;
    uint3 brickLocalCoord = Get3DCoordFrom1DIndex(groupThreadId, 8, 8);

    BrickInfo brickInfo = BrickInfoData[groupId];

    GPUSDFBrick outBrick;
    outBrick.localCoords = (float3)targetBrickCoord / (float)BrickTextureSize;
    outBrick.min = brickInfo.min;
    outBrick.max = brickInfo.max;
    
    RWBrickTexture[targetBrickCoord + brickLocalCoord] = BrickData[groupId * 512 + groupThreadId];
    RWBricks[groupId] = outBrick;
}