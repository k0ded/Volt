#pragma once

#include "Volt-Physics/Config.h"

#include <SubSystem/SubSystem.h>

#include <PhysicsInterface/PhysicsCore.h>
#include <PhysicsInterface/PhysicsScene.h>

namespace Volt
{
	class VTP_API PhysicsSubSystem : public SubSystem
	{
	public:
		PhysicsSubSystem();
		~PhysicsSubSystem() override = default;

		void Initialize() override;
		void Shutdown() override;

		PhysicsCore* GetPhysicsCore() const;

		VT_DECLARE_SUBSYSTEM("{DFE20F48-A2ED-411A-B02C-0A7FCF3E48B6}"_guid);
	
	private:
		bool LoadPhysicsInterface();
		void InitializePhysicsLayers();
		
		typedef PhysicsCore*(*PFN_CreatePhysicsCore)(const PhysicsCoreCreateInfo& createInfo);
		typedef void(*PFN_DestroyPhysicsCore)(PhysicsCore* core);

		inline static constexpr const char* PHYSICS_CREATE_CORE_FUNC_NAME = "CreatePhysicsCore";
		inline static constexpr const char* PHYSICS_DESTROY_CORE_FUNC_NAME = "DestroyPhysicsCore";

		PFN_CreatePhysicsCore m_physicsCoreCreateFunc = nullptr;
		PFN_DestroyPhysicsCore m_physicsCoreDestroyFunc = nullptr;

		PhysicsCore* m_physicsCore = nullptr;
		const std::filesystem::path m_physicsModulePath;
	};
}
