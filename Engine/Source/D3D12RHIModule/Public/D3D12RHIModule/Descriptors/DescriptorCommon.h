#pragma once

#include "D3D12RHIModule/Common/D3D12Common.h"

#include <RHIModule/Shader/ShaderCommon.h>

#include <CoreUtilities/Core.h>

#include <cstdint>
#include <limits>

namespace Volt::RHI
{
	struct D3D12DescriptorPointer
	{
		uint64_t cpuPointer = std::numeric_limits<uint64_t>::max();
		uint64_t gpuPointer = std::numeric_limits<uint64_t>::max();

		VT_NODISCARD VT_INLINE bool IsValid() const { return cpuPointer != std::numeric_limits<uint64_t>::max(); }
		VT_NODISCARD VT_INLINE uint64_t GetCPUPointer() const { return cpuPointer; }
		VT_NODISCARD VT_INLINE uint64_t GetGPUPointer() const { return gpuPointer; }

		VT_INLINE void Reset()
		{
			cpuPointer = std::numeric_limits<uint64_t>::max();
			gpuPointer = std::numeric_limits<uint64_t>::max();
		}
	};

	enum class D3D12DescriptorType
	{
		None,
		RTV,
		DSV,
		CBV_SRV_UAV,
		Sampler
	};

	struct DescriptorCopyInfo
	{
		D3D12DescriptorPointer srcPointer;
		D3D12DescriptorPointer dstPointer;
	};

	struct AllocatedDescriptorInfo
	{
		D3D12DescriptorPointer pointer;
		D3D12ViewType viewType;
		uint32_t descriptorIndex;
		ShaderRegisterType registerType;
	};

	namespace Utility
	{
		inline D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorType(D3D12DescriptorType descriptorType)
		{
			switch (descriptorType)
			{
				case D3D12DescriptorType::RTV: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				case D3D12DescriptorType::DSV: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				case D3D12DescriptorType::CBV_SRV_UAV: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				case D3D12DescriptorType::Sampler: return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			}

			VT_ENSURE(false);
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		}
	}
}
