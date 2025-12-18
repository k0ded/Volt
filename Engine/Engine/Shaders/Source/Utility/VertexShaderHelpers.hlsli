#include "RenderScene/GPUScene.hlsli"

#pragma once

namespace VertexShaderHelpers
{
    float3 TransformVertexToWorldSpace(in PrimitiveDrawData primitiveData, in GPUMesh gpuMesh, float3 vertexPosition)
    {
        Transform combinedTransform = primitiveData.transform.Combine(gpuMesh.transform);
        return combinedTransform.TransformPosition(vertexPosition);
    }
}