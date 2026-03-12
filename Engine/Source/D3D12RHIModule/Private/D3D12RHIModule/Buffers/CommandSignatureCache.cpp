#include "dxpch.h"

#include "D3D12RHIModule/Buffers/CommandSignatureCache.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <CoreUtilities/Math/Hash.h>

namespace Volt::RHI
{
	CommandSignatureCache g_commandSignatureCache;

	VT_INLINE ComPtr<ID3D12CommandSignature> CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE argType, uint32_t stride)
	{
		auto device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>();
		ID3D12Device10* d3d12Device = device->GetDevice10();

		D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
		argDesc.Type = argType;

		D3D12_COMMAND_SIGNATURE_DESC signatureDesc{};
		signatureDesc.pArgumentDescs = &argDesc;
		signatureDesc.NumArgumentDescs = 1;
		signatureDesc.ByteStride = stride;

		ComPtr<ID3D12CommandSignature> result;
		VT_D3D12_CHECK(d3d12Device->CreateCommandSignature(&signatureDesc, nullptr, VT_D3D12_WRID(result)));

		return result;
	}

	template<D3D12_INDIRECT_ARGUMENT_TYPE ARG_TYPE, typename TYPE>
	VT_INLINE ComPtr<ID3D12CommandSignature> CreateCommandSignature()
	{
		return CreateCommandSignature(ARG_TYPE, sizeof(TYPE));
	}

	VT_INLINE size_t GetCommandSignatureHash(const CommandSignatureType type, const uint32_t stride)
	{
		size_t result = std::hash<uint8_t>()(static_cast<uint8_t>(type));
		result = Math::HashCombine(result, std::hash<uint32_t>()(stride));
		return result;
	}

	void CommandSignatureCache::Initialize()
	{
		VT_ENSURE(!m_initialized);
		m_initialized = true;

		m_signatureCache[GetCommandSignatureHash(CommandSignatureType::Draw, sizeof(DrawIndirectCommand))] = CreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, DrawIndirectCommand>();
		m_signatureCache[GetCommandSignatureHash(CommandSignatureType::DrawIndexed, sizeof(DrawIndexedIndirectCommand))] = CreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, DrawIndexedIndirectCommand>();
		m_signatureCache[GetCommandSignatureHash(CommandSignatureType::Dispatch, sizeof(DispatchIndirectCommand))] = CreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, DispatchIndirectCommand>();
	}

	ComPtr<ID3D12CommandSignature> CommandSignatureCache::GetCommandSignature(CommandSignatureType type, const uint32_t stride)
	{
		const size_t hash = GetCommandSignatureHash(type, stride);
		VT_ENSURE(m_signatureCache.contains(hash));

		return m_signatureCache.at(hash);
	}

	void CommandSignatureCache::Shutdown()
	{
		m_signatureCache.clear();
	}
}
