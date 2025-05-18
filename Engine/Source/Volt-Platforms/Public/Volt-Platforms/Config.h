#pragma once

#ifdef VOLT_PLATFORMS_DLL_EXPORT
#define VTPL_API __declspec(dllexport)
#else
#define VTPL_API __declspec(dllimport)
#endif
