#pragma once

#include "Volt-Animation/Config.h"

#include "Volt-Animation/Assets/AssetTypes.h"

#include <AssetSystem/Asset.h>

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
		struct AnimationData
		{
			AssetHandle handle;
			glm::vec2 value;

			VT_INLINE friend Archive& operator<<(Archive& archive, AnimationData& value)
			{
				archive << value.handle;
				archive << value.value;
				return archive;
			}
		};

		BlendSpace() = default;
		~BlendSpace() override = default;

		inline void AddAnimation(Volt::AssetHandle animation, const glm::vec2& position) { m_animations.emplace_back(animation, position); }
		
		inline const BlendSpaceDimension GetDimension() const { return m_dimension; }
		inline const glm::vec2& GetHorizontalValues() const { return m_horizontalValues; }
		inline const glm::vec2& GetVerticalValues() const { return m_verticalValues; }
		inline const Vector<AnimationData>& GetAnimations() const { return m_animations; }

		inline void SetDimension(BlendSpaceDimension dim) { m_dimension = dim; }

		static AssetType GetStaticType() { return AssetTypes::BlendSpace; }
		virtual AssetType GetType() const override { return AssetTypes::BlendSpace; }
		uint32_t GetVersion() const override { return 1; }
		VTA_API void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

	private:
		friend class BlendSpaceImporter;

		BlendSpaceDimension m_dimension = BlendSpaceDimension::OneD;

		glm::vec2 m_horizontalValues = { -1.f, 1.f };
		glm::vec2 m_verticalValues = { -1.f, 1.f };

		Vector<AnimationData> m_animations;
	};
}
