#pragma once

namespace vt
{
    template<typename T>
    struct TypedBuffer
    {
        ResourceHandle handle;
        
        void GetDimensions(out uint dimension)
        {
            StructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle];
            buffer.GetDimensions(dimension);
        }
        
        T Load(in uint index)
        {
            StructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle];
            return buffer[index];
        }
        
        T operator[](in uint index)
        {
            return Load(index);
        }
    };
    
    template<typename T, bool IsGloballyCoherent = false>
    struct RWTypedBuffer;

    template<typename T>
    struct RWTypedBuffer<T, false>
    {
        ResourceHandle handle;
        
        void GetDimensions(out uint dimension)
        {
            RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            buffer.GetDimensions(dimension);
        }
        
        T Load(in uint index)
        {
            RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            return buffer[index];
        }
      
        void Store(in uint index, T value)
        {
            RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            buffer[index] = value;
        }
    };

    template<typename T>
    struct RWTypedBuffer<T, true>
    {
        ResourceHandle handle;
        
        void GetDimensions(out uint dimension)
        {
            globallycoherent RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            buffer.GetDimensions(dimension);
        }
        
        T Load(in uint index)
        {
            globallycoherent RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            return buffer[index];
        }
      
        void Store(in uint index, T value)
        {
            globallycoherent RWStructuredBuffer<T> buffer = ResourceDescriptorHeap[handle.handle + 1];
            buffer[index] = value;
        }
    };
}