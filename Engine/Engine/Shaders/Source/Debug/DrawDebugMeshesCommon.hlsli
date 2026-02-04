#pragma once

struct DrawDebugMeshesPixelShaderInput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;

    uint primitiveIndex : PRIMITIVE_INDEX;
    uint objectId : OBJECTID;
    uint visProxyId : VISPROXYID;
};

