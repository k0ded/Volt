#pragma once

#include <Volt-Core/Version.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Pointers/Ref.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltAssert.h>

namespace Volt
{
	class Upgrade;
	struct Project;

	class UpgradesRegistry
	{
	public:
		struct UpgradeInfo
		{
			std::function<Ref<Upgrade>(const Project&)> CreateFunction;
			Volt::Version TargetVersion;
		};

		template<typename T>
		inline bool RegisterUpgrade(Volt::Version targetVersion)
		{
			if (m_registry.contains(targetVersion))
			{
				return false;
			}

			UpgradeInfo upgradeInfo{};
			upgradeInfo.TargetVersion = targetVersion;
			upgradeInfo.CreateFunction = [](const Project& inProject)
			{
				return CreateRef<T>(inProject);
			};

			m_registry[targetVersion] = upgradeInfo;
			return true;
		}

		inline Ref<Upgrade> CreateUpgrade(Volt::Version version, const Project& inProject)
		{
			VT_ENSURE(m_registry.contains(version));
			return m_registry.at(version).CreateFunction(inProject);
		}

		inline const auto& GetRegistry() { return m_registry; }
		inline const auto& GetUpgradeInfo(const Volt::Version version) { return m_registry.at(version); }

		static UpgradesRegistry& Get();

	private:
		Map<Volt::Version, UpgradeInfo> m_registry;
	};
}

#define UNPACK(...) __VA_ARGS__
#define REGISTER_UPGRADE(targetVersion, classType) inline static bool classType ## _upgrade_registered = Volt::UpgradesRegistry::Get().RegisterUpgrade<classType>(targetVersion)
