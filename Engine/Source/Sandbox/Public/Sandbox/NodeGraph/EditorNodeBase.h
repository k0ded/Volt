#pragma once


#include <CoreUtilities/VoltGUID.h>

#include <string>

class EditorNodeTypeDefinitionBuilder;
class EditorNodeBuilder;

class EditorNodeTypeBase
{
public:
	EditorNodeTypeBase() = default;
	virtual ~EditorNodeTypeBase() = default;

	//should be created using EDITOR_NODE_TYPE define
	virtual VoltGUID GetTypeGUID() const = 0;
	//should be created using EDITOR_NODE_TYPE define
	virtual std::string GetTypeName() const = 0;

	// pin types definition
	virtual void MakeTypeDefinition(EditorNodeTypeDefinitionBuilder& builder) = 0;
	// create all pins using this function
	virtual void Build(EditorNodeBuilder& builder) = 0;

	VT_INLINE bool WantsRebuild() const { return m_wantsRebuild; }
	VT_INLINE void RequestRebuild() { m_wantsRebuild = true; }
private:
	bool m_wantsRebuild = true;
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
	void Build(EditorNodeBuilder& nodeBuilder) override;
	void MakeTypeDefinition(EditorNodeTypeDefinitionBuilder& builder) override;

};

