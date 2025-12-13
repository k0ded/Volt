#pragma once

#include <CoreUtilities/VoltGUID.h>

#include <string>

class EditorNodeTypeBase
{
public:
	EditorNodeTypeBase() = default;
	virtual ~EditorNodeTypeBase() = default;

	//should be created using EDITOR_NODE_TYPE define
	virtual VoltGUID GetTypeGUID() const = 0;
	//should be created using EDITOR_NODE_TYPE define
	virtual std::string GetTypeName() const = 0;

	virtual void OnCreate() {};
	virtual void Draw() {};
	virtual void OnDestroy() {};
};

#define EDITOR_NODE_TYPE(class_name, type_guid) \
	public: \
		static VoltGUID GetStaticTypeGUID() { return type_guid; } \
		VoltGUID GetTypeGUID() const override{ return GetStaticTypeGUID(); } \
		static std::string GetStaticTypeName() { return #class_name; } \
		std::string GetTypeName() const override{ return #class_name; } 

class NothingNode : public EditorNodeTypeBase
{
	EDITOR_NODE_TYPE(NothingNode, "{947F14F5-709F-4C7F-BE5C-4A24E63A241C}"_guid)
public:

};

