#pragma once

#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Containers/Vector.h>
#include <string>

namespace Volt::RHI
{
	struct VTRHI_API ShaderPermutationConfig
	{
	public:
		void AddPermutation(const std::string& name, const std::string& value);
		VT_INLINE void SetPermutationIndex(size_t index) { m_permutationIndex = index; }

		VT_NODISCARD VT_INLINE const Vector<std::string>& GetPermutations() const { return m_permutationDefines; }
		VT_NODISCARD VT_INLINE size_t GetPermutationIndex() const { return m_permutationIndex; }
		VT_NODISCARD Vector<std::wstring> GetPermutationsWideStr() const;

	private:
		// Stringified versions of the permutations.
		Vector<std::string> m_permutationDefines;
		size_t m_permutationIndex = 0;
	};
}
