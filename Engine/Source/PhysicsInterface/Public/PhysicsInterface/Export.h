#pragma once

#include "PhysicsInterface/PhysicsCore.h"
#include "PhysicsInterface/PhysicsScene.h"

#include <CoreUtilities/Core.h>

#define VT_EXPORT_DLL __declspec(dllexport)

extern "C"
{
	VT_EXPORT_DLL Volt::PhysicsCore* CreatePhysicsCore(const Volt::PhysicsCoreCreateInfo& createInfo);
	VT_EXPORT_DLL void DestroyPhysicsCore(Volt::PhysicsCore* core);
}

#define VT_REGISTER_PHYSICS_INTERFACE(physicsCoreType) \
VT_EXPORT_DLL Volt::PhysicsCore* CreatePhysicsCore(const Volt::PhysicsCoreCreateInfo& createInfo) \
{																									 \
	return new physicsCoreType(createInfo);															 \
}																									 \
																									 \
VT_EXPORT_DLL void DestroyPhysicsCore(Volt::PhysicsCore* core)										 \
{																									 \
	delete core;																					 \
}																									 \
