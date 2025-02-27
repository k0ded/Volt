#pragma once

#ifdef VOLT_RENDERER_DLL_EXPORT
#define VTR_API __declspec(dllexport)
#else
#define VTR_API __declspec(dllimport)
#endif

