#pragma once

#ifdef FILESYSTEMMODULE_DLL_EXPORT
#define VTFS_API __declspec(dllexport)
#else
#define VTFS_API __declspec(dllimport)
#endif
