#pragma once

#include "Volt-Renderer/BlueNoise.h"
#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Debug/DebugRenderer.h"

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetReference.h>

#include <RenderCore/TransientResourceSystem/TransientResourceAllocator.h>
#include <RenderCore/CommandBufferPool.h>
#include <RenderCore/SamplerStateCache.h>

#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Core/RHICommon.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <EventSystem/EventListener.h>

namespace Volt
{
	namespace RHI
	{
		class SamplerState;
		struct SamplerStateDesc;
	}

	class Texture2D;
	class RenderMaterial;
	class Mesh;

	struct DefaultResources
	{
		Ref<RenderMaterial> defaultMaterial;
		Ref<RenderMaterial> defaultTranslucentMaterial;
		Ref<Mesh> defaultMesh;

		RefPtr<RHI::Image> DFGLuT;
		RefPtr<RHI::Image> blackCubeTexture;
		RefPtr<RHI::Image> black1x1x1;
		RefPtr<RHI::Image> white1x1;
		RefPtr<RHI::Image> black1x1;

		RefPtr<RHI::Buffer> cubeIndexBuffer;

		VT_INLINE void Clear()
		{
			defaultMaterial = nullptr;
			defaultMesh = nullptr;

			white1x1 = nullptr;
			black1x1 = nullptr;
			DFGLuT = nullptr;
			blackCubeTexture = nullptr;
			black1x1x1 = nullptr;

			cubeIndexBuffer = nullptr;
		}
	};

	class Mesh;
	class ShaderMap;

	class AppPostFrameUpdateEvent;
	class AppPreRenderEvent;

	class VTR_API Renderer : public SubSystem, public EventListener
	{
	public:
		struct EnvironmentTextures
		{
			RefPtr<RHI::Image> diffuse;
			RefPtr<RHI::Image> specular;
		};

		Renderer();
		~Renderer() override;

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		void Initialize() override;
		void CreateBlueNoise();
		void Shutdown() override;

		static const uint32_t GetFramesInFlight();

		static const DefaultResources& GetDefaultResources();
		static EnvironmentTextures GenerateEnvironmentTextures(AssetHandle baseTextureHandle);

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{2E420D68-01AC-47D5-B7F4-F31F13D57ABF}"_guid);

	private:
		bool OnEndOfFrameUpdate(AppPostFrameUpdateEvent& event);
		bool OnPreRenderEvent(AppPreRenderEvent& event);

		void CreateDefaultResources();
		void GenerateDFGLuT();

		inline static Renderer* s_instance = nullptr;

		DefaultResources m_defaultResources;

		Scope<BlueNoise> m_blueNoise;
		Scope<SamplerStateCache> m_samplerStateCache;
		Scope<CommandBufferPool> m_commandBufferPool;
		Scope<TransientResourceAllocator> m_transientResourceAllocator;

		uint32_t m_frameIndex = 0;
	};
}
