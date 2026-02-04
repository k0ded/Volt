#include "RenderPipelineLegacy/Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "DrawDebugMeshesCommon.hlsli"
#include "Utility/Packing.hlsli"

struct DebugMeshData
{
    float3 position;
    float padding0;
    float3 scale;
    float padding1;
    float4 rotation;
    float4 userData;
};

StructuredBuffer<DebugMeshData> DebugMeshDatas;

DrawDebugMeshesPixelShaderInput MainVS(in FullVertex input)
{
    const DebugMeshData debugMeshData = DebugMeshDatas[input.primitiveIndex];

    Transform meshTransform;
    meshTransform.Initialize(debugMeshData.position, debugMeshData.scale, debugMeshData.rotation);

    const float3 normal = UnpackNormalFromUInt32(input.normal);
    const float3 tangent = DecodeTangent(normal, input.tangent);

    TransformedVertAttribs transformedVertAttribs;
    transformedVertAttribs.position = meshTransform.TransformPosition(input.position);
    transformedVertAttribs.normal = meshTransform.RotateVector(normal);
    transformedVertAttribs.tangent = meshTransform.RotateVector(tangent);

    DrawDebugMeshesPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(transformedVertAttribs.position, 1.f));
    result.worldPosition = transformedVertAttribs.position;
    result.texCoords = input.texCoords;
    result.normal = normalize(transformedVertAttribs.normal);
    result.tangent = float4(normalize(transformedVertAttribs.tangent), input.tangentW);
    result.primitiveIndex = input.primitiveIndex;
    result.objectId = asuint(debugMeshData.userData.x);
    result.visProxyId = asuint(debugMeshData.userData.y);

    return result;
}