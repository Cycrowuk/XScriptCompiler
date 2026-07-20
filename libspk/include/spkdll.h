#ifndef __SPKDLL_H__
#define __SPKDLL_H__

#ifdef _SPKDLL
 #ifdef SPK_EXPORTS
 #define SPKEXPORT __declspec(dllexport)
 #else
 #define SPKEXPORT __declspec(dllimport)
 #endif
#else
#define SPKEXPORT
#endif

#endif //__SPKDLL_H__
