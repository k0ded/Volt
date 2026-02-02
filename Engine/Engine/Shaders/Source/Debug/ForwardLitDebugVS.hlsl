#include "RenderPipelineLegacy/Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "DrawDebugMeshesCommon.hlsli"
#include "Utility/Packing.hlsli"

struct DebugMeshData
{
    float3 position;
    uint userData;
    float3 scale;
    float padding0;
    float4 rotation;
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
    result.objectId = debugMeshData.userData;

    return result;
}