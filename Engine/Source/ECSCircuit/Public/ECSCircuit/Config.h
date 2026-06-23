#pragma once

#ifdef ECSCIRCUIT_DLL_EXPORT
#define ECSCIRCUIT_API __declspec(dllexport)
#else
#define ECSCIRCUIT_API __declspec(dllimport)
#endif
