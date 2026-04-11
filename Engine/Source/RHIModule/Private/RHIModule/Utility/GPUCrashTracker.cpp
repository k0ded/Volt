#include "rhipch.h"

#include "RHIModule/Utility/GPUCrashTracker.h"

#include <CoreModule/AppVersion.h>
#include <CoreModule/GlobalCommandLine.h>
#include <CoreModule/Project/ProjectManager.h>

#include <FileSystemModule/Filesystem.h>
#include <PlatformsModule/Platform.h>

#ifdef VT_ENABLE_NV_AFTERMATH

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_Defines.h>

inline String GetAftermathErrorMessage(GFSDK_Aftermath_Result result)
{
	switch (result)
	{
		case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
			return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
		default:
			return "Aftermath Error 0x" + FormatString("{:x}", static_cast<uint32_t>(result));
	}
}

#define VT_AFTERMATH_CHECK(x) \
	[&]() \
	{ \
		GFSDK_Aftermath_Result _res = x; \
		VT_ENSURE_MSG(GFSDK_Aftermath_SUCCEED(_res), GetAftermathErrorMessage(_res)); \
	}(); \

#endif

namespace Volt::RHI
{
#ifdef VT_ENABLE_NV_AFTERMATH
	inline const uint32_t GetAftermathAPIFlag(const GraphicsAPI api)
	{
		switch (api)
		{
			case GraphicsAPI::Vulkan: return GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan;
			case GraphicsAPI::D3D12: return GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX;

			case GraphicsAPI::MoltenVk:
			case GraphicsAPI::Mock:
				break;
		}

		return GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_None;
	}
#endif

	GPUCrashTracker::~GPUCrashTracker()
	{
		if (m_initialized)
		{
			Shutdown();
		}
	}

	void GPUCrashTracker::Initialize(GraphicsAPI api)
	{
		m_initialized = true;

		m_markerAllocator.Initialize(NumMaxMarkers);
	
#ifdef VT_ENABLE_NV_AFTERMATH
		VT_AFTERMATH_CHECK(GFSDK_Aftermath_EnableGpuCrashDumps(
			GFSDK_Aftermath_Version_API,
			GetAftermathAPIFlag(api),
			GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
			GpuCrashDumpCallback,
			ShaderDebugInfoCallback,
			CrashDumpDescriptionCallback,
			ResolveMarkerCallback,
			this));
#endif
	}

	void GPUCrashTracker::HandleGPUCrash()
	{
		// Wait for the driver.
		auto tdrTerminationTimeout = std::chrono::seconds(3);
		auto start = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::milliseconds::zero();

		GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;

		while (status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed &&
			status != GFSDK_Aftermath_CrashDump_Status_Finished &&
			elapsed < tdrTerminationTimeout)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			VT_AFTERMATH_CHECK(GFSDK_Aftermath_GetCrashDumpStatus(&status));

			auto end = std::chrono::steady_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		}

		VT_FATAL_MSG(status == GFSDK_Aftermath_CrashDump_Status_Finished, "Unexpected crash dump status!");
	}

	void GPUCrashTracker::Shutdown()
	{
		m_initialized = false;

#ifdef VT_ENABLE_NV_AFTERMATH
		GFSDK_Aftermath_DisableGpuCrashDumps();
#endif
	}

	void GPUCrashTracker::WriteGPUCrashDumpToFile(const void* pGPUCrashDump, const uint32_t gpuCrashDumpSize)
	{
#ifdef VT_ENABLE_NV_AFTERMATH

		GFSDK_Aftermath_GpuCrashDump_Decoder decoder{};
		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
			GFSDK_Aftermath_Version_API, 
			pGPUCrashDump, 
			gpuCrashDumpSize, 
			&decoder));

		GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo{};
		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

		uint32_t applicationNameLength = 0;
		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(decoder, 
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, 
			&applicationNameLength));

		Vector<char> applicationName(applicationNameLength, '\0');

		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_GetDescription(
			decoder,
			GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
			uint32_t(applicationName.size()),
			applicationName.data()));

		// Need to do this due to a driver bug where multiple files may be generated.
		static uint32_t count = 0;

		Filesystem::Path filename = FormatString(L"{}-{}-{}.nv-gpudmp", String(applicationName.data()), baseInfo.pid, ++count);

		// Write the file to disk.
		{
			const Filesystem::Path filepath = ProjectManager::GetGeneratedDirectory() / "CrashDumps" / filename;
		
			if (!Filesystem::Exists(filepath.ParentPath()))
			{
				Filesystem::CreateDirectories(filepath.ParentPath());
			}

			FileHandle fileHandle = PlatformFileSystem::CreateFile(filepath);

			if (fileHandle.IsValid())
			{
				PlatformFileSystem::WriteFile(fileHandle, pGPUCrashDump, gpuCrashDumpSize);
				PlatformFileSystem::CloseFile(fileHandle);
			}
			else
			{
				VT_LOGC(Error, LogRHI, "Unable to write nv-gpudmp file to disk!");
			}
		}

		// Get and write the crash info as a json.
		{
			uint32_t jsonSize = 0;
			VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
				GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
				ShaderDebugInfoLookupCallback,
				ShaderLookupCallback,
				ShaderSourceDebugInfoLookupCallback,
				this,
				&jsonSize));
		
			Vector<char> json(jsonSize);
			VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_GetJSON(
				decoder,
				uint32_t(json.size()),
				json.data()
			));

			const Filesystem::Path filepath = ProjectManager::GetGeneratedDirectory() / "CrashDumps" / (filename + ".json");

			if (!Filesystem::Exists(filepath.ParentPath()))
			{
				Filesystem::CreateDirectories(filepath.ParentPath());
			}

			FileHandle fileHandle = PlatformFileSystem::CreateFile(filepath);

			if (fileHandle.IsValid())
			{
				PlatformFileSystem::WriteFile(fileHandle, json.data(), json.size() - 1);
				PlatformFileSystem::CloseFile(fileHandle);
			}
			else
			{
				VT_LOGC(Error, LogRHI, "Unable to write nv-gpudmp.json file to disk!");
			}
		}

		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
#endif
	}

#ifdef VT_ENABLE_NV_AFTERMATH
	void GPUCrashTracker::WriteShaderDebugInfoToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier, const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize)
	{
		const Filesystem::Path filepath = ProjectManager::GetGeneratedDirectory() / "NvShaderDebugInfo" / FormatString("shader-{:x}_{:x}.nvdgb", identifier.id[0], identifier.id[1]);

		if (!Filesystem::Exists(filepath.ParentPath()))
		{
			Filesystem::CreateDirectories(filepath.ParentPath());
		}

		FileHandle fileHandle = PlatformFileSystem::CreateFile(filepath);

		if (fileHandle.IsValid())
		{
			PlatformFileSystem::WriteFile(fileHandle, shaderDebugInfo, shaderDebugInfoSize);
			PlatformFileSystem::CloseFile(fileHandle);
		}
		else
		{
			VT_LOGC(Error, LogRHI, "Unable to write .nvdgb file to disk!");
		}
	}

	void GPUCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
	{
		std::scoped_lock lock{ m_mutex };
		WriteGPUCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
	}

	void GPUCrashTracker::OnShaderDebugInfo(const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize)
	{
		std::scoped_lock lock{ m_mutex };

		GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier{};
		VT_AFTERMATH_CHECK(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
			GFSDK_Aftermath_Version_API,
			shaderDebugInfo,
			shaderDebugInfoSize,
			&identifier
		));

		DataBuffer& dataBuffer = m_shaderDebugInfos[identifier];
		dataBuffer.Allocate(shaderDebugInfoSize);
		dataBuffer.Copy(shaderDebugInfo, shaderDebugInfoSize);

		WriteShaderDebugInfoToFile(identifier, shaderDebugInfo, shaderDebugInfoSize);
	}

	void GPUCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
	{
		addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "Volt");
		addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, VT_VERSION.ToString().c_str());
	
		const CommandLineBuilder& commandLine = GlobalCommandLine::Get();
		String commandLineString = FormatString("CommandLine: {}", commandLine.GetAsString());

		addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, commandLineString.c_str());
	}

	void GPUCrashTracker::OnResolveMarker(const void* markerData, const uint32_t markerDataSize, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker)
	{
		uint64_t markerIndex = (uint64_t)markerData;
		String& str = m_markerAllocator.GetMarker(markerIndex);
	
		resolveMarker(str.data(), static_cast<uint32_t>(str.length()));
	}

	void GPUCrashTracker::OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const
	{
		auto it = m_shaderDebugInfos.find(identifier);
		if (it == m_shaderDebugInfos.end())
		{
			return;
		}

		setShaderDebugInfo(it->second.As<void*>(), static_cast<uint32_t>(it->second.GetSize()));
	}

	void GPUCrashTracker::OnShaderLookup(const GFSDK_Aftermath_ShaderBinaryHash & shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) const
	{}

	void GPUCrashTracker::OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName & shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary) const
	{}

	void GPUCrashTracker::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(pUserData);
		gpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
	}

	void GPUCrashTracker::ShaderDebugInfoCallback(const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize, void* userData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnShaderDebugInfo(shaderDebugInfo, shaderDebugInfoSize);
	}
	
	void GPUCrashTracker::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* userData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnDescription(addDescription);
	}
	
	void GPUCrashTracker::ResolveMarkerCallback(const void* markerData, const uint32_t markerDataSize, void* userData, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnResolveMarker(markerData, markerDataSize, resolveMarker);
	}
	
	void GPUCrashTracker::ShaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* userData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnShaderDebugInfoLookup(*identifier, setShaderDebugInfo);
	}

	void GPUCrashTracker::ShaderLookupCallback(const GFSDK_Aftermath_ShaderBinaryHash * shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* userData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnShaderLookup(*shaderHash, setShaderBinary);
	}
	
	void GPUCrashTracker::ShaderSourceDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* userData)
	{
		GPUCrashTracker* gpuCrashTracker = reinterpret_cast<GPUCrashTracker*>(userData);
		gpuCrashTracker->OnShaderSourceDebugInfoLookup(*shaderDebugName, setShaderBinary);
	}
#endif

	void SimpleMarkerAllocator::Initialize(uint64_t numMaxMarkers)
	{
		m_markers.resize(numMaxMarkers);
		m_numMaxMarkers = numMaxMarkers;
	}

	uint64_t SimpleMarkerAllocator::AllocateMarker()
	{
		uint64_t markerIndex = m_nextMarker.fetch_add(1, std::memory_order::relaxed) % m_numMaxMarkers;
		return markerIndex;
	}

	String& SimpleMarkerAllocator::GetMarker(uint64_t index)
	{
		return m_markers[index];
	}
}
