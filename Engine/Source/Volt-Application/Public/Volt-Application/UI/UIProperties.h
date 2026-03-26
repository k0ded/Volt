#pragma once
#include "Volt-Application/Config.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <CoreUtilities/Filesystem/Path.h>
#include <CoreUtilities/Containers/Vector.h>

namespace FileDialogueHelpers
{
	struct FileFilter;
}

namespace UI
{
	inline static constexpr float PROPERTY_ROW_HEIGHT = 17.f;
	inline static constexpr float PROPERTY_ROW_PADDING = 4.f;

	VTAPP_API int32_t GetAndIncrementPropertiesStackID();
	VTAPP_API String MakePropertyID();


	 VTAPP_API bool BeginProperties(const String& name = "", const glm::vec2 size = {0,0});
	 VTAPP_API void EndProperties();

	 VTAPP_API void BeginPropertyRow();
	 VTAPP_API void EndPropertyRow();

	 VTAPP_API void PropertyInfoString(const String& key, const String& info);

	 VTAPP_API bool PropertyDragFloat(const String& text, float& value, float increment, float min = 0.f, float max = 0.f, const String& toolTip = "");
	 VTAPP_API bool PropertyTextBox(const String& text, const String& value, bool readOnly = false, const String& toolTip = "");
	 VTAPP_API bool PropertyPassword(const String& text, String& value, bool readOnly = false, const String& toolTip = "");
	 VTAPP_API bool PropertyMultiline(const String& text, String& value, bool readOnly = false, const String& toolTip = "");
	 VTAPP_API bool PropertyDirectory(const String& text, Filesystem::Path& path, const Filesystem::Path& baseDir = "", const String& toolTip = "");
	 VTAPP_API bool PropertyFile(const String& text, Filesystem::Path& path, const Vector<FileDialogueHelpers::FileFilter>& fileFilter);

	 VTAPP_API bool ComboProperty(const String& text, int& currentItem, const Vector<String>& strItems, float width = 0.f);
	 VTAPP_API bool ComboProperty(const String& text, int& currentItem, const Vector<const char*>& items, float width = 0.f);

	 VTAPP_API bool PropertyAxisColor(const String& text, glm::vec3& value, float resetValue = 0.f);
	 VTAPP_API bool PropertyAxisColor(const String& text, glm::vec2& value, float resetValue = 0.f);

	 VTAPP_API bool PropertyColor(const String& text, glm::vec4& value, const String& toolTip = "");
	 VTAPP_API bool PropertyColor(const String& text, glm::vec3& value, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, bool& value, const String& toolTip = "");
	
	 VTAPP_API bool Property(const String& text, int32_t& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, uint32_t& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, int16_t& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, uint16_t& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, int8_t& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, uint8_t& value, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, double& value, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, float& value, float min = 0.f, float max = 0.f, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, glm::vec2& value, float min = 0.f, float max = 0.f, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::vec3& value, float min = 0.f, float max = 0.f, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::vec4& value, float min = 0.f, float max = 0.f, const String& toolTip = "");
	
	 VTAPP_API bool Property(const String& text, glm::uvec2& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::uvec3& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::uvec4& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, glm::ivec2& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::ivec3& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, glm::ivec4& value, uint32_t min = 0, uint32_t max = 0, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, glm::quat& value, const String& toolTip = "");

	 VTAPP_API bool Property(const String& text, const String& value, bool readOnly = false, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, String& value, bool readOnly = false, const String& toolTip = "");
	 VTAPP_API bool Property(const String& text, Filesystem::Path& path, const Filesystem::Path& baseDir = "", const String& toolTip = "");

	 VTAPP_API bool IsPropertyRowHovered();
	 VTAPP_API bool IsPropertyColumnHovered(const uint32_t column);
	 VTAPP_API void SetPropertyBackgroundColor();

}
