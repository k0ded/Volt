#include "nvpch.h"
#include "Navigation/Filesystem/NavMeshImporter.h"

namespace Volt
{
	namespace AI
	{
		static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
		static const int NAVMESHSET_VERSION = 2;

		static const int TILECACHESET_MAGIC = 'T' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'TSET';
		static const int TILECACHESET_VERSION = 2;

		bool NavMeshImporter::LoadNavMeshLegacy(std::ifstream& input, Ref<dtNavMesh>& asset)
		{
			std::streampos currentPosition = input.tellg();

			// Read header.
			NavMeshSetHeader header;
			if (!input.read(reinterpret_cast<char*>(&header), sizeof(NavMeshSetHeader)))
			{
				input.close();
				return false;
			}

			input.seekg(currentPosition);

			switch (header.magic)
			{
				case NAVMESHSET_MAGIC:
				{
					return LoadSingleNavMeshLegacy(input, asset);
				}
				case TILECACHESET_MAGIC:
				{
					//return LoadTiledNavMesh(input, asset);
				}
				default:
				{
					input.close();
					return false;
				}
			}
		}

		bool NavMeshImporter::LoadSingleNavMeshLegacy(std::ifstream& input, Ref<dtNavMesh>& asset)
		{
			// Read header.
			NavMeshSetHeader header;
			if (!input.read(reinterpret_cast<char*>(&header), sizeof(NavMeshSetHeader)))
			{
				input.close();
				return false;
			}

			if (header.magic != NAVMESHSET_MAGIC)
			{
				input.close();
				return false;
			}
			if (header.version != NAVMESHSET_VERSION)
			{
				input.close();
				return false;
			}

			dtNavMesh* mesh = dtAllocNavMesh();
			if (!mesh)
			{
				input.close();
				dtFree(mesh);
				return false;
			}
			dtStatus status = mesh->init(&header.params);
			if (dtStatusFailed(status))
			{
				input.close();
				dtFree(mesh);
				return false;
			}

			// Read tiles.
			for (int i = 0; i < header.numTiles; ++i)
			{
				NavMeshTileHeader tileHeader;
				if (!input.read(reinterpret_cast<char*>(&tileHeader), sizeof(NavMeshTileHeader)))
				{
					input.close();
					dtFree(mesh);
					return false;
				}

				if (!tileHeader.tileRef || !tileHeader.dataSize)
					break;

				unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
				if (!data) break;
				memset(data, 0, tileHeader.dataSize);
				if (!input.read(reinterpret_cast<char*>(data), tileHeader.dataSize))
				{
					dtFree(data);
					dtFree(mesh);
					input.close();
					return false;
				}

				mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
			}

			asset = CreateRef<dtNavMesh>();
			asset.reset(mesh);
			return true;
		}
	}
}
