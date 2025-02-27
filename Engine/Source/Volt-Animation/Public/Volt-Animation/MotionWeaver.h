#pragma once

#include "Volt-Animation/Config.h"

#include <AssetSystem/AssetHandle.h>
#include <CoreUtilities/Containers/Vector.h>

#include <glm/mat4x4.hpp>

namespace Volt
{
	class Animation;
	class MotionWeaveDatabase;
	class Skeleton;
	class VTA_API MotionWeaver
	{
	public:
		MotionWeaver(Ref<MotionWeaveDatabase> database);
		~MotionWeaver() = default;

		static Ref<MotionWeaver> Create(AssetHandle databaseHandle);

		void SetDatabase(Ref<MotionWeaveDatabase> database);

		void Update(float deltaTime);

		Vector<glm::mat4x4> Sample();
		
		VT_NODISCARD VT_INLINE Weak<Skeleton> GetSkeleton() const { return m_Skeleton; }

	private:
		Ref<MotionWeaveDatabase> m_Database;
		Vector<Ref<Animation>> m_Animations;
		Ref<Skeleton> m_Skeleton;
		
		float m_Time;
	};

}
