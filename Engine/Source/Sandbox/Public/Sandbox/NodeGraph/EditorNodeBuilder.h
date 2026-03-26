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
	PinDirection direction;
	PinType pinType;

	EditorNodePinID pinID;
	String pinName;
	EditorPinUserData userData;
};

struct EditorNodeTypeDefinition
{
	Map<EditorNodePinID, EditorNodePinDefinition> inputPins;
	Map<EditorNodePinID, EditorNodePinDefinition> outputPins;
	bool usesDynamicPins = false;
};

class EditorNodeTypeDefinitionBuilder
{
public:
	EditorNodeTypeDefinitionBuilder();
	~EditorNodeTypeDefinitionBuilder() = default;

	VT_INLINE bool UsesDynamicPins() const { return m_nodeTypeDefinition.usesDynamicPins; }
	VT_INLINE void MarkUsesDynamicPins() { m_nodeTypeDefinition.usesDynamicPins = true; }

	void Pin(PinDirection pinDirection, EditorNodePinID pinID, const char* pinName, EditorPinUserData* userData = nullptr, PinType pinType = PinType::Default);

	EditorNodeTypeDefinition&& MoveDefinition() { return std::move(m_nodeTypeDefinition); }
private:
	EditorNodeTypeDefinition m_nodeTypeDefinition;
};
