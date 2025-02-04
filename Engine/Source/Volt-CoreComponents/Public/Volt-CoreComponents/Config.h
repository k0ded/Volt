#pragma once

#ifdef VOLT_CORECOMPONENTS_DLL_EXPORT
#define VTCC_API __declspec(dllexport)
#else
#define VTCC_API __declspec(dllimport)
#endif
