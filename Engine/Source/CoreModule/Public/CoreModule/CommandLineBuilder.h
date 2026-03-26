#pragma once

#include "CoreModule/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/String/VoltString.h>

#include <cstdint>

namespace Volt
{
	class VTC_API CommandLineBuilder
	{
	public:
		CommandLineBuilder() = default;
		CommandLineBuilder(std::initializer_list<std::pair<String, String>> initializer);

		void AddArgument(const String& key, const String& value = "");
		void BuildFromArgV(wchar_t** argList, int32_t numArgs);
		void BuildFromString(const String& string);

		String GetAsString() const;
		WString GetAsWString() const;

		VT_NODISCARD VT_INLINE size_t GetNumArgs() const { return m_arguments.size(); }
		VT_NODISCARD VT_INLINE bool IsArgDefined(const String& argKey) const { return m_arguments.contains(argKey); }
		VT_NODISCARD VT_INLINE const String& GetArgValue(const String& argKey) const { return m_arguments.at(argKey); }
		VT_NODISCARD VT_INLINE const String& GetExecutableFilepath() const { return m_executableFilepath; }

	private:
		String m_executableFilepath;
		Map<String, String> m_arguments;
	};
}
