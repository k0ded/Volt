#pragma once

Texture2D RayTracingTexture2DTable[] : register(t0, space9);
ByteAddressBuffer RayTracingBufferTable[] : register(t1, space9);

ByteAddressBuffer LoadRTBuffer(uint index)
{
	return RayTracingBufferTable[NonUniformResourceIndex(index)];
}

Texture2D LoadRTTexture(uint index)
{
	return RayTracingTexture2DTable[NonUniformResourceIndex(index)];
}