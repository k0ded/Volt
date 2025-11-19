#pragma once

#include <CoreUtilities/VoltGUID.h>

#include <string>

class EditorNodeTypeBase
{
public:
	EditorNodeTypeBase() = default;
	virtual ~EditorNodeTypeBase() = default;

	virtual void OnCreate() {};
	virtual void Draw() {};
	virtual void OnDestroy() {};
};

#define EDITOR_NODE_TYPE(class_name, type_guid) \
	public: \
		static VoltGUID GetStaticTypeGUID() { return type_guid; } \
		VoltGUID GetTypeGUID() const { return GetStaticTypeGUID(); } \
		static std::string GetStaticTypeName() { return #class_name; } \
		std::string GetTypeName() const { return #class_name; } 

