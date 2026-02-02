#pragma once

#include "Quaternion.hlsli"

struct Transform
{
    Quaternion rotation;
    float3 position;
    float padding0;
    float3 scale;
    float padding1;

    float3 RotateVector(float3 v)
    {
        return rotation.RotateVector(v);
    }

    float3 TransformPosition(float3 vertexPos)
    {
        return RotateVector(vertexPos * scale) + position;
    }

    Transform Combine(Transform other)
    {
        Transform result;
        result.scale = scale * other.scale;
        result.rotation = rotation * other.rotation;
        result.position = RotateVector(other.position * scale) + position;

        return result;
    }

    void Initialize(float3 inPosition, float3 inScale, float4 inRotation)
    {
        position = inPosition;
        scale = inScale;
        rotation.x = inRotation.x;
        rotation.y = inRotation.y;
        rotation.z = inRotation.z;
        rotation.w = inRotation.w;
    }
};