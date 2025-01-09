#pragma once

#ifdef VOLT_SCENE_DLL_EXPORT
#define VTS_API __declspec(dllexport)
#else
#define VTS_API __declspec(dllimport)
#endif
