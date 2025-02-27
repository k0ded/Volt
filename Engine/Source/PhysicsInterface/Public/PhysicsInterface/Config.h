#pragma once

#ifdef PHYSICSINTERFACE_DLL_EXPORT
#define VTPI_API __declspec(dllexport)
#else
#define VTPI_API __declspec(dllimport)
#endif
