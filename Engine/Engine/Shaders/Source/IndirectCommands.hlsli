#pragma once

struct DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;  
    uint firstInstance;

    static const uint SizeInUInts = 5;
};

struct DispatchIndirectCommand
{
    uint x;
    uint y;
    uint z;
};