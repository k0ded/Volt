#include "Defines.hlsli"
#include "Resources.hlsli"

#include "GPUScene.hlsli"
#include "Structures.hlsli"
#include "Utility.hlsli"
#include "VectorUtility.hlsli"

#include "MeshletHelpers.hlsli"
#include "Barycentrics.hlsli"
#include "VisibilityBuffer.hlsli"

vt::Tex2D<uint2> VisibilityBuffer;
vt::TypedBuffer<uint> MaterialCountBuffer;
vt::TypedBuffer<uint> MaterialStartBuffer;
vt::TypedBuffer<uint2> PixelCollection;

GPUScene GPUSceneData;

vt::UniformBuffer<ViewData> View;

vt::RWTex2D<float4> Albedo;
vt::RWTex2D<float3> Normals;
vt::RWTex2D<float2> Material;
vt::RWTex2D<float3> Emissive;

uint MaterialId;

float2 ViewSize; // Move to buffer

struct MaterialEvaluationData
{
    float2 texCoords;
    float2 texCoordsDX;
    float2 texCoordsDY;
};

struct EvaluatedMaterial
{
    float4 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 emissive;
    
    void Setup()
    {
        albedo = 1.f;
        roughness = 0.9f;
        metallic = 0.f;
        normal = float3(0.5f, 0.5f, 1.f);
        emissive = 0.f;
    }
};

EvaluatedMaterial EvaluateMaterial(in GPUMaterial material, in MaterialEvaluationData evalData)
{
    GENERATED_SHADER
}

[numthreads(256, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex)
{
    const ViewData viewData = View.Load();
    
    uint materialCount = MaterialCountBuffer.Load(MaterialId);
    uint materialStart = MaterialStartBuffer.Load(MaterialId);
    
    const uint pixelIndex = materialStart + threadId.x;
    
    if (threadId.x >= materialCount)
    {
        return;
    }
    
    const float2 pixelPosition = PixelCollection.Load(pixelIndex) + 0.5f;
    const uint2 visibilityValues = VisibilityBuffer.Load(int3(pixelPosition, 0));
    
    const uint objectId = visibilityValues.x;
    const uint triangleId = UnpackTriangleID(visibilityValues.y);
    const uint meshletId = UnpackMeshletID(visibilityValues.y);
    
    const PrimitiveDrawData drawData = GPUSceneData.primitiveDrawDataBuffer.Load(objectId);
    const GPUMesh mesh = GPUSceneData.meshesBuffer.Load(drawData.meshId);
    const Meshlet meshlet = mesh.meshletsBuffer.Load(mesh.meshletStartOffset + meshletId);

    const uint3 meshletTriIndices = UnpackPrimitive(mesh.meshletDataBuffer.Load(meshlet.dataOffset + meshlet.GetVertexCount() + triangleId)); 
    const uint3 triIndices = LoadTriangleIndices(mesh.meshletDataBuffer, meshlet.dataOffset, mesh.vertexStartOffset, meshletTriIndices);
    const PositionData vertexPositions = LoadVertexPositions(mesh.vertexPositionsBuffer, triIndices);

    const float4 worldPositions[] = 
    {
        float4(drawData.transform.GetWorldPosition(vertexPositions.positions[0]), 1.f),
        float4(drawData.transform.GetWorldPosition(vertexPositions.positions[1]), 1.f),
        float4(drawData.transform.GetWorldPosition(vertexPositions.positions[2]), 1.f),
    };

    const float4 clipPositions[] =
    {
        mul(viewData.projection, mul(viewData.view, worldPositions[0])),
        mul(viewData.projection, mul(viewData.view, worldPositions[1])),
        mul(viewData.projection, mul(viewData.view, worldPositions[2]))
    };

    const float2 screenPos = float2((pixelPosition.x / ViewSize.x) * 2.f - 1.f, -(pixelPosition.y / ViewSize.y) * 2.f + 1.f);

    const PartialDerivatives derivatives = CalculateDerivatives(clipPositions, screenPos, ViewSize);
    const MaterialData materialData = LoadVertexMaterialData(mesh.vertexMaterialBuffer, triIndices);    
    const UVGradient uvGradient = CalculateUVGradient(derivatives, materialData.texCoords);
    
    const float3 normal = normalize(drawData.transform.RotateVector(normalize(InterpolateFloat3(derivatives, materialData.normals))));
    const float3 tangent = normalize(drawData.transform.RotateVector(normalize(InterpolateFloat3(derivatives, materialData.tangents))));
    const float3x3 TBN = CalculateTBN(normal, tangent, materialData.tangentW);
    
    const GPUMaterial material = GPUSceneData.materialsBuffer.Load(MaterialId);
    
    MaterialEvaluationData evalData;
    evalData.texCoords = uvGradient.uv;
    evalData.texCoordsDX = uvGradient.ddx;
    evalData.texCoordsDY = uvGradient.ddy;
    
    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(material, evalData);
    
    float3 resultNormal = evaluatedMaterial.normal.xyz * 2.f - 1.f;
    resultNormal.z = sqrt(1.f - saturate(resultNormal.x * resultNormal.x + resultNormal.y * resultNormal.y));
    resultNormal = normalize(mul(TBN, normalize(resultNormal)));
    
    float4 albedo = evaluatedMaterial.albedo;

    // #TODO_Ivar: This depends on the texture format
    //albedo.xyz = SRGBToLinear(albedo.xyz);
    
    Albedo.Store(pixelPosition, albedo);
    Normals.Store(pixelPosition, resultNormal * 0.5f + 0.5f);
    Material.Store(pixelPosition, float2(evaluatedMaterial.metallic, evaluatedMaterial.roughness));
    Emissive.Store(pixelPosition, evaluatedMaterial.emissive);
}