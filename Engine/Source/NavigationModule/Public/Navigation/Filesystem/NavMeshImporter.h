#pragma once

#include "Navigation/NavMesh/VTNavMesh.h"

#include <fstream>

namespace Volt
{
	namespace AI
	{
		class NavMeshImporter
		{
		public:
			NavMeshImporter() = delete;
			virtual ~NavMeshImporter() = delete;
			
			static bool LoadNavMeshLegacy(std::ifstream& input, Ref<dtNavMesh>& asset);

		private:
			struct NavMeshSetHeader
			{
				int magic;
				int version;
				int numTiles;
				dtNavMeshParams params;
			};

			struct NavMeshTileHeader
			{
				dtTileRef tileRef;
				int dataSize;
			};

			static bool LoadSingleNavMeshLegacy(std::ifstream& input, Ref<dtNavMesh>& asset);
		};
	}
}
