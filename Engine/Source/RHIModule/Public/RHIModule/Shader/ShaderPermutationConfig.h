#pragma once

#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Containers/Vector.h>
#include <string>

namespace Volt::RHI
{
	struct VTRHI_API ShaderPermutationConfig
	{
	public:
		void AddPermutation(const String& name, const String& value);
		VT_INLINE void SetPermutationIndex(size_t index) { m_permutationIndex = index; }

		VT_NODISCARD VT_INLINE const Vector<String>& GetPermutations() const { return m_permutationDefines; }
		VT_NODISCARD VT_INLINE size_t GetPermutationIndex() const { return m_permutationIndex; }
		VT_NODISCARD Vector<WString> GetPermutationsWideStr() const;

	private:
		// Stringified versions of the permutations.
		Vector<String> m_permutationDefines;
		size_t m_permutationIndex = 0;
	};
}
