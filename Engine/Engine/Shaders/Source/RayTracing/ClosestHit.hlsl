#include "Utility.hlsli"
#include "GPUScene.hlsli"
#include "RayTracing.hlsli"

#include "VisibilityBuffer.hlsli"
#include "MonteCarlo.hlsli"

#include "BlueNoise.hlsli"
#include "Lights.hlsli"
#include "PBR/BRDF.hlsli"

#include "PathTracingCommon.hlsli"

struct Attributes
{
  float2 bary;
};

struct Constants
{
    vt::UniformBuffer<ViewData> viewData;
    vt::UniformBuffer<DirectionalLight> directionalLight;
    vt::RWTex2D<float4> outputTexture;
    uint frameIndex;

    GPUScene gpuScene;
    BlueNoiseData blueNoiseData;
};

float TraceShadowRay(float3 dirToLight, float3 worldPosition)
{
    RayDesc rayDesc;
    rayDesc.Origin = worldPosition;
    rayDesc.Direction = dirToLight;
    rayDesc.TMin = 0.01f;
    rayDesc.TMax = 10000.f;
    
    ShadowPayload payload;
    TraceRay(g_accelerationStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 1, 0, 1, rayDesc, payload);

    return payload.visibility;
}

float3 CalculateDirectionalLight(in DirectionalLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 D = normalize(light.direction.xyz);

    const float NdotD = saturate(dot(brdfInput.N, D));

    float shadow = TraceShadowRay(D, worldPosition);
    float illuminance = light.intensity * NdotD;

    return BRDF(brdfInput, D) * light.color * illuminance * shadow;
}

float hash12n(float2 p) 
{
	p  = frac(p * float2(5.3987f, 5.4421f));
    p += dot(p.yx, p.xy + float2(21.5351f, 14.3137f));
	return frac(p.x * p.y * 95.4307f);
}

float3 OffsetPositionAlongNormal(float3 worldPosition, float3 normal)
{
  // Convert the normal to an integer offset.
  const float int_scale = 256.0f;
  const int3 of_i      = int3(int_scale * normal);

  // Offset each component of worldPosition using its binary representation.
  // Handle the sign bits correctly.
  const float3 p_i = float3(  //
      asfloat(asint(worldPosition.x) + ((worldPosition.x < 0) ? -of_i.x : of_i.x)),
      asfloat(asint(worldPosition.y) + ((worldPosition.y < 0) ? -of_i.y : of_i.y)),
      asfloat(asint(worldPosition.z) + ((worldPosition.z < 0) ? -of_i.z : of_i.z)));

  // Use a floating-point offset instead for points near (0,0,0), the origin.
  const float origin     = 1.0f / 32.0f;
  const float floatScale = 1.0f / 65536.0f;
  return float3(  //
      abs(worldPosition.x) < origin ? worldPosition.x + floatScale * normal.x : p_i.x,
      abs(worldPosition.y) < origin ? worldPosition.y + floatScale * normal.y : p_i.y,
      abs(worldPosition.z) < origin ? worldPosition.z + floatScale * normal.z : p_i.z);
}

float3 DiffuseReflection(float3 normal, inout uint rngState)
{
    const float theta = 2.f * PI * StepAndOutputRNGFloat(rngState);
    const float u = 2.f * StepAndOutputRNGFloat(rngState) - 1.f;
    const float r = sqrt(1.f - u * u);
    
    float3 rayDirection = normal + float3(r * cos(theta), r * sin(theta), u);
    rayDirection = normalize(rayDirection);

    return rayDirection;
}

[shader("closesthit")]
void main(inout Payload p, in Attributes attribs)
{
    const Constants constants = GetConstants<Constants>();
    const GPUScene gpuScene = constants.gpuScene;
 
    const uint primitiveIndex = PrimitiveIndex();

    const PrimitiveDrawData primitiveDrawData = gpuScene.primitiveDrawDataBuffer.Load(InstanceID());
    const GPUMesh mesh = gpuScene.meshesBuffer.Load(primitiveDrawData.meshId);

    const uint3 triIndices = uint3(mesh.indexBuffer.Load(primitiveIndex * 3), mesh.indexBuffer.Load(primitiveIndex * 3 + 1), mesh.indexBuffer.Load(primitiveIndex * 3 + 2));
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
     
    const PositionData vertexPositions = LoadVertexPositions(mesh.vertexPositionsBuffer, triIndices);
    const MaterialData materialData = LoadVertexMaterialData(mesh.vertexMaterialBuffer, triIndices);    
    
    float3 normal = normalize(GetInterpolatedFloat3(materialData.normals, barycentricCoords));
    normal = normalize(primitiveDrawData.transform.RotateVector(normal));    

    float3 worldPosition = primitiveDrawData.transform.GetWorldPosition(GetInterpolatedFloat3(vertexPositions.positions, barycentricCoords));

    const float3 ambiance = 0.2f;
    const float3 color = 1.f;
    const float metallic = 0.f;
    const float roughness = 1.f;
    
    BRDFInput brdfInput;
    brdfInput.V = -WorldRayDirection();
    brdfInput.N = normal;
    brdfInput.diffuseColor = CalculateDiffuseColor(color, metallic);
    brdfInput.f0 = CalculateF0(color, metallic);
    brdfInput.f90 = CalculateF90(color, metallic);
    brdfInput.roughness = roughness;
    brdfInput.metalness = metallic;
    
    p.radiance = CalculateDirectionalLight(constants.directionalLight.Load(), brdfInput, worldPosition) + ambiance;
    p.rayDirection = DiffuseReflection(normal, p.rngState);
    p.rayOrigin = OffsetPositionAlongNormal(worldPosition, normal);
    p.miss = false;

    //const uint MaxPathLength = 6;
    //
    //if (p.pathLength + 1 < MaxPathLength)
    //{
    //    float3 localRayDirection = CosineSampleHemisphere(BlueNoiseScalar(p.pixel, constants.frameIndex, constants.blueNoiseData)).xyz;
    //    float3x3 tangentBasis = GetTangentBasis(normal);
    //    float3 worldRayDirection = normalize(mul(localRayDirection, tangentBasis));
    //
    //    RayDesc rayDesc;
    //    rayDesc.Origin = worldPosition;
    //    rayDesc.Direction = reflect(WorldRayDirection(), normal);
    //    rayDesc.TMin = 0.01f;
    //    rayDesc.TMax = 10000.f;
    //
    //    Payload payload;
    //    payload.pathLength = p.pathLength + 1;
    //    payload.radiance = p.radiance;
    //    payload.pixel = p.pixel;
    //    TraceRay(g_accelerationStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);
    //
    //    p.radiance *= payload.radiance;
    //}

    

    //
    //if (p.pathLength + 1 < MaxPathLength)
    //{
    //    float3 localRayDirection = CosineSampleHemisphere(BlueNoiseScalar(p.pixel, constants.frameIndex, constants.blueNoiseData)).xyz;
    //    float3x3 tangentBasis = GetTangentBasis(normal);
    //    float3 worldRayDirection = normalize(mul(localRayDirection, tangentBasis));
    //
    //    RayDesc rayDesc;
    //    rayDesc.Origin = worldPosition;
    //    rayDesc.Direction = worldRayDirection;
    //    rayDesc.TMin = 0.01f;
    //    rayDesc.TMax = 10000.f;
    //    
    //    Payload payload;
    //    payload.pathLength = p.pathLength + 1;
    //    payload.hitValue = 0.f;
    //    payload.pixel = p.pixel;
    //    TraceRay(g_accelerationStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);
    //
    //    p.hitValue += payload.hitValue * brdfInput.diffuseColor;
    //}
}
