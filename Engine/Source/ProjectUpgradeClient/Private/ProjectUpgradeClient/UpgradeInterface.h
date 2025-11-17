#pragma once

#include <string>

namespace Volt
{
	class Version;
	struct Project;

	class Upgrade
	{
	public:
		Upgrade() = delete;
		Upgrade(const Project& inProject) 
			: m_targetProject(inProject)
		{}
		virtual ~Upgrade() = default;

		//return true when done upgrading
		virtual bool ProcessUpgrade() = 0;

		virtual size_t GetNumTotalActions() = 0;
		virtual size_t GetNumActionsCompleted() = 0;

		virtual std::string GetCurrentActionText() = 0;

	protected:
		const Project& GetTargetProject() { return m_targetProject; }
	private:
		const Project& m_targetProject;
	};

}
