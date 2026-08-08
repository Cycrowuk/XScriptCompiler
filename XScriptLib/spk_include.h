#pragma once
// spk_include.h — isolated wrapper for the libspk umbrella header.
//
// spk.h pulls in zlib, lzma (7Decoder.h), and other libraries that define
// macros which break rapidxml's template parsing if included first.
// By including this wrapper AFTER pch.h (which already compiled rapidxml
// cleanly), the spk headers compile without affecting rapidxml.
//
// Only include this in .cpp files that actually need libspk functionality
// (e.g. VFSHelper.cpp, XScriptLib.cpp). Do NOT add it to pch.h.

#include "spk.h"
