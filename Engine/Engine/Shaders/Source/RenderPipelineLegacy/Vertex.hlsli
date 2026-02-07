#pragma once

#include "RenderScene/GPUScene.hlsli"

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
    uint vertexId : SV_VertexID;
};

struct FullVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;

    [[vt::inputIndex(1)]] uint normal : NORMAL;
    [[vt::inputIndex(1)]] float tangent : TANGENT;
    [[vt::inputIndex(1)]] float tangentW : TANGENTW;
    [[vt::inputIndex(1)]] [[vt::half2]] float2 texCoords : TEXCOORD;
    
    [[vt::inputIndex(2)]] uint4 influences : INFLUENCES;
    [[vt::inputIndex(2)]] float4 weights : WEIGHTS;

    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
};

struct TransformedVertAttribs
{
    float3 position;
    float3 normal;
    float3 tangent;
};

TransformedVertAttribs GetTransformedVertAttribs(PrimitiveDrawData primitiveData, float3 skinnedPosition, float3 normal, float3 tangent)
{
    TransformedVertAttribs result;
    result.position = primitiveData.transform.TransformPosition(skinnedPosition);
    result.normal = primitiveData.transform.RotateVector(normal);
    result.tangent = primitiveData.transform.RotateVector(tangent);

    return result;
}