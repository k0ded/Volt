#pragma once

#ifdef VOLT_ANIMATION_DLL_EXPORT
#define VTA_API __declspec(dllexport)
#else
#define VTA_API __declspec(dllimport)
#endif
