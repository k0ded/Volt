#pragma once

#include "RayTracingResourceTable.hlsli"
#include "Barycentrics.hlsli"

#include "RenderScene/GPUScene.hlsli"
#include "Utility/Packing.hlsli"

struct TriangleAttributes
{
	float3 position;
	float3 normal;
	float3 tangent;
	float2 texCoords;
};

struct VertexPositionData
{
	float3 position;
};

struct VertexMaterialData
{
	uint normal;
	float tangent;
	float tangentW;
	uint texCoords;
};

struct TriangleVertexPositions
{
	float3 position[3];
};

struct TriangleMaterialData
{
	float3 normal[3];
	float3 tangent[3];
	float2 texCoords[3];
};

TriangleVertexPositions LoadVertexPositions(ByteAddressBuffer buffer, uint3 vertexIndices)
{
	TriangleVertexPositions result;
	result.position[0] = buffer.Load<VertexPositionData>(vertexIndices.x * sizeof(VertexPositionData)).position;
	result.position[1] = buffer.Load<VertexPositionData>(vertexIndices.y * sizeof(VertexPositionData)).position;
	result.position[2] = buffer.Load<VertexPositionData>(vertexIndices.z * sizeof(VertexPositionData)).position;

	return result;
}

TriangleMaterialData LoadVertexMaterialData(ByteAddressBuffer buffer, uint3 vertexIndices)
{
	const VertexMaterialData material0 = buffer.Load<VertexMaterialData>(vertexIndices.x * sizeof(VertexMaterialData));
	const VertexMaterialData material1 = buffer.Load<VertexMaterialData>(vertexIndices.y * sizeof(VertexMaterialData));
	const VertexMaterialData material2 = buffer.Load<VertexMaterialData>(vertexIndices.z * sizeof(VertexMaterialData));

	TriangleMaterialData result;
	result.texCoords[0] = UnpackHalf2FromUInt(material0.texCoords);
	result.texCoords[1] = UnpackHalf2FromUInt(material1.texCoords);
	result.texCoords[2] = UnpackHalf2FromUInt(material2.texCoords);

	result.normal[0] = UnpackNormalFromUInt32(material0.normal);
	result.normal[1] = UnpackNormalFromUInt32(material1.normal);
	result.normal[2] = UnpackNormalFromUInt32(material2.normal);
	
	result.tangent[0] = DecodeTangent(result.normal[0], material0.tangent);
	result.tangent[1] = DecodeTangent(result.normal[1], material1.tangent);
	result.tangent[2] = DecodeTangent(result.normal[2], material2.tangent);

	return result;
}

TriangleAttributes LoadTriangleAttributes(in GPUMesh mesh, in Barycentrics triangleBarycentrics, uint primitiveIndex)
{
	ByteAddressBuffer indexBuffer = LoadRTBuffer(mesh.RT_IndexBuffer);
	ByteAddressBuffer vertexPositionBuffer = LoadRTBuffer(mesh.RT_vertexPositionsBuffer);
	ByteAddressBuffer vertexMaterialBuffer = LoadRTBuffer(mesh.RT_vertexMaterialBuffer);

	const uint3 indices = uint3(
		indexBuffer.Load<uint>((primitiveIndex * 3 + 0) * sizeof(uint)),
		indexBuffer.Load<uint>((primitiveIndex * 3 + 1) * sizeof(uint)),
		indexBuffer.Load<uint>((primitiveIndex * 3 + 2) * sizeof(uint))
	);

	TriangleVertexPositions vertexPositions = LoadVertexPositions(vertexPositionBuffer, indices);
	TriangleMaterialData vertexMaterialData = LoadVertexMaterialData(vertexMaterialBuffer, indices);

	TriangleAttributes result;
	result.position = triangleBarycentrics.Interpolate(vertexPositions.position[0], vertexPositions.position[1], vertexPositions.position[2]);
	result.normal = triangleBarycentrics.Interpolate(vertexMaterialData.normal[0], vertexMaterialData.normal[1], vertexMaterialData.normal[2]);
	result.tangent = triangleBarycentrics.Interpolate(vertexMaterialData.tangent[0], vertexMaterialData.tangent[1], vertexMaterialData.tangent[2]);
	result.texCoords = triangleBarycentrics.Interpolate(vertexMaterialData.texCoords[0], vertexMaterialData.texCoords[1], vertexMaterialData.texCoords[2]);

	return result;
}