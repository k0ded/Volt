#pragma once

#ifdef ENTITYSYSTEMMODULE_DLL_EXPORT
#define VTES_API __declspec(dllexport)
#else
#define VTES_API __declspec(dllimport)
#endif

#ifdef VT_ENALBE_ENTITY_VALIDATION
#define VT_ENTITY_VALIDATE(x) VT_ASSERT(x)
#else
#define VT_ENTITY_VALIDATE(x)
#endif
