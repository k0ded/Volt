#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Utility/RootSignatureBuilder.h"

namespace Volt::RHI
{
	inline D3D12_SHADER_VISIBILITY GetShaderVisibilityFromShaderStage(ShaderStage shaderStage)
	{
		switch (shaderStage)
		{
			case ShaderStage::Vertex: return D3D12_SHADER_VISIBILITY_VERTEX;
			case ShaderStage::Pixel: return D3D12_SHADER_VISIBILITY_PIXEL;
			case ShaderStage::Hull: return D3D12_SHADER_VISIBILITY_HULL;
			case ShaderStage::Domain: return D3D12_SHADER_VISIBILITY_DOMAIN;
			case ShaderStage::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
			case ShaderStage::Compute: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::RayGen: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::AnyHit: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::ClosestHit: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::Miss: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::Intersection: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::Callable: return D3D12_SHADER_VISIBILITY_ALL;
			case ShaderStage::Amplification: return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
			case ShaderStage::Mesh: return D3D12_SHADER_VISIBILITY_MESH;
			case ShaderStage::All: return D3D12_SHADER_VISIBILITY_ALL;
		}

		VT_ENSURE(false);
		return D3D12_SHADER_VISIBILITY_ALL;
	}

	RootSignatureBuilder::RootSignature RootSignatureBuilder::BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindings& resourceBindings, bool accessesRayTracingResourceTable)
	{
		VT_ENSURE_MSG(!accessesRayTracingResourceTable, "Not implemented!");

		Map<ShaderStage, Vector<D3D12_DESCRIPTOR_RANGE>> perStageDescriptorRanges;
		Map<ShaderStage, Vector<D3D12_DESCRIPTOR_RANGE>> perStageSamplerRanges;

		auto allocateDescriptorRange = [&perStageDescriptorRanges, &perStageSamplerRanges](ShaderStage shaderStage, ShaderResourceType resourceType, ShaderRegisterType registerType) -> D3D12_DESCRIPTOR_RANGE&
		{
			D3D12_DESCRIPTOR_RANGE& descriptorRange = resourceType == ShaderResourceType::Sampler ? perStageSamplerRanges[shaderStage].emplace_back() : perStageDescriptorRanges[shaderStage].emplace_back();
			switch (registerType)
			{
				case ShaderRegisterType::CBV: descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
				case ShaderRegisterType::UAV: descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
				case ShaderRegisterType::SRV: descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
				case ShaderRegisterType::Sampler: descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
			}

			return descriptorRange;
		};

		// Start by adding a range per binding.
		for (const auto& [binding, nameHash] : resourceBindings)
		{
			auto& descriptorRange = allocateDescriptorRange(binding.shaderStage, binding.resourceType, binding.registerType);
			descriptorRange.RegisterSpace = binding.set;
			descriptorRange.BaseShaderRegister = binding.binding;
			descriptorRange.NumDescriptors = 1;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		Map<ShaderStage, Vector<D3D12_DESCRIPTOR_RANGE>> compactedDescriptorRanges;
		Map<ShaderStage, Vector<D3D12_DESCRIPTOR_RANGE>> compactedSamplerRanges;

		// Next we compact them.
		for (const auto& [shaderStage, descriptorRanges] : perStageDescriptorRanges)
		{
			auto& compactedDescrptorRange = compactedDescriptorRanges[shaderStage];

			for (const D3D12_DESCRIPTOR_RANGE& range : descriptorRanges)
			{
				bool foundMatching = false;
				// See if we can find a matching descriptor range
				for (D3D12_DESCRIPTOR_RANGE& compactedRange : compactedDescrptorRange)
				{
					if (compactedRange.RangeType == range.RangeType &&
						compactedRange.BaseShaderRegister + compactedRange.NumDescriptors == range.BaseShaderRegister)
					{
						foundMatching = true;
						compactedRange.NumDescriptors += range.NumDescriptors;
						break;
					}
				}

				// No matching found, add this range
				if (!foundMatching)
				{
					compactedDescrptorRange.emplace_back(range);
				}
			}
		}

		for (const auto& [shaderStage, descriptorRanges] : perStageSamplerRanges)
		{
			auto& compactedSamplerRange = compactedSamplerRanges[shaderStage];

			for (const D3D12_DESCRIPTOR_RANGE& range : descriptorRanges)
			{
				bool foundMatching = false;
				// See if we can find a matching descriptor range
				for (D3D12_DESCRIPTOR_RANGE& compactedRange : compactedSamplerRange)
				{
					if (compactedRange.RangeType == range.RangeType &&
						compactedRange.BaseShaderRegister + compactedRange.NumDescriptors == range.BaseShaderRegister)
					{
						foundMatching = true;
						compactedRange.NumDescriptors += range.NumDescriptors;
						break;
					}
				}

				// No matching found, add this range
				if (!foundMatching)
				{
					compactedSamplerRange.emplace_back(range);
				}
			}
		}

		// Set offsets
		RootSignature result;

		for (auto& [shaderStage, descriptorRanges] : compactedDescriptorRanges)
		{
			uint32_t descriptorOffsetInTable = 0;

			for (D3D12_DESCRIPTOR_RANGE& range : descriptorRanges)
			{
				RootSignature::DescriptorRange& newRange = result.shaderStageDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)].emplace_back();
				newRange.baseBinding = range.BaseShaderRegister;
				newRange.numDescriptors = range.NumDescriptors;
				newRange.offsetFromTableStart = descriptorOffsetInTable;
		
				switch (range.RangeType)
				{
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV: newRange.registerType = ShaderRegisterType::CBV; break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: newRange.registerType = ShaderRegisterType::SRV; break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV: newRange.registerType = ShaderRegisterType::UAV; break;
				}

				range.OffsetInDescriptorsFromTableStart = descriptorOffsetInTable;
				descriptorOffsetInTable += range.NumDescriptors;
			}
		}

		for (auto& [shaderStage, descriptorRanges] : compactedSamplerRanges)
		{
			uint32_t descriptorOffsetInTable = 0;

			for (D3D12_DESCRIPTOR_RANGE& range : descriptorRanges)
			{
				RootSignature::DescriptorRange& newRange = result.shaderStageSamplerDescriptorRanges[GetDescriptorSetIndexFromShaderStage(shaderStage)].emplace_back();
				newRange.registerType = ShaderRegisterType::Sampler;
				newRange.baseBinding = range.BaseShaderRegister;
				newRange.numDescriptors = range.NumDescriptors;
				newRange.offsetFromTableStart = descriptorOffsetInTable;

				range.OffsetInDescriptorsFromTableStart = descriptorOffsetInTable;
				descriptorOffsetInTable += range.NumDescriptors;
			}
		}

		// Next we create the root parameter descriptor tables.

		Vector<D3D12_ROOT_PARAMETER> rootParameters;

		for (const auto& [shaderStage, descriptorRanges] : compactedDescriptorRanges)
		{
			result.shaderStageTableIndex[GetDescriptorSetIndexFromShaderStage(shaderStage)] = static_cast<uint32_t>(rootParameters.size());

			auto& rootParameter = rootParameters.emplace_back();
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.DescriptorTable = { static_cast<uint32_t>(descriptorRanges.size()), descriptorRanges.data() };
			rootParameter.ShaderVisibility = GetShaderVisibilityFromShaderStage(shaderStage);
		}

		for (const auto& [shaderStage, descriptorRanges] : compactedSamplerRanges)
		{
			result.shaderStageSamplerTableIndex[GetDescriptorSetIndexFromShaderStage(shaderStage)] = static_cast<uint32_t>(rootParameters.size());

			auto& rootParameter = rootParameters.emplace_back();
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.DescriptorTable = { static_cast<uint32_t>(descriptorRanges.size()), descriptorRanges.data() };
			rootParameter.ShaderVisibility = GetShaderVisibilityFromShaderStage(shaderStage);
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
		rootSignatureDesc.NumParameters = static_cast<uint32_t>(rootParameters.size());
		rootSignatureDesc.pParameters = rootParameters.data();
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		VT_D3D12_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errorBlob));

		auto d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();

		ComPtr<ID3D12RootSignature> rootSignature;
		VT_D3D12_CHECK(d3d12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), VT_D3D12_ID(rootSignature)));

		result.rootSignature = rootSignature;
		return result;
	}

	void AppendBindings(ShaderParameterMap::ResourceBindings& outBindings, const ShaderParameterMap::ResourceBindings& shaderBindings)
	{
		// Because multiple shader stages might have bindings with the same name, we need 
		// to check for and handle duplicates. As the names does not matter here, we can replace them
		// with temporary ones.
		uint32_t duplicateIndex = 0;
		for (const auto& [binding, shaderBindingHash] : shaderBindings)
		{
			StringHash newNameHash = shaderBindingHash;

			for (const auto& [newBinding, newBindingHash] : outBindings)
			{
				if (shaderBindingHash == newBindingHash)
				{
					newNameHash = StringHash::Construct("Duplicate" + std::to_string(duplicateIndex++));
				}
			}

			outBindings.emplace_back(binding, newNameHash);
		}
	}

	RootSignatureBuilder::RootSignature RootSignatureBuilder::BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings, bool accessesRayTracingResourceTable)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderParameterMap::ResourceBindings mergedShaderBindings;

		for (const auto& shaderBindings : resourceBindings)
		{
			AppendBindings(mergedShaderBindings, shaderBindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return BuildFromShaderResourceBindings(mergedShaderBindings, accessesRayTracingResourceTable);
	}
}
