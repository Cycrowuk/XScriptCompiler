// RARStubs.cpp — stub implementations of unrar functions.
// libspk includes Packages.cpp which references unrar, but XScriptCompiler
// does not use RAR archive functionality. These stubs satisfy the linker
// without requiring the unrar library to be present.

#include "pch.h"

extern "C"
{
    void* __cdecl RAROpenArchiveEx(void* data)   { return nullptr; }
    int   __cdecl RARCloseArchive(void* hArc)    { return 0; }
    int   __cdecl RARReadHeaderEx(void* hArc, void* header) { return 1; }
    int   __cdecl RARProcessFileW(void* hArc, int op, wchar_t* dest, wchar_t* name) { return 1; }
}
