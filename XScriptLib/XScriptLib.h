#pragma once

#include "pch.h"

#define XSCRIPT_VERSION		0.7f
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

bool exportUDL(const std::wstring& udlFile, const std::wstring& autoFile);
bool exportUDL(const std::string& udlFile, const std::string& autoFile);
