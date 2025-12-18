#pragma once

#include "Quaternion.hlsli"

struct Transform
{
    Quaternion rotation;
    float3 position;
    float3 scale;

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
};