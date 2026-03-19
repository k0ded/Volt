#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/Shader/ShaderMap.h"
#include "RenderCore/Shader/PipelineStateCache.h"

#include <SubSystem/SubSystem.h>

#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/Pointers/Unique.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTRC_API, LogShaderSubSystem, LogVerbosity::Trace);

namespace Volt
{
	namespace RHI
	{
		class ShaderCompiler;
		class ShaderCache;
	}

	class VTRC_API ShaderSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
		void Shutdown() override;

		VT_DECLARE_SUBSYSTEM("{B017EA3B-6D55-46B8-BEDE-A299C3580B4E}"_guid);
	
	private:
		void LoadRegisteredShaders();

		RefPtr<RHI::ShaderCompiler> m_shaderCompiler;
		RefPtr<RHI::ShaderCache> m_shaderCache;
		Unique<ShaderMap> m_shaderMap;
		Unique<PipelineStateCache> m_pipelineStateCache;
	};
}
