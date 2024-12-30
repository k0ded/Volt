#pragma once

#ifdef VOLT_PHYSICS_DLL_EXPORT
#define VTP_API __declspec(dllexport)
#else
#define VTP_API __declspec(dllimport)
#endif
