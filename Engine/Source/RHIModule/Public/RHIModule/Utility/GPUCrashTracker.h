#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"

#include <CoreModule/DataBuffer.h>

#ifdef VT_ENABLE_NV_AFTERMATH
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#endif

#ifdef VT_ENABLE_NV_AFTERMATH
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
	if (lhs.id[0] == rhs.id[0])
	{
		return lhs.id[1] < rhs.id[1];
	}

	return lhs.id[0] < rhs.id[0];
}

inline bool operator==(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
	return lhs.id[0] == rhs.id[0] && lhs.id[1] == rhs.id[1];
}
#endif

namespace Volt::RHI
{
	class SimpleMarkerAllocator
	{
	public:
		SimpleMarkerAllocator() = default;

		void Initialize(uint64_t numMaxMarkers);

		VTRHI_API uint64_t AllocateMarker();
		VTRHI_API String& GetMarker(uint64_t index);

	private:
		std::atomic<uint64_t> m_nextMarker = 0;
		uint64_t m_numMaxMarkers = 0;
		Vector<String> m_markers;
	};

	class VTRHI_API GPUCrashTracker
	{
	public:
		inline static constexpr uint64_t NumMaxMarkers = 10'000;

		GPUCrashTracker() = default;
		~GPUCrashTracker();

		void Initialize(GraphicsAPI api);
		void Shutdown();

		void HandleGPUCrash();

		VT_INLINE SimpleMarkerAllocator& GetMarkerAllocator() { return m_markerAllocator; }

	private:
		void WriteGPUCrashDumpToFile(const void* pGPUCrashDump, const uint32_t gpuCrashDumpSize);

		// Callbacks
#ifdef VT_ENABLE_NV_AFTERMATH
		void WriteShaderDebugInfoToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier, const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize);

		void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);
		void OnShaderDebugInfo(const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize);
		void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);
		void OnResolveMarker(const void* markerData, const uint32_t markerDataSize, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker);

		void OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const;
		void OnShaderLookup(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) const;
		void OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

		static void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
		static void ShaderDebugInfoCallback(const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize, void* userData);
		static void CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* userData);
		static void ResolveMarkerCallback(const void* markerData, const uint32_t markerDataSize, void* userData, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker);

		static void ShaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* userData);
		static void ShaderLookupCallback(const GFSDK_Aftermath_ShaderBinaryHash* shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* userData);
		static void ShaderSourceDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* userData);

		std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, DataBuffer> m_shaderDebugInfos;
#endif

		SimpleMarkerAllocator m_markerAllocator;
		mutable std::mutex m_mutex;
		bool m_initialized = false;
	};
}
