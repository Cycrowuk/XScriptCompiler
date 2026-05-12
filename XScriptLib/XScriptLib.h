#pragma once

#include "pch.h"

#define XSCRIPT_VERSION		0.5f
#define XSCRIPT_BETA

bool compileScriptFile(const std::string& filename, const std::string& out);
bool compileScriptFile(const std::wstring& filename, const std::wstring& out);

bool decompileScriptFile(const std::string& filename, const std::string& output);
bool decompileScriptFile(const std::wstring& filename, const std::wstring& output);

bool loadData(const std::wstring& dataFile);
bool loadData(const std::string& dataFile);

bool loadXmlData(const std::string& filename, const std::string& output);
bool loadXmlData(const std::wstring& filename, const std::wstring& output);

bool exportUDL(const std::wstring& udlFile, const std::wstring& autoFile);
bool exportUDL(const std::string& udlFile, const std::string& autoFile);
