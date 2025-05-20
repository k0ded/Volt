#pragma once

#include "EntitySystem/Scripting/ECSSystemRegistry.h"

namespace env
{
	struct VariableUpdate
	{
		float deltaTime;
	};
	VT_REGISTER_ECS_ENV_TYPE(VariableUpdate);

	struct FixedUpdate
	{
		float deltaTime;
	};
	VT_REGISTER_ECS_ENV_TYPE(FixedUpdate);
}
