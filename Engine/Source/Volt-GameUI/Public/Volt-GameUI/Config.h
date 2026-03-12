#pragma once

#ifdef VOLT_GAMEUI_DLL_EXPORT
#define VTGUI_API __declspec(dllexport)
#else
#define VTGUI_API __declspec(dllimport)
#endif
