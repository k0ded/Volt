#pragma once

#ifdef VOLT_IMGUI_DLL_EXPORT
#define VTIMGUI_API __declspec(dllexport)
#else
#define VTIMGUI_API __declspec(dllimport)
#endif
