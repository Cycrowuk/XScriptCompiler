#pragma once
// rapidxml_include.h — include rapidxml before any spk/zlib/lzma headers.
// Those libraries define macros (zlib's OF(), LZMA types) that break
// rapidxml's template parameter parsing in MSVC.
// Always include THIS file instead of rapidxml.hpp directly.

// Guard against any debug new macro
#ifdef new
#pragma push_macro("new")
#undef new
#define RAPIDXML_PUSHED_NEW
#endif

#include <rapidxml/rapidxml.hpp>

#ifdef RAPIDXML_PUSHED_NEW
#pragma pop_macro("new")
#undef RAPIDXML_PUSHED_NEW
#endif