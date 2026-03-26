#pragma once

#ifdef COREMODULE_DLL_EXPORT
#define VTC_API __declspec(dllexport)
#else
#define VTC_API __declspec(dllimport)
#endif
