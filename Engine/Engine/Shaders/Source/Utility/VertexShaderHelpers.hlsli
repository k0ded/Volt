#include "RenderScene/GPUScene.hlsli"

#pragma once

namespace VertexShaderHelpers
{
    float3 TransformVertexToWorldSpace(in PrimitiveDrawData primitiveData, float3 vertexPosition)
    {
        return primitiveData.transform.TransformPosition(vertexPosition);
    }
}