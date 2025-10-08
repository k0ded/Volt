#pragma once
#include "Volt-Application/Config.h"

#include <string>
#include <filesystem>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Vector.h>

struct FileFilter;

namespace UI
{
	inline static constexpr float PROPERTY_ROW_HEIGHT = 17.f;
	inline static constexpr float PROPERTY_ROW_PADDING = 4.f;

	VTAPP_API int32_t GetAndIncrementPropertiesStackID();
	VTAPP_API std::string MakePropertyID();


	 VTAPP_API bool BeginProperties(const std::string& name = "", const glm::vec2 size = {0,0});
	 VTAPP_API void EndProperties();

	 VTAPP_API void BeginPropertyRow();
	 VTAPP_API void EndPropertyRow();

	 VTAPP_API void PropertyInfoString(const std::string& key, const std::string& info);

	 VTAPP_API bool PropertyDragFloat(const std::string& text, float& value, float increment, float min = 0.f, float max = 0.f, const std::string& toolTip = "");
	 VTAPP_API bool PropertyTextBox(const std::string& text, const std::string& value, bool readOnly = false, const std::string& toolTip = "");
	 VTAPP_API bool PropertyPassword(const std::string& text, std::string& value, bool readOnly = false, const std::string& toolTip = "");
	 VTAPP_API bool PropertyMultiline(const std::string& text, std::string& value, bool readOnly = false, const std::string& toolTip = "");
	 VTAPP_API bool PropertyDirectory(const std::string& text, std::filesystem::path& path, const std::filesystem::path& baseDir = "", const std::string& toolTip = "");
	 VTAPP_API bool PropertyFile(const std::string& text, std::filesystem::path& path, const Vector<FileFilter>& fileFilter);

	 VTAPP_API bool ComboProperty(const std::string& text, int& currentItem, const Vector<std::string>& strItems, float width = 0.f);
	 VTAPP_API bool ComboProperty(const std::string& text, int& currentItem, const Vector<const char*>& items, float width = 0.f);

	 VTAPP_API bool PropertyAxisColor(const std::string& text, glm::vec3& value, float resetValue = 0.f);
	 VTAPP_API bool PropertyAxisColor(const std::string& text, glm::vec2& value, float resetValue = 0.f);

	 VTAPP_API bool PropertyColor(const std::string& text, glm::vec4& value, const std::string& toolTip = "");
	 VTAPP_API bool PropertyColor(const std::string& text, glm::vec3& value, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, bool& value, const std::string& toolTip = "");
	
	 VTAPP_API bool Property(const std::string& text, int32_t& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, uint32_t& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, int16_t& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, uint16_t& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, int8_t& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, uint8_t& value, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, double& value, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, float& value, float min = 0.f, float max = 0.f, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, glm::vec2& value, float min = 0.f, float max = 0.f, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::vec3& value, float min = 0.f, float max = 0.f, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::vec4& value, float min = 0.f, float max = 0.f, const std::string& toolTip = "");
	
	 VTAPP_API bool Property(const std::string& text, glm::uvec2& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::uvec3& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::uvec4& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, glm::ivec2& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::ivec3& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, glm::ivec4& value, uint32_t min = 0, uint32_t max = 0, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, glm::quat& value, const std::string& toolTip = "");

	 VTAPP_API bool Property(const std::string& text, const std::string& value, bool readOnly = false, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, std::string& value, bool readOnly = false, const std::string& toolTip = "");
	 VTAPP_API bool Property(const std::string& text, std::filesystem::path& path, const std::filesystem::path& baseDir = "", const std::string& toolTip = "");

	 VTAPP_API bool IsPropertyRowHovered();
	 VTAPP_API bool IsPropertyColumnHovered(const uint32_t column);
	 VTAPP_API void SetPropertyBackgroundColor();

}
