#pragma once

#include "Volt-Assets/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

#include <glm/glm.hpp>

namespace Volt
{
	struct MSDFData;
	class Texture2D;

	class Font : public Asset
	{
	public:
		struct FontHeader
		{
			uint32_t width = 0;
			uint32_t height = 0;
		};

		//TODO: temporary change to add API for circuit
		VTASSETS_API Font() = default;
		VTASSETS_API ~Font() override;

		VTASSETS_API void Initialize(const std::filesystem::path& filePath);

		VTASSETS_API float GetStringWidth(const std::string& string, const glm::vec2& scale, float maxWidth);
		VTASSETS_API float GetStringHeight(const std::string& string, const glm::vec2& scale, float maxWidth);

		inline Ref<Texture2D> GetAtlas() const { return myAtlas; }
		inline MSDFData* GetMSDFData() const { return myMSDFData; }

		static AssetType GetStaticType() { return AssetTypes::Font; }
		AssetType GetType() const override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }

	private:
		MSDFData* myMSDFData = nullptr;
		Ref<Texture2D> myAtlas;
	};
}
