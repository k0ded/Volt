#pragma once

#include "D3D12RHIModule/Shader/D3D12Shader.h"

#include <RHIModule/Descriptors/ShaderBindingMap.h>

#include <CoreUtilities/Containers/VectorVariants.h>

namespace Volt::RHI
{
	class RootSignatureBuilder
	{
	public:
		struct RootSignature
		{
			struct DescriptorRange
			{
				ShaderRegisterType registerType;
				uint32_t baseBinding;
				uint32_t numDescriptors;
				uint32_t offsetFromTableStart;
			};

			VT_INLINE uint32_t GetDescriptorTableIndexFromShaderStage(ShaderStage shaderStage) const
			{
				return shaderStageTableIndex[GetDescriptorSetIndexFromShaderStage(shaderStage)];
			}

			VT_INLINE uint32_t GetSamplerDescriptorTableIndexFromShaderStage(ShaderStage shaderStage) const
			{
				return shaderStageSamplerTableIndex[GetDescriptorSetIndexFromShaderStage(shaderStage)];
			}

			VT_INLINE uint32_t GetFlatDescriptorIndexFromBindingAndType(ShaderStage shaderStage, ShaderRegisterType registerType, uint32_t binding) const
			{
				for (const DescriptorRange& descriptorRange : shaderStageDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)])
				{
					if (descriptorRange.registerType == registerType && 
						binding >= descriptorRange.baseBinding &&
						binding < descriptorRange.baseBinding + descriptorRange.numDescriptors)
					{
						return descriptorRange.offsetFromTableStart + (binding - descriptorRange.baseBinding);
					}
				}

				VT_ENSURE(false);
				return 0;
			}

			VT_INLINE uint32_t GetFlatSamplerDescriptorIndexFromBinding(ShaderStage shaderStage, uint32_t binding) const
			{
				for (const DescriptorRange& descriptorRange : shaderStageSamplerDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)])
				{
					if (binding >= descriptorRange.baseBinding &&
						binding < descriptorRange.baseBinding + descriptorRange.numDescriptors)
					{
						return descriptorRange.offsetFromTableStart + (binding - descriptorRange.baseBinding);
					}
				}

				VT_ENSURE(false);
				return 0;
			}

			VT_INLINE ArrayView<DescriptorRange> GetDescriptorRangesFromShaderStage(ShaderStage shaderStage) const
			{
				return shaderStageDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)];
			}

			VT_INLINE ArrayView<DescriptorRange> GetSamplerDescriptorRangesFromShaderStage(ShaderStage shaderStage) const
			{
				return shaderStageSamplerDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)];
			}

			Array<uint32_t, GetNumShaderStages()> shaderStageTableIndex;
			Array<uint32_t, GetNumShaderStages()> shaderStageSamplerTableIndex;

			Array<Vector<DescriptorRange>, GetNumShaderStages()> shaderStageDescriptorRanges;
			Array<Vector<DescriptorRange>, GetNumShaderStages()> shaderStageSamplerDescriptorRanges;

			ComPtr<ID3D12RootSignature> rootSignature;
		};

		RootSignature BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindings& resourceBindings, bool accessesRayTracingResourceTable);
		RootSignature BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings, bool accessesRayTracingResourceTable);
	};

}
