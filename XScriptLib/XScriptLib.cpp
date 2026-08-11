// XScriptLib.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "CScriptParser.h"
#include "CScript.h"
#include "CScriptData.h"
#include "ParseFail.h"
#include "ScriptRead.h"
#include "Utils.h"
#include "XScriptUDL.h"
#include "VFSHelper.h"

#include <locale>
#include <codecvt>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <set>
#include <io.h>

#include "rapidxml_include.h"

using namespace XScript;

CScriptData* g_scriptData;

void setGameDir(const std::wstring& dir, const std::wstring& mod)
{
	VFSHelper_SetDir(dir.c_str(), mod.empty() ? nullptr : mod.c_str());
	std::wcout << L"\tVirtualFileSystem loading from: " << dir << std::endl;
	if (!mod.empty())
		std::wcout << L"\tMod: " << mod << std::endl;
}

bool vfsIsLoaded()
{
	return VFSHelper_IsLoaded();
}

std::wstring vfsExtractFile(const std::wstring& vfsPath, const std::wstring& tempPath)
{
	return VFSHelper_ExtractFile(vfsPath.c_str(), tempPath.c_str());
}

std::wstring vfsFindText(int lang, int page, int id)
{
	return VFSHelper_FindText(lang, page, id);
}

std::string convertDataType(DataTypes dt)
{
	switch (dt)
	{
	case DataTypes::Constant:
		return "Constant";
	case DataTypes::Null:
		return "NULL";
	case DataTypes::Number:
		return "Integer";
	case DataTypes::String:
		return "String";
	case DataTypes::ObjectCommand:
		return "ObjectCommand";
	case DataTypes::ParDef:
		return "ScriptDefinition";
	case DataTypes::Race:
		return "Race";
	case DataTypes::Table:
		return "Table";
	case DataTypes::Variable:
		return "Variable";
	case DataTypes::Ship:
		return "ShipObject";
	}

	return "Unknown";
}

void displayWarning(const Warnings& warning)
{
	std::wstring string_to_convert;

	//setup converter
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	std::string sLine = converter.to_bytes(warning.line);

	std::stringstream str;
	size_t colPos = warning.colPos;

	std::stringstream strm;
	strm << "[" << converter.to_bytes(warning.file) << ":" << warning.linePos << ":" << colPos << "]";
	str << std::setw(12) << strm.str() << " ";

	std::cout << "Compile Warning [#" << static_cast<unsigned int>(warning.warning) << "]: " << str.str();

	switch (warning.warning)
	{
	case ParseWarnings::InvalidDataType:
		std::cout << "- Argument datatype mismatch, argument " << std::stoi(warning.data[1]) << " (" << converter.to_bytes(warning.data[0]) << ") Expected: " << converter.to_bytes(warning.data[3]);
		if (!warning.data[2].empty())
			std::cout << ", Got: " << converter.to_bytes(warning.data[2]);
		break;
	case ParseWarnings::InvalidObjectDataType:
		std::cout << "- Object '" << converter.to_bytes(warning.data[0]) << "' datatype mismatch, Expected: " << converter.to_bytes(warning.data[1]);
		if (!warning.data[2].empty())
			std::cout << ", Got: " << converter.to_bytes(warning.data[2]);
		break;
	case ParseWarnings::NullVariable:
		std::cout << "- Variable '" << converter.to_bytes(warning.data[0]) << "' has not be initilised";
		break;
	case ParseWarnings::InvalidArrayValue:
		std::cout << "- Array subscript '" << converter.to_bytes(warning.data[0]) << "' must be an integer";
		if (!warning.data[1].empty())
			std::cout << ", Got: " << converter.to_bytes(warning.data[1]);
		break;
	case ParseWarnings::InvalidExpressionOperator:
		std::cout << "- Operator '" << converter.to_bytes(warning.data[0]) << "' is incompitble with non-numeric values";
		break;
	case ParseWarnings::InvalidConstantGroup:
		std::cout << "- Invalid Constant '" << converter.to_bytes(warning.data[0]) << "' for argument " << std::stoi(warning.data[1]) << ", Expected Group: " << converter.to_bytes(warning.data[2]);
		break;
	}

	std::cout << std::endl;
	std::cout << "\t" + sLine << std::endl;
	std::cout << "\t";
	for (int i = 0; i < colPos; i++)
		std::cout << " ";
	std::cout << "^" << std::endl;
}

int displayWarnings(XScript::CScriptParser& parser)
{
	if (!parser.hasWarnings())
		return 0;

	std::cout << std::endl;

	auto& warningData = parser.warnings();
	for (auto itr = warningData.cbegin(); itr != warningData.cend(); itr++)
		displayWarning(*itr);

	return static_cast<int>(parser.warnings().size());
}
int displayError(XScript::CScriptParser& parser, const std::wstring& line)
{
	if (parser.errorData().empty())
		return 0;

	std::cout << std::endl;

	auto& errorData = parser.errorData();
	for (auto itr = errorData.cbegin(); itr != errorData.cend(); itr++)
	{
		std::wstring string_to_convert;

		//setup converter
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;

		std::string sLine = converter.to_bytes((*itr)->line());

		std::stringstream str;
		size_t colPos = (*itr)->startingPos();

		// some errors display the end of the line not the start
		switch ((*itr)->error())
		{
		case ParseErrors::MissingSemiColonEnd:
			colPos = (*itr)->endingPos();
			break;
		}


		std::stringstream strm;
		strm << "[" << converter.to_bytes((*itr)->file()) << ":" << (*itr)->linePos() << ":" << colPos << "]";
		str << std::setw(12) << strm.str() << " ";

		std::cout << "Compile Error [#" << static_cast<unsigned int>((*itr)->error()) << "]: " << str.str();

		switch ((*itr)->error())
		{
		case ParseErrors::InternalFunctionError:
			std::cout << "- Internal error '" << std::to_string(static_cast<int>((*itr)->error())) << "'";
			break;
		case ParseErrors::TooMuchData:
			std::cout << "- Superfluous data '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::FunctionWithoutRetVar:
			std::cout << "- Function call '" << converter.to_bytes((*itr)->data(0)) << "' has no return value";
			break;
		case ParseErrors::FunctionRequiresRetVar:
			std::cout << "- Function call '" << converter.to_bytes((*itr)->data(0)) << "' requires a return value";
			break;
		case ParseErrors::DataBeforeEndArray:
			std::cout << "- Missing end array ']' before value '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidKeywordDollar:
			std::cout << "- Invalid keyword '" << converter.to_bytes((*itr)->data(0)) << "' cant contain '$'";
			break;
		case ParseErrors::InvalidKeywordIntStart:
			std::cout << "- Invalid keyword '" << converter.to_bytes((*itr)->data(0)) << "' cant start with integer";
			break;
		case ParseErrors::InvalidArgumentType:
			std::cout << "- Invalid argument type, argument " << std::stoi((*itr)->data(1)) << " (" << converter.to_bytes((*itr)->data(0)) << ") Valid=" << converter.to_bytes((*itr)->data(2));
			break;
		case ParseErrors::InvalidVariableName:
			std::cout << "- Invalid variable name '" << converter.to_bytes((*itr)->data(0)) << "' cant start with an integer";
			break;
		case ParseErrors::InvalidArgumentDataType:
			std::cout << "- Invalid argument datatype, argument " << std::stoi((*itr)->data(1)) << " (" << converter.to_bytes((*itr)->data(0)) << ") Expected: " << convertDataType(static_cast<DataTypes>(std::stoi((*itr)->data(3))));
			if (std::stoi((*itr)->data(2)) >= 0 && std::stoi((*itr)->data(2)) != static_cast<int>(DataTypes::Unknown))
				std::cout << ", Got: " << convertDataType(static_cast<DataTypes>(std::stoi((*itr)->data(2))));
			break;
		case ParseErrors::InvalidArray:
			std::cout << "- Invalid array, it must contain an assignment (either before or after)";
			break;
		case ParseErrors::InvalidNumber:
			std::cout << "- Invalid integer '" << converter.to_bytes((*itr)->data(0)) << "' must only contain numbers";
			break;
		case ParseErrors::InvalidVariable:
			std::cout << "- Invalid variable '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidReturnValue:
			std::cout << "- Invalid Return Value '" << converter.to_bytes((*itr)->data(0)) << "' must be a variable";
			break;
		case ParseErrors::InvalidSymbol:
			std::cout << "- Invalid symbol '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidAssignmentVariable:
			std::cout << "- Invalid assignment variable '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidExpressionBrackets:
			std::cout << "- Invalid Expression, brackets must be seeprated by symbols";
			break;
		case ParseErrors::InvalidExpressionUnaryOperator:
			std::cout << "- Invalid Expression, non unary operator '" << converter.to_bytes((*itr)->data(0)) << "' without preceding value";
			break;
		case ParseErrors::InvalidExpressionEmpty:
			std::cout << "- Invalid Expression, empty expression after assignment";
			break;
		case ParseErrors::InvalidLabel:
			std::cout << "- Invalid label name '" << converter.to_bytes((*itr)->data(0)) << "' already exists";
			break;
		case ParseErrors::InvalidStartCondition:
			std::cout << "- Invalid START condition, not compatible with function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidCondition:
			std::cout << "- Invalid condition, not compatible with function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidNamespace:
			std::cout << "- Invalid namespace";
			break;
		case ParseErrors::InvalidAssignment:
			std::cout << "- Invalid assignment, expected expression after assignment";
			break;
		case ParseErrors::InvalidPreprocessor:
			std::cout << "- Invalid preprocessor '" << converter.to_bytes((*itr)->data(0)) << "', expected keyword";
			break;
		case ParseErrors::InvalidDefine:
			std::cout << "- Invalid define '" << converter.to_bytes((*itr)->data(0)) << "', expected keyword";
			break;
		case ParseErrors::InvalidKeyword:
			std::cout << "- Invalid keyword '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidComma:
			std::cout << "- Invalid symbol to seperate arguments '" << converter.to_bytes((*itr)->data(0)) << "' expected comma";
			break;
		case ParseErrors::MissingSemiColon:
			std::cout << "- Missing semi colon before '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::MissingSemiColonEnd:
			std::cout << "- Missing semi colon at end '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::MissingEndArray:
			std::cout << "- Missing end of array ']'";
			break;
		case ParseErrors::MissingStartArray:
			std::cout << "- Missing matching start array '['";
			break;
		case ParseErrors::MissingQuote:
			std::cout << "- Missing end of string";
			break;
		case ParseErrors::MissingBracket:
			std::cout << "- Missing end of bracket ')'";
			break;
		case ParseErrors::MissingStartBracket:
			std::cout << "- Missing matching open bracket '('";
			break;
		case ParseErrors::MissingStartBrace:
			std::cout << "- Missing start of block '{'";
			break;
		case ParseErrors::MissingEndBrace:
			std::cout << "- Missing end of block '}'";
			break;
		case ParseErrors::MissingIf:
			std::cout << "- Missing 'if' to match the else";
			break;
		case ParseErrors::MissingArraySubscript:
			std::cout << "- Missing array subscript value";
			break;
		case ParseErrors::MissingArrayVariable:
			std::cout << "- Missing assignment variable for array";
			break;
		case ParseErrors::MissingArrayAssignment:
			std::cout << "- Missing array assignment value";
			break;
		case ParseErrors::MissingFunctionArgument:
			std::cout << "- Missing argument value after comma ','";
			break;
		case ParseErrors::MissingAssignment:
			std::cout << "- Missing assignment variable";
			break;
		case ParseErrors::MissingWhile:
			std::cout << "- Function '" << converter.to_bytes((*itr)->data(0)) << "' required to be in a while loop";
			break;
		case ParseErrors::MissingLabel:
			std::cout << "- Label '" << converter.to_bytes((*itr)->data(0)) << "' has not been defined";
			break;
		case ParseErrors::MissingPreprocessor:
			std::cout << "- Preprocessor has not been defined";
			break;
		case ParseErrors::MissingDefine:
			std::cout << "- Missing define keyword";
			break;
		case ParseErrors::InvalidArgumentCount:
			std::cout << "- Invalid argument count (Requires=" << converter.to_bytes((*itr)->data(1)) << ", Found=" << converter.to_bytes((*itr)->data(0)) << ")";
			break;
		case ParseErrors::UnknownProperty:
			std::cout << "- Unknown property name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UnknownFunction:
			std::cout << "- Unknown function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UnknownConstant:
			std::cout << "- Unknown Constant '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UnknownObjectFunction:
			std::cout << "- Unknown object function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UnknownPreprocessor:
			std::cout << "- Unknown preprocessor '#" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::AmbiguousObjectFunction:
			std::cout << "- Ambiguous object function '" << converter.to_bytes((*itr)->data(0)) << "', unable to determine matching function: " << converter.to_bytes((*itr)->data(1));
			break;
		case ParseErrors::IncompleteLine:
			std::cout << "- Incomplete line, expression or statement is not complete";
			break;
		case ParseErrors::InvalidFunctionDefinition:
			std::cout << "- Invalid function definition";
			break;
		case ParseErrors::CodeOutsideFunction:
			std::cout << "- Code found outside a function definition block";
			break;
		case ParseErrors::NestedFunctionDefinition:
			std::cout << "- Nested function definition, 'function' keyword found inside an existing function";
			break;
		case ParseErrors::MissingFunctionBodyBrace:
			std::cout << "- Missing opening '{' for function body";
			break;
		case ParseErrors::MissingFunctionName:
			std::cout << "- Missing function name after 'function' keyword";
			break;
		case ParseErrors::MissingFunctionParameterList:
			std::cout << "- Missing parameter list '(...)' after function name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidFunctionParameterType:
			std::cout << "- Invalid parameter type '" << converter.to_bytes((*itr)->data(0)) << "' in function definition";
			break;
		case ParseErrors::MissingFunctionParameterVariable:
			std::cout << "- Missing parameter variable after type '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::DuplicateFunctionParameterName:
			std::cout << "- Duplicate parameter name '" << converter.to_bytes((*itr)->data(0)) << "' in function definition";
			break;
		case ParseErrors::EndsubWithoutLabel:
			std::cout << "- 'endsub' used before any label or sub has been defined";
			break;
		case ParseErrors::DuplicateLabel:
			std::cout << "- Duplicate label or sub name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::MissingSubName:
			std::cout << "- Missing sub name after 'sub' keyword";
			break;
		case ParseErrors::MissingSubParameterList:
			std::cout << "- Missing '()' after sub name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::SubParametersNotAllowed:
			std::cout << "- Sub '" << converter.to_bytes((*itr)->data(0)) << "' cannot take parameters";
			break;
		case ParseErrors::NestedSubDefinition:
			std::cout << "- Nested sub definition, 'sub' keyword found inside an existing sub block";
			break;
		case ParseErrors::MissingSubBodyBrace:
			std::cout << "- Missing opening '{' for sub body '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UserFunctionCallNotStandalone:
			std::cout << "- User function '" << converter.to_bytes((*itr)->data(0)) << "' must be called as a standalone statement, not inside an expression";
			break;
		case ParseErrors::UserFunctionArgumentCountMismatch:
			std::cout << "- User function '" << converter.to_bytes((*itr)->data(0)) << "' argument count mismatch (expected " << converter.to_bytes((*itr)->data(1)) << ", got " << converter.to_bytes((*itr)->data(2)) << ")";
			break;
		case ParseErrors::ReturnValueNotAllowed:
			std::cout << "- 'return' with a value is not allowed here";
			break;
		case ParseErrors::DuplicateUserFunctionName:
			std::cout << "- Duplicate user function name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::UserFunctionNameConflict:
			std::cout << "- User function name '" << converter.to_bytes((*itr)->data(0)) << "' conflicts with an existing script command or constant";
			break;
		case ParseErrors::MissingObjectFunction:
			std::cout << "- Missing object function name";
			break;
		case ParseErrors::MissingSpecialArgument:
			std::cout << "- Missing special argument for function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidObject:
			std::cout << "- Invalid object '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidObjectDataType:
			std::cout << "- Invalid object datatype for function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::InvalidSpecialArgument:
			std::cout << "- Invalid special argument for function '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::DataAfterFunction:
			std::cout << "- Unexpected data after function call";
			break;
		case ParseErrors::InvalidString:
			std::cout << "- Invalid string";
			break;
		case ParseErrors::InvalidDoubleCondition:
			std::cout << "- Invalid double condition";
			break;
		}
		std::cout << std::endl;
		std::cout << "\t" + sLine << std::endl;
		std::cout << "\t";
		for (int i = 0; i < colPos; i++)
			std::cout << " ";
		std::cout << "^" << std::endl;

	}
	return 1;
}

bool compileScriptFile(const std::wstring& filename, const std::wstring& out)
{
	if (!g_scriptData)
		throw std::exception("Unable to Compile script, No game data loaded");

	// load the script parser
	XScript::CScriptParser parser(g_scriptData);

	// read the script file using the parser
	parser.addCurrentFile(filename);
	std::wifstream infile(filename);
	std::wstring line;
	int iLine = 0;
	while (std::getline(infile, line))
	{
		++iLine;
		if (!parser.parseLine(iLine, line))
			break;

	}
	parser.removeCurrentFile();
	infile.close();

	if (!parser.errorData().empty())
	{
		displayError(parser, line);
		return false;
	}

	if (!parser.finalise())
	{
		displayError(parser, line);
		return false;
	}

	if (parser.hasWarnings())
		displayWarnings(parser);

	XScript::CScript* script = parser.currentScript();
	if (script->save(out, g_scriptData->functionData()))
	{
		// If the output file has a .pck extension, compress it in-place
		VFSHelper_CompressPck(out.c_str());
		return true;
	}

	return false;
}

bool compileScriptFile(const std::string& filename, const std::string& out)
{
	return compileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(out));
}

bool compileScriptFile(const std::wstring& filename, const std::wstring& out, const std::vector<std::wstring>& defines)
{
	if (!g_scriptData)
		throw std::exception("Unable to Compile script, No game data loaded");

	XScript::CScriptParser parser(g_scriptData);
	for (const auto& d : defines)
		parser.addDefine(d);

	parser.addCurrentFile(filename);
	std::wifstream infile(filename);
	std::wstring line;
	int iLine = 0;
	while (std::getline(infile, line))
	{
		++iLine;
		if (!parser.parseLine(iLine, line))
			break;
	}
	parser.removeCurrentFile();
	infile.close();

	if (!parser.errorData().empty())
	{
		displayError(parser, line);
		return false;
	}

	if (!parser.finalise())
	{
		displayError(parser, line);
		return false;
	}

	if (parser.hasWarnings())
		displayWarnings(parser);

	XScript::CScript* script = parser.currentScript();
	return script->save(out, g_scriptData->functionData());
}

bool compileScriptFile(const std::string& filename, const std::string& out, const std::vector<std::string>& defines)
{
	std::vector<std::wstring> wdefines;
	for (const auto& d : defines)
		wdefines.push_back(XScript::Utils::s2ws(d));
	return compileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(out), wdefines);
}


bool decompileScriptFile(const std::wstring& filename, const std::wstring& output)
{
	if (!g_scriptData)
		throw std::exception("Unable to Compile script, No game data loaded");

	XScript::ScriptRead reader(g_scriptData);

	bool success = reader.read(filename);
	if (success)
		success = reader.write(output);
	return success;
}

bool decompileScriptFile(const std::string& filename, const std::string& output)
{
	return decompileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(output));
}

bool decompileScriptFile(const std::wstring& filename, const std::wstring& output, bool useNamespace)
{
	if (!g_scriptData)
		throw std::exception("Unable to decompile script, No game data loaded");

	XScript::ScriptRead reader(g_scriptData);
	reader.setUseNamespace(useNamespace);

	bool success = reader.read(filename);
	if (success)
		success = reader.write(output);
	return success;
}

bool decompileScriptFile(const std::string& filename, const std::string& output, bool useNamespace)
{
	return decompileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(output), useNamespace);
}
bool loadData(const std::wstring& dataFile)
{
	if (g_scriptData)
		delete g_scriptData;
	g_scriptData = new XScript::CScriptData();
	return g_scriptData->loadData(dataFile);
}
bool loadData(const std::string& dataFile)
{
	return loadData(XScript::Utils::s2ws(dataFile));
}

bool loadXmlData(const std::wstring& filename, const std::wstring& output)
{
	if (g_scriptData)
		delete g_scriptData;
	g_scriptData = new XScript::CScriptData();
	bool success = g_scriptData->readXMLData(filename);
	if (success)
		success = g_scriptData->saveData(output);
	return success;
}

bool loadXmlData(const std::string& filename, const std::string& output)
{
	return loadXmlData(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(output));
}

// ── Script file scanner ───────────────────────────────────────────────────────
// Parses .xs (v0.8) and .xml (legacy) script files in workingDir to extract
// argument types and return types from function main() headers, registers them
// in g_scriptData, then re-saves the .dat so the VS Code extension can use them.

// Parse "function [ReturnType[|ReturnType2]] main(ParDef $var, ...)" from an
// .xs source file. Returns false if no function main() header is found.
static bool _parseXsHeader(const std::wstring& path, CScriptData::ScriptDef& def)
{
	std::wifstream f(path);
	if (!f.is_open()) return false;
	f.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));

	std::wstring line;
	while (std::getline(f, line))
	{
		// Strip leading whitespace
		size_t start = line.find_first_not_of(L" \t\r\n");
		if (start == std::wstring::npos) continue;
		line = line.substr(start);

		// Look for "function" keyword
		if (line.substr(0, 8) != L"function") continue;
		line = line.substr(8);

		// Clear return types for this function — don't carry over from previous functions
		def.returnTypes.clear();

		// Skip whitespace
		start = line.find_first_not_of(L" \t");
		if (start == std::wstring::npos) continue;
		line = line.substr(start);

		// Skip optional return type(s) — keep consuming tokens until we find
		// one that is immediately followed by '(' (i.e. the function name).
		// If that name isn't "main", skip this whole line.
		std::wstring funcName;
		while (!line.empty())
		{
			size_t end = line.find_first_of(L" \t(");
			if (end == std::wstring::npos) { funcName = line; line.clear(); break; }
			std::wstring token = line.substr(0, end);
			// Skip whitespace after token to find what follows
			size_t next = line.find_first_not_of(L" \t", end);
			if (next == std::wstring::npos) break;
			if (line[next] == L'(')
			{
				// token is the function name
				funcName = token;
				line = line.substr(next + 1); // skip past '('
				break;
			}
			// token is a return type — try to parse it
			const ConstantData* dtConst = g_scriptData->findConstant(token);
			if (dtConst && dtConst->type() == DataTypes::DataType)
				def.returnTypes.insert(static_cast<DataTypes>(dtConst->id()));
			// Advance past '|' separators and whitespace
			line = line.substr(end);
			start = line.find_first_not_of(L" \t|");
			if (start == std::wstring::npos) { line.clear(); break; }
			line = line.substr(start);
		}

		// Only process "main"
		if (funcName != L"main") continue;

		// If no return type was declared, default to Unknown (type not specified)
		if (def.returnTypes.empty())
			def.returnTypes.insert(DataTypes::Unknown);

		// line is already positioned just after the opening '('
		// Find closing ')'
		size_t parenEnd = line.find(L')');
		if (parenEnd == std::wstring::npos) continue;
		std::wstring params = line.substr(0, parenEnd);

		// Parse comma-separated parameters: "ParDef $name" or just "$name"
		std::wstringstream ss(params);
		std::wstring token;
		while (std::getline(ss, token, L','))
		{
			// Trim
			size_t s = token.find_first_not_of(L" \t");
			size_t e = token.find_last_not_of(L" \t\r\n");
			if (s == std::wstring::npos) continue;
			token = token.substr(s, e - s + 1);
			if (token.empty()) continue;

			CScriptData::ScriptArgDef arg;
			arg.pardef = ParDef::Unknown;

			// Check if first character is '$' — no pardef type, default to VALUE (id=9)
			if (token[0] == L'$')
			{
				arg.pardef = static_cast<ParDef>(9); // VALUE
				arg.name = token;
			}
			else
			{
				// Split into "ParDefType $name"
				size_t sp = token.find(L' ');
				if (sp == std::wstring::npos) continue;
				std::wstring pardefCode = token.substr(0, sp);
				std::wstring varName = token.substr(sp + 1);
				varName.erase(0, varName.find_first_not_of(L" \t"));

				const ParDefData* pd = g_scriptData->findParDefData(pardefCode);
				if (pd)
				{
					arg.pardef = pd->id;
					arg.name = varName;
				}
				else
					continue; // unknown pardef — skip this arg
			}

			def.args.push_back(arg);
		}

		return true; // successfully parsed main header
	}
	return false;
}

// Parse SetArgument entries from the top of a compiled .xml script.
// Format: each argument is an sval array of [int pardefId, string desc].
static bool _parseXmlArguments(const std::wstring& path, CScriptData::ScriptDef& def)
{
	// Read file into buffer for rapidxml
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	if (!f.is_open()) return false;

	std::streamsize size = f.tellg();
	f.seekg(0, std::ios::beg);
	if (size <= 0) return false;

	std::vector<char> buf(static_cast<size_t>(size) + 1, 0);
	f.read(buf.data(), size);
	f.close();

	std::string narrow(buf.data(), static_cast<size_t>(size));
	std::wstring wide(narrow.begin(), narrow.end());
	std::vector<wchar_t> wbuf(wide.begin(), wide.end());
	wbuf.push_back(0);

	// Helper to get attribute value
	auto attr = [](rapidxml::xml_node<wchar_t>* node, const wchar_t* name) -> std::wstring
		{
			if (!node) return L"";
			auto* a = node->first_attribute(name);
			return a ? std::wstring(a->value()) : L"";
		};

	// Helper: is this sval node an array?
	auto isArray = [&](rapidxml::xml_node<wchar_t>* node) -> bool
		{
			return node && attr(node, L"type") == L"array";
		};

	try
	{
		rapidxml::xml_document<wchar_t> doc;
		doc.parse<0>(wbuf.data());

		// X3 XML format: <script><codearray><sval type="array" ...> is the root array
		// Structure of root array children (in order):
		//   [0] string  — script name
		//   [1] int     — engine version
		//   [2] string  — description
		//   [3] int     — return type ID
		//   [4] int     — unknown
		//   [5] array   — variable names (string per entry)
		//   [6] array   — commands (we skip this)
		//   [7] array   — arguments: each child is [int pardefId, string desc]
		//   [8] array   — return lines (we skip this)

		rapidxml::xml_node<wchar_t>* scriptNode = doc.first_node(L"script");
		if (!scriptNode) return false;
		rapidxml::xml_node<wchar_t>* codearray = scriptNode->first_node(L"codearray");
		if (!codearray) return false;

		// Root element is the first sval of type array
		rapidxml::xml_node<wchar_t>* root = codearray->first_node(L"sval");
		if (!root || attr(root, L"type") != L"array") return false;

		// Collect top-level children
		std::vector<rapidxml::xml_node<wchar_t>*> children;
		for (auto* c = root->first_node(L"sval"); c; c = c->next_sibling(L"sval"))
			children.push_back(c);

		// Need at least 8 children to have the arguments array at index 7
		if (children.size() < 8) return false;

		// [5] Variable names array
		std::vector<std::wstring> varNames;
		if (isArray(children[5]))
		{
			for (auto* v = children[5]->first_node(L"sval"); v; v = v->next_sibling(L"sval"))
				varNames.push_back(attr(v, L"val"));
		}

		// [7] Arguments array — each child is [int pardefId, string desc]
		if (!isArray(children[7])) return false;

		int argIndex = 0;
		for (auto* argNode = children[7]->first_node(L"sval"); argNode; argNode = argNode->next_sibling(L"sval"), argIndex++)
		{
			if (attr(argNode, L"type") != L"array") continue;

			// First child: pardef id
			auto* pdNode = argNode->first_node(L"sval");
			if (!pdNode || attr(pdNode, L"type") != L"int") continue;
			int pardefId = std::stoi(attr(pdNode, L"val"));

			// Second child: description string
			std::wstring desc;
			auto* descNode = pdNode->next_sibling(L"sval");
			if (descNode && attr(descNode, L"type") == L"string")
				desc = attr(descNode, L"val");

			const ParDefData* pd = g_scriptData->getParDefData(static_cast<ParDef>(pardefId));
			if (!pd) continue;

			CScriptData::ScriptArgDef arg;
			arg.pardef = pd->id;
			arg.desc = desc;
			// Match variable name by index from variables array
			if (argIndex < static_cast<int>(varNames.size()))
				arg.name = varNames[argIndex];
			def.args.push_back(arg);
		}

		if (def.returnTypes.empty())
			def.returnTypes.insert(DataTypes::Unknown);
	}
	catch (...) { return false; }

	return true;
}

bool scanScriptFiles(const std::wstring& workingDir, const std::wstring& output)
{
	if (!g_scriptData)
		return false;

	std::wstring dir = workingDir.empty() ? L"." : workingDir;
	// Ensure no trailing backslash issues
	if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/')
		dir += L'\\';

	unsigned int found = 0;

	// ── Step 1: scan .xs files ────────────────────────────────────────────────
	std::set<std::wstring> xsNames; // base names of .xs files found (for dedup)
	{
		std::wstring pattern = dir + L"*.xs";
		_wfinddata_t fd;
		intptr_t h = _wfindfirst(pattern.c_str(), &fd);
		if (h != -1)
		{
			do
			{
				std::wstring fname(fd.name);
				// Strip .xs extension to get the base script name
				std::wstring baseName = fname.substr(0, fname.size() - 3);
				xsNames.insert(baseName);

				CScriptData::ScriptDef def;
				def.name = baseName;

				if (_parseXsHeader(dir + fname, def))
				{
					g_scriptData->addScript(def);
					found++;
				}
			} while (_wfindnext(h, &fd) == 0);
			_findclose(h);
		}
	}

	// ── Step 2: scan .xml files where no .xs exists ───────────────────────────
	{
		std::wstring pattern = dir + L"*.xml";
		_wfinddata_t fd;
		intptr_t h = _wfindfirst(pattern.c_str(), &fd);
		if (h != -1)
		{
			do
			{
				std::wstring fname(fd.name);
				// Strip .xml extension
				std::wstring baseName = fname.substr(0, fname.size() - 4);
				// Skip if we already have a .xs version
				if (xsNames.find(baseName) != xsNames.end()) continue;

				CScriptData::ScriptDef def;
				def.name = baseName;
				// .xml-only: no return type available

				if (_parseXmlArguments(dir + fname, def) && !def.args.empty())
				{
					g_scriptData->addScript(def);
					found++;
				}
			} while (_wfindnext(h, &fd) == 0);
			_findclose(h);
		}
	}

	std::cout << "\tRegistered " << found << " script definition(s)" << std::endl;

	// Re-save the .dat with the new SCRIPTS section — skipped when output is
	// empty, which signals memory-only mode (e.g. during compile).
	if (output.empty())
		return true;
	return g_scriptData->saveData(output);
}

bool exportUDL(const std::wstring& udlFile, const std::wstring& autoFile)
{
	if (!g_scriptData)
		throw std::exception("Unable to export UDL file, No game data loaded");

	XScript::XScriptUDL exporter(g_scriptData);
	if (exporter.writeUDL(udlFile))
	{
		if (exporter.writeAutoComplete(autoFile))
			return true;
	}

	return false;
}
bool exportUDL(const std::string& udlFile, const std::string& autoFile)
{
	return exportUDL(XScript::Utils::s2ws(udlFile), XScript::Utils::s2ws(autoFile));
}