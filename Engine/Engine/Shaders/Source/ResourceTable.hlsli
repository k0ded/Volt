#pragma once

Texture2D ResourceTable_Texture2DTable[] : register(t0, space9);
ByteAddressBuffer ResourceTable_BufferTable[] : register(t1, space9);

namespace ResourceTable
{
	ByteAddressBuffer LoadBuffer(uint index)
	{
		return ResourceTable_BufferTable[NonUniformResourceIndex(index)];
	}
	
	Texture2D LoadTexture(uint index)
	{
		return ResourceTable_Texture2DTable[NonUniformResourceIndex(index)];
	}
}