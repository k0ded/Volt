#include "vapch.h"

#include "Volt-Animation/BlendSpace.h"

namespace Volt
{
#if 0
	void BlendSpace::Serialize(Archive& archive)
	{
		uint32_t dimUint = static_cast<uint32_t>(m_dimension);

		archive << dimUint;

		if (archive.IsLoading())
		{
			m_dimension = static_cast<BlendSpaceDimension>(dimUint);
		}

		archive << m_horizontalValues;
		archive << m_verticalValues;
		archive << m_animations;
	}
#endif
}
