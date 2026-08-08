#pragma once
#include <string>
#include <vector>

// Plain C++ interface to the VirtualFileSystem — no spk.h dependency.

void         VFSHelper_SetDir(const wchar_t* dir, const wchar_t* mod = nullptr);
bool         VFSHelper_IsLoaded();
std::wstring VFSHelper_ExtractFile(const wchar_t* vfsPath, const wchar_t* tempPath);
std::wstring VFSHelper_FindText(int lang, int page, int id);
// Read a file into a wchar_t buffer, unpacking .pck format if needed.
// Returns false on failure. Buffer is null-terminated on success.
bool         VFSHelper_ReadFile(const wchar_t* path, std::vector<wchar_t>& outBuffer);
// Compress a file in-place to .pck format if it has a .pck extension.
void         VFSHelper_CompressPck(const wchar_t* path);
