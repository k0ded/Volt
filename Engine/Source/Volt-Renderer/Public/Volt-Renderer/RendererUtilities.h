#pragma once

#include "Volt-Renderer/BlueNoise.h"
#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Debug/DebugRenderer.h"

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetReference.h>

#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Core/RHICommon.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Pointers/Unique.h>

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

		IntRef<RHI::Image> DFGLuT;
		IntRef<RHI::Image> blackCubeTexture;
		IntRef<RHI::Image> black1x1x1;
		IntRef<RHI::Image> white1x1;
		IntRef<RHI::Image> black1x1;

		IntRef<RHI::Buffer> cubeIndexBuffer;

		IntRef<RHI::Shader> generateMipMapsShader;

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
			generateMipMapsShader = nullptr;
		}
	};

	class Mesh;
	class ShaderMap;

	class AppPostFrameUpdateEvent;
	class AppPreRenderEvent;

	class VTR_API RendererUtilities : public SubSystem
	{
	public:
		struct EnvironmentTextures
		{
			IntRef<RHI::Image> diffuse;
			IntRef<RHI::Image> specular;
		};

		RendererUtilities();
		~RendererUtilities() override;

		RendererUtilities(const RendererUtilities&) = delete;
		RendererUtilities& operator=(const RendererUtilities&) = delete;

		void Initialize() override;
		void CreateBlueNoise();
		void Shutdown() override;

		static const uint32_t GetFramesInFlight();

		static const DefaultResources& GetDefaultResources();
		static EnvironmentTextures GenerateEnvironmentTextures(AssetHandle baseTextureHandle);

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{2E420D68-01AC-47D5-B7F4-F31F13D57ABF}"_guid);

	private:
		void CreateDefaultResources();
		void GenerateDFGLuT();

		inline static RendererUtilities* s_instance = nullptr;

		DefaultResources m_defaultResources;

		Unique<BlueNoise> m_blueNoise;
	};
}
