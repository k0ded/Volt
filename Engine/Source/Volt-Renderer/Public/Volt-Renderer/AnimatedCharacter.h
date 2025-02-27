#pragma once

#include "Volt-Renderer/Config.h"

#include <Volt-Core/AssetTypes.h>

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

#include <glm/glm.hpp>

#include <map>

namespace Volt
{
	class Skeleton;
	class Mesh;
	class Animation;

	class VTR_API AnimatedCharacter : public Asset
	{
	public:
		struct Event
		{
			uint32_t frame;
			std::string name;
		};
		
		struct JointAttachment
		{
			std::string name;
			int32_t jointIndex = -1;
			UUID64 id{};

			glm::vec3 positionOffset = 0.f;
			glm::quat rotationOffset = { 1.f, 0.f, 0.f, 0.f };
		};

		AnimatedCharacter() = default;
		~AnimatedCharacter() override = default;

		const Vector<glm::mat4> SampleAnimation(uint32_t index, float aStartTime, bool looping = true) const;
		const Vector<glm::mat4> SampleAnimation(uint32_t index, uint32_t frameIndex) const;
		const float GetAnimationDuration(uint32_t index) const;

		inline const std::map<uint32_t, Ref<Animation>>& GetAnimations() const { return m_animations; }
		inline const size_t GetAnimationCount() const { return m_animations.size(); }
		inline const bool HasAnimationEvents(const uint32_t animationIndex) const { return m_animationEvents.contains(animationIndex); }
		inline const Vector<Event>& GetAnimationEvents(uint32_t animationIndex) const { return m_animationEvents.at(animationIndex); }
		inline const std::map<uint32_t,Vector<Event>>& GetAnimationEventsAndIndex(uint32_t) const { return m_animationEvents; }

		inline const Vector<JointAttachment>& GetJointAttachments() const { return m_jointAttachments; };
		const JointAttachment GetJointAttachmentFromName(const std::string& name) const;
		const JointAttachment GetJointAttachmentFromID(const UUID64& id) const;
		const bool HasJointAttachment(const std::string& attachmentName) const;

		inline void SetSkeleton(Ref<Skeleton> skeleton) { m_skeleton = skeleton; }
		inline void SetSkin(Ref<Mesh> skin) { m_skin = skin; }
		inline void SetAnimation(uint32_t index, Ref<Animation> anim) { m_animations[index] = anim; }

		void RemoveAnimation(uint32_t index);
		void RemoveAnimationEvent(const std::string& name, uint32_t frame, uint32_t animationIndex);
		void AddAnimationEvent(const std::string& name, uint32_t frame, uint32_t animationIndex);

		const int32_t GetAnimationIndexFromHandle(Volt::AssetHandle handle);

		inline Ref<Mesh> GetSkin() const { return m_skin; }
		inline Ref<Skeleton> GetSkeleton() const { return m_skeleton; }

		static AssetType GetStaticType() { return AssetTypes::AnimatedCharacter; }
		AssetType GetType() override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }

	private:
		friend class AnimatedCharacterImporter;
		friend class AnimatedCharacterSerializer;

		Ref<Skeleton> m_skeleton;
		Ref<Mesh> m_skin;

		std::map<uint32_t, Ref<Animation>> m_animations;
		std::map<uint32_t, Vector<Event>> m_animationEvents;
	
		Vector<JointAttachment> m_jointAttachments;
	};
}
