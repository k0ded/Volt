#include "rhipch.h"

#include "RHIModule/Descriptors/ShaderBindingMap.h"

namespace Volt::RHI
{
	void ShaderBindingMap::SetUniformBuffer(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::CBV;
		resourceBinding.resourceType = ShaderResourceType::UniformBuffer;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.bufferView = bufferView;
	}
	
	void ShaderBindingMap::SetSampler(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::SamplerState> samplerState)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::Sampler;
		resourceBinding.resourceType = ShaderResourceType::Sampler;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.samplerState = samplerState;
	}
	
	void ShaderBindingMap::SetStructuredBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.bufferView = bufferView;
	}
	
	void ShaderBindingMap::SetStructuredBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.bufferView = bufferView;
	}
	
	void ShaderBindingMap::SetTexelBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.bufferView = bufferView;
	}
	
	void ShaderBindingMap::SetTexelBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.bufferView = bufferView;
	}
	
	void ShaderBindingMap::SetTextureSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.imageView = imageView;
	}
	
	void ShaderBindingMap::SetTextureUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.imageView = imageView;
	}
	
	void ShaderBindingMap::SetAccelerationStructure(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::AccelerationStructure> accelerationStructure)
	{
		auto& resourceBinding = m_resourceBindings[shaderStage].emplace_back();
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::AccelerationStructure;
		resourceBinding.bindingIndex = bindingIndex;
		resourceBinding.accelerationStructure = accelerationStructure;
	}
}
