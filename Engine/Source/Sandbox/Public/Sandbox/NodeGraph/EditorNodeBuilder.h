#pragma once

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <string_view>
#include <string>

enum class PinType
{
	// most nodes use this type
	Default,
	// if a node wants to draw something custom next to this pin
	Custom,
};

enum class PinDirection
{
	Input,
	Output
};

// should be created by converting 4 chars, example: 'cond'
typedef uint32_t EditorNodePinID;

constexpr uint8_t PIN_USERDATA_MAX_SIZE = 32;
typedef InlineVector<uint8_t, PIN_USERDATA_MAX_SIZE> EditorPinUserData;
struct EditorNodePinDefinition
{
	static constexpr uint8_t PIN_USERDATA_MAX_SIZE = 32;

	PinDirection direction;
	PinType pinType;

	EditorNodePinID pinID;
	std::string pinName;
	EditorPinUserData userData;
};

class EditorNodeTypeDefinitionBuilder
{
public:
	EditorNodeTypeDefinitionBuilder();
	~EditorNodeTypeDefinitionBuilder() = default;

	VT_INLINE bool UsesDynamicPins() const { return m_usesDynamicPins; }
	VT_INLINE void MarkUsesDynamicPins() { m_usesDynamicPins = true; }

	void Pin(PinDirection pinDirection, EditorNodePinID pinID, std::string_view pinName, EditorPinUserData* userData = nullptr, PinType pinType = PinType::Default);

private:
	Map<EditorNodePinID, EditorNodePinDefinition> m_inputPins;
	Map<EditorNodePinID, EditorNodePinDefinition> m_outputPins;

	bool m_usesDynamicPins;
};
