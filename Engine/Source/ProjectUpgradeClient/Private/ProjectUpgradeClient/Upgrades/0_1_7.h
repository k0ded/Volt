#pragma once
#include "UpgradeInterface.h"

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Containers/Vector.h>
#include <filesystem>

namespace Volt
{
	class Upgrade_0_1_7 : public Upgrade
	{
	public:
		Upgrade_0_1_7(const Project& inProject);
		~Upgrade_0_1_7() override;

		bool ProcessUpgrade() override;

		size_t GetNumTotalActions() override;
		size_t GetNumActionsCompleted() override;

		std::string GetCurrentActionText() override;

	private:
		enum class UpgradeStage
		{
			Collecting,
			Converting,
			Done
		};

		void ProcessFile(AssetHandle asset);
		void ProcessAsset(AssetHandle asset);

		UpgradeStage m_currentStage;

		size_t m_numTotalActions;
		size_t m_numActionsCompleted;

		Vector<AssetHandle> m_assetsToProcess;
	};
}
