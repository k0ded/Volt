#pragma once

#ifdef VOLT_APPLICATION_DLL_EXPORT
#define VTAPP_API __declspec(dllexport)
#else
#define VTAPP_API __declspec(dllimport)
#endif
