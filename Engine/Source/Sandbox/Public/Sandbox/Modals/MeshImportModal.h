#pragma once

#include "Sandbox/Modals/Modal.h"

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/SourceAssetImporter.h>

#include <CoreUtilities/Containers/Vector.h>

class MeshImportModal final : public Modal
{
public:
	MeshImportModal(const String& strId);
	~MeshImportModal() override = default;

	void SetImportMeshes(const Vector<Filesystem::Path>& filePaths);
	VT_INLINE void SetDestinationDirectory(const Filesystem::Path& destinationDirectory) { m_destinationDirectory = destinationDirectory; }

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	struct ImportOptions
	{
		bool isSkeletalMesh = false;
		bool combineMeshes = false;
		bool importVertexColors = false;
		Volt::AssetHandle targetSkeleton = Volt::Asset::Null();

		bool importAnimations = true;

		glm::vec3 translation = 0.f;
		glm::vec3 rotation = 0.f;
		glm::vec3 scale = 1.f;

		bool convertScene = true;

		bool importMaterial = true;
	};

	enum class ImportType
	{
		StaticMesh,
		SkeletalMesh,
		Animation
	};

	const String GetStringFromImportType(const ImportType importType);
	void GetInformationOfCurrentMesh();

	void Import(const Filesystem::Path& importPath, const Filesystem::Path& destinationDirectory);
	void Clear();

	ImportType m_currentImportType = ImportType::StaticMesh;
	ImportOptions m_importOptions{};
	Volt::SourceAssetFileInformation m_fileInformation;

	Filesystem::Path m_destinationDirectory;
	Vector<Filesystem::Path> m_importFilePaths;
};
