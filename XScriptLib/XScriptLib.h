#pragma once

#include "pch.h"

#define XSCRIPT_VERSION		0.9f
#define XSCRIPT_BETA

bool compileScriptFile(const std::string& filename, const std::string& out);
bool compileScriptFile(const std::wstring& filename, const std::wstring& out);
bool compileScriptFile(const std::string& filename, const std::string& out, const std::vector<std::string>& defines);
bool compileScriptFile(const std::wstring& filename, const std::wstring& out, const std::vector<std::wstring>& defines);

bool decompileScriptFile(const std::string& filename, const std::string& output);
bool decompileScriptFile(const std::wstring& filename, const std::wstring& output);
bool decompileScriptFile(const std::string& filename, const std::string& output, bool useNamespace);
bool decompileScriptFile(const std::wstring& filename, const std::wstring& output, bool useNamespace);

bool loadData(const std::wstring& dataFile);
bool loadData(const std::string& dataFile);

bool loadXmlData(const std::string& filename, const std::string& output);
bool loadXmlData(const std::wstring& filename, const std::wstring& output);

// Set the X3 game installation directory — used by --builddata to load
// game data files via the VirtualFileSystem instead of a local Data\ folder.
// Optionally load a mod on top of the base game filesystem.
void setGameDir(const std::wstring& dir, const std::wstring& mod = L"");

// VFS helpers — used by ScriptDataReader without needing spk.h.
std::wstring vfsExtractFile(const std::wstring& vfsPath, const std::wstring& tempPath);
bool vfsIsLoaded();
// Look up a text string by language/page/id from the VFS text database.
std::wstring vfsFindText(int lang, int page, int id);

// Scan a directory for .xs and .xml script files, extract argument/return type
// information from their function main() headers, and register the results in
// the loaded data, then re-save the .dat file. Call AFTER loadXmlData.
bool scanScriptFiles(const std::wstring& workingDir, const std::wstring& output);

bool exportUDL(const std::wstring& udlFile, const std::wstring& autoFile);
bool exportUDL(const std::string& udlFile, const std::string& autoFile);
