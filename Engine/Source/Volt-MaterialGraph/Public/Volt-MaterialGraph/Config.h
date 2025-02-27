#pragma once

#ifdef VOLT_MATERIALGRAPH_DLL_EXPORT
#define VTMG_API __declspec(dllexport)
#else
#define VTMG_API __declspec(dllimport)
#endif
