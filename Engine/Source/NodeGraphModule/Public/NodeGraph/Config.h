#pragma once

#ifdef NODEGRAPHMODULE_DLL_EXPORT
#define VTNODE_API __declspec(dllexport)
#else
#define VTNODE_API __declspec(dllimport)
#endif
