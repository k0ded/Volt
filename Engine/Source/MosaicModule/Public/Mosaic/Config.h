#pragma once

#ifdef MOSAICMODULE_DLL_EXPORT
#define VTMOSAIC_API __declspec(dllexport)
#else
#define VTMOSAIC_API __declspec(dllimport)
#endif
