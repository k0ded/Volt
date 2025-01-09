#include "vrpch.h"

#include "Volt-Renderer/AnimatedCharacter.h"
#include <Volt-Animation/Assets/Animation.h>
#include <Volt-Animation/AnimationManager.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::AnimatedCharacter, AnimatedCharacter);

	const Vector<glm::mat4> AnimatedCharacter::SampleAnimation(uint32_t index, float aStartTime, bool looping) const
	{
		if (m_animations.find(index) == m_animations.end())
		{
			return {};
		}

		return m_animations.at(index)->SampleStartTime(aStartTime, m_skeleton, looping);
	}

	const Vector<glm::mat4> AnimatedCharacter::SampleAnimation(uint32_t index, uint32_t frameIndex) const
	{
		if (m_animations.find(index) == m_animations.end())
		{
			return {};
		}

		return m_animations.at(index)->Sample(frameIndex, m_skeleton);
	}

	const float AnimatedCharacter::GetAnimationDuration(uint32_t index) const
	{
		if (m_animations.find(index) == m_animations.end())
		{
			return 0.f;
		}

		if (!m_animations.at(index))
		{
			return 0.f;
		}

		return m_animations.at(index)->GetDuration();
	}

	const AnimatedCharacter::JointAttachment AnimatedCharacter::GetJointAttachmentFromName(const std::string& jntName) const
	{
		for (const auto& jnt : m_jointAttachments)
		{
			if (jnt.name == jntName)
			{
				return jnt;
			}
		}

		return {};
	}

	const AnimatedCharacter::JointAttachment AnimatedCharacter::GetJointAttachmentFromID(const UUID64& id) const
	{
		for (const auto& jnt : m_jointAttachments)
		{
			if (jnt.id == id)
			{
				return jnt;
			}
		}

		return {};
	}

	const bool AnimatedCharacter::HasJointAttachment(const std::string& attachmentName) const
	{
		for (const auto& jnt : m_jointAttachments)
		{
			if (jnt.name == attachmentName)
			{
				return true;
			}
		}
		return false;
	}

	void AnimatedCharacter::RemoveAnimation(uint32_t index)
	{
		if (!m_animations.contains(index))
		{
			return;
		}

		if (m_animationEvents.contains(index))
		{
			m_animationEvents.erase(index);
		}

		m_animations.erase(index);
	}

	void AnimatedCharacter::RemoveAnimationEvent(const std::string& eventName, uint32_t frame, uint32_t animationIndex)
	{
		if (!m_animations.contains(animationIndex))
		{
			VT_LOG(Error, "Trying to remove animation event from invalid animation index!");
			return;
		}

		m_animationEvents[animationIndex].erase(std::remove_if(m_animationEvents[animationIndex].begin(), m_animationEvents[animationIndex].end(), [&eventName, &frame](const auto& lhs)
		{
			return lhs.name == eventName && lhs.frame == frame;
		}));
	}

	void AnimatedCharacter::AddAnimationEvent(const std::string& eventName, uint32_t frame, uint32_t animationIndex)
	{
		if (!m_animations.contains(animationIndex))
		{
			VT_LOG(Error, "Trying to add animation event to invalid animation index!");
			return;
		}

		m_animationEvents[animationIndex].emplace_back(frame, eventName);
	}

	const int32_t AnimatedCharacter::GetAnimationIndexFromHandle(Volt::AssetHandle animationHandle)
	{
		for (const auto& [index, animation] : m_animations)
		{
			if (!animation)
			{
				continue;
			}

			if (animation->handle == animationHandle)
			{
				return index;
			}
		}

		return -1;
	}
}
