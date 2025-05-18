#pragma once

#include "CoreUtilities/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>

#include <string>
#include <cstdint>

namespace Volt
{
	class VTCOREUTIL_API CommandLineBuilder
	{
	public:
		void AddArgument(const std::string& key, const std::string& value = "");
		void BuildFromArgV(wchar_t** argList, int32_t numArgs);

		std::string GetAsString() const;
		std::wstring GetAsWString() const;

		VT_NODISCARD VT_INLINE size_t GetNumArgs() const { return m_arguments.size(); }
		VT_NODISCARD VT_INLINE bool IsArgDefined(const std::string& argKey) const { return m_arguments.contains(argKey); }
		VT_NODISCARD VT_INLINE const std::string& GetArgValue(const std::string& argKey) const { return m_arguments.at(argKey); }

	private:
		vt::map<std::string, std::string> m_arguments;
	};
}
