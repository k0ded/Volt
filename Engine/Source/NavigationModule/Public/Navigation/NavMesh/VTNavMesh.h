#pragma once

#include "Navigation/Core/CoreInterfaces.h"

#include "Navigation/NavMesh/DtNavMesh.h"
#include "Navigation/Crowd/DtCrowd.h"

#include <Volt-Renderer/Mesh/Mesh.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset.h>

namespace Volt
{
	namespace AI
	{
		class NavMesh : public Asset
		{
		public:
			NavMesh() = default;
			virtual ~NavMesh() = default;

			void Initialize(Ref<dtNavMesh> navmesh, Ref<dtTileCache> tilecache = nullptr);

			Ref<DtNavMesh>& GetNavMesh() { return myNavMesh; }
			Ref<DtCrowd>& GetCrowd() { return myCrowd; }

			static AssetType GetStaticType() { return AssetTypes::NavMesh; }
			AssetType GetType() const override { return GetStaticType(); }
			uint32_t GetVersion() const override { return 1; }

		private:
			friend class NavigationSystem;

			void Update(float deltaTime);

			Ref<DtNavMesh> myNavMesh = nullptr;
			Ref<DtCrowd> myCrowd = nullptr;
		};
	}
}
