#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/MaterialAsset.h"

#include <AssetSystem/AssetReference.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

namespace Volt
{
	class MaterialAsset;

	class VTASSETS_API MaterialCompilerSubSystem : public SubSystem
	{ 
	public:
		DECLARE_DELEGATE_OneParam(MaterialCompiledDelegate, AssetHandle);

		void Initialize() override;
		void Shutdown() override;

		void RequestMaterialCompilation(AssetReference<MaterialAsset> materialAsset);
		
		VT_INLINE MaterialCompiledDelegate& GetMaterialCompiledDelegate() { return m_materialCompiledDelegate; }

		VT_DECLARE_SUBSYSTEM("{EEB3C410-3128-486A-8F66-810CEB39314C}"_guid);
	private:
		struct CompilationJob
		{
			AssetReference<MaterialAsset> material;
		};
		
		MaterialCompiledDelegate m_materialCompiledDelegate;
	};
}
