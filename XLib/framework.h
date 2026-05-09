#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef _SPKDLL
#ifdef SPK_EXPORTS
#define SPKEXPORT __declspec(dllexport)
#else
#define SPKEXPORT __declspec(dllimport)
#endif
#else
#define SPKEXPORT
#endif

#include <string>