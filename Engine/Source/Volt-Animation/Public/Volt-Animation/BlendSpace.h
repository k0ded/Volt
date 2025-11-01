#pragma once

#include "Volt-Animation/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Asset_New.h>

#include <glm/glm.hpp>

namespace Volt
{
	enum class BlendSpaceDimension
	{
		OneD = 0,
		TwoD
	};

	class BlendSpace : public Asset
	{
	public:
		BlendSpace() = default;
		~BlendSpace() override = default;

		inline void AddAnimation(Volt::AssetHandle animation, const glm::vec2& position) { myAnimations.emplace_back(position, animation); }
		
		inline const BlendSpaceDimension GetDimension() const { return m_dimension; }
		inline const glm::vec2& GetHorizontalValues() const { return m_horizontalValues; }
		inline const glm::vec2& GetVerticalValues() const { return m_verticalValues; }
		inline const Vector<std::pair<glm::vec2, AssetHandle>>& GetAnimations() const { return myAnimations; }

		inline void SetDimension(BlendSpaceDimension dim) { m_dimension = dim; }

		static AssetType GetStaticType() { return AssetTypes::BlendSpace; }
		virtual AssetType GetType() const override { return AssetTypes::BlendSpace; }
		uint32_t GetVersion() const override { return 1; }

	private:
		friend class BlendSpaceImporter;
		friend class BlendSpaceSerializer;

		BlendSpaceDimension m_dimension = BlendSpaceDimension::OneD;

		glm::vec2 m_horizontalValues = { -1.f, 1.f };
		glm::vec2 m_verticalValues = { -1.f, 1.f };

		Vector<std::pair<glm::vec2, AssetHandle>> myAnimations;
	};
}
