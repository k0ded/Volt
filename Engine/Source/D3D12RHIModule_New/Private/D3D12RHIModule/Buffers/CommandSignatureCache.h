#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

struct ID3D12CommandSignature;

namespace Volt::RHI
{
	enum class CommandSignatureType : uint8_t
	{
		Draw,
		DrawIndexed,
		Dispatch,
		DispatchRays,
		DispatchMesh
	};

	class CommandSignatureCache
	{
	public:
		void Initialize();
		void Shutdown();
		ComPtr<ID3D12CommandSignature> GetCommandSignature(CommandSignatureType type, const uint32_t stride);

	private:
		Map<size_t, ComPtr<ID3D12CommandSignature>> m_signatureCache;
		bool m_initialized = false;
	};

	extern CommandSignatureCache g_commandSignatureCache;
}
