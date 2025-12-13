#pragma once

#include "Sandbox/NodeGraph/EditorNodeBase.h"

class AnimationGraphNode : public EditorNodeTypeBase
{
public:
	AnimationGraphNode() = default;
	virtual ~AnimationGraphNode() override = default;

	virtual void OnCreate() override;
	virtual void Draw() override;
	virtual void OnDestroy() override;

	EDITOR_NODE_TYPE(AnimationGraphNode, "{DCA3F0C3-E255-4014-B21F-240F613D2C37}"_guid)
};
