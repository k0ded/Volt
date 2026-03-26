#pragma once

#include "Volt-Animation/Assets/Animation.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

#include <CoreUtilities/Containers/Map.h>

#include <glm/glm.hpp>

namespace Volt
{
	class VTA_API Skeleton : public Asset
	{
	public:
		struct Joint
		{
			String name;
			int32_t parentIndex = -1;

			VT_INLINE friend Archive& operator<<(Archive& archive, Joint& value)
			{
				archive << value.name;
				archive << value.parentIndex;

				return archive;
			}
		};

		struct JointAttachment
		{
			String name;
			int32_t jointIndex = -1;
			UUID64 id = 0;

			glm::vec3 positionOffset = 0.f;
			glm::quat rotationOffset = { 1.f, 0.f, 0.f, 0.f };

			inline const bool IsValid() const { return id != 0; }

			VT_INLINE friend Archive& operator<<(Archive& archive, JointAttachment& value)
			{
				archive << value.name;
				archive << value.jointIndex;
				archive << value.id;
				archive << value.positionOffset;
				archive << value.rotationOffset;

				return archive;
			}
		};

		Skeleton() = default;
		~Skeleton() override;

		inline const size_t GetJointCount() const { return m_joints.size(); }
		inline const Vector<glm::mat4>& GetInverseBindPose() const { return m_inverseBindPose; }
		inline const Vector<Animation::TRS>& GetRestPose() const { return m_restPose; }
		inline const Vector<Joint>& GetJoints() const { return m_joints; }
		inline const Vector<JointAttachment>& GetJointAttachments() const { return m_jointAttachments; }

		const JointAttachment& GetJointAttachmentFromName(StringView name) const;
		const JointAttachment& GetJointAttachmentFromID(const UUID64& id) const;
		bool HasJointAttachment(StringView name) const;

		const bool JointIsDecendantOf(int32_t jointIndex, int32_t parentIndex) const;

		const int32_t GetJointIndexFromName(const String& str);
		const String GetNameFromJointIndex(int32_t index);

		static AssetType GetStaticType() { return AssetTypes::Skeleton; }
		AssetType GetType() const override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

	private:
		friend class FbxSourceImporter;
		friend class SkeletonImporter;

		Vector<Joint> m_joints;
		Vector<JointAttachment> m_jointAttachments;
		Vector<Animation::TRS> m_restPose;
		Vector<glm::mat4> m_inverseBindPose;

		Map<String, size_t> m_jointNameToIndex;

		String m_name = "Skeleton";
	};
}
