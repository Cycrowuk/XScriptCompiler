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

#include <locale>
#include <codecvt>
#include <sstream>
#include <iomanip>

#include "../XLib/XLib.h"

using namespace XScript;

CScriptData* g_scriptData;

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
			std::cout << "- Invalid label name '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::DuplicateLabel:
			std::cout << "- Duplicate label or sub name '" << converter.to_bytes((*itr)->data(0)) << "' - each label and sub must have a unique name";
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
		case ParseErrors::InvalidFunctionDefinition:
			std::cout << "- Invalid function definition";
			if (!(*itr)->data(0).empty())
				std::cout << " '" << converter.to_bytes((*itr)->data(0)) << "'";
			break;
		case ParseErrors::CodeOutsideFunction:
			std::cout << "- Code is not allowed outside of a function definition";
			break;
		case ParseErrors::NestedFunctionDefinition:
			std::cout << "- Nested function definitions are not supported - close the current function with '}' before starting a new one";
			break;
		case ParseErrors::MissingFunctionBodyBrace:
			std::cout << "- Expected '{' to start the function body";
			break;
		case ParseErrors::MissingFunctionName:
			std::cout << "- Expected a function name after 'function'";
			break;
		case ParseErrors::MissingFunctionParameterList:
			std::cout << "- Expected '(' to start the function's parameter list";
			break;
		case ParseErrors::InvalidFunctionParameterType:
			std::cout << "- Invalid parameter type";
			if (!(*itr)->data(0).empty())
				std::cout << " '" << converter.to_bytes((*itr)->data(0)) << "'";
			std::cout << " - expected a recognised datatype or pardef name";
			break;
		case ParseErrors::MissingFunctionParameterVariable:
			std::cout << "- Expected a $variable name after the parameter type";
			break;
		case ParseErrors::DuplicateFunctionParameterName:
			std::cout << "- Duplicate parameter name '" << converter.to_bytes((*itr)->data(0)) << "' - each parameter must have a unique name";
			break;
		case ParseErrors::EndsubWithoutLabel:
			std::cout << "- 'endsub' cannot be used before any label/sub has been defined";
			break;
		case ParseErrors::MissingSubName:
			std::cout << "- Expected a sub name after 'sub'";
			break;
		case ParseErrors::MissingSubParameterList:
			std::cout << "- Expected '()' after the sub name";
			break;
		case ParseErrors::SubParametersNotAllowed:
			std::cout << "- Subs cannot take parameters - use empty parentheses, e.g. 'sub name()'";
			break;
		case ParseErrors::NestedSubDefinition:
			std::cout << "- Nested sub definitions are not supported - close the current sub with '}' before starting a new one";
			break;
		case ParseErrors::MissingSubBodyBrace:
			std::cout << "- Expected '{' to start the sub body";
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

	// ── Pass 1 (pre-pass) ───────────────────────────────────────────────────
	// Reads the file once to hoist subroutine/label variable types and other
	// forward-reference information before the real compile runs.
	parser.addCurrentFile(filename);
	{
		std::wifstream prefile(filename);
		std::wstring preline;
		int iPreLine = 0;
		while (std::getline(prefile, preline))
		{
			++iPreLine;
			if (!parser.prePassLine(iPreLine, preline))
				break;
		}
		prefile.close();
	}
	parser.removeCurrentFile();

	// Reset all per-pass state (errors, warnings, current data list, etc.)
	// while preserving what pass 1 learned (e.g. _subVariables).
	parser.resetForRealPass();

	// ── Pass 2 (real compile) ───────────────────────────────────────────────
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
		XLib::FileIO f(out);
		if (f.exists() && f.isFileExtension("pck"))
		{
			size_t size;
			unsigned char* readData = (unsigned char *)f.readAll(&size);
			if (readData)
			{
				size_t newSize;
				unsigned char* data = XLib::PCKData(readData, size, &newSize, false);
				if (data)
				{
					f.close();
					XLib::FileIO fWrite(out, true);
					fWrite.write((const char*)data, newSize);
					fWrite.close();
					delete[]data;
				}
				delete[]readData;
			}
		}
		return true;
	}

	return false;
}

bool compileScriptFile(const std::wstring& filename, const std::wstring& out, const std::vector<std::wstring>& defines)
{
	if (!g_scriptData)
		throw std::exception("Unable to Compile script, No game data loaded");

	XScript::CScriptParser parser(g_scriptData);

	// Pre-register command-line defines — these persist across resetForRealPass()
	// so they remain active for both the pre-pass and the real compile pass.
	for (const auto& d : defines)
		parser.addDefine(d);

	// ── Pass 1 (pre-pass) ───────────────────────────────────────────────────
	parser.addCurrentFile(filename);
	{
		std::wifstream prefile(filename);
		std::wstring preline;
		int iPreLine = 0;
		while (std::getline(prefile, preline))
		{
			++iPreLine;
			if (!parser.prePassLine(iPreLine, preline))
				break;
		}
		prefile.close();
	}
	parser.removeCurrentFile();

	parser.resetForRealPass();

	// ── Pass 2 (real compile) ───────────────────────────────────────────────
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
		XLib::FileIO f(out);
		if (f.exists() && f.isFileExtension("pck"))
		{
			size_t size;
			unsigned char* readData = (unsigned char*)f.readAll(&size);
			if (readData)
			{
				size_t newSize;
				unsigned char* data = XLib::PCKData(readData, size, &newSize, false);
				if (data)
				{
					f.close();
					XLib::FileIO fWrite(out, true);
					fWrite.write((const char*)data, newSize);
					fWrite.close();
					delete[] data;
				}
				delete[] readData;
			}
		}
		return true;
	}

	return false;
}

bool compileScriptFile(const std::string& filename, const std::string& out)
{
	return compileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(out));
}

bool compileScriptFile(const std::string& filename, const std::string& out, const std::vector<std::string>& defines)
{
	std::vector<std::wstring> wdefines;
	for (const auto& d : defines)
		wdefines.push_back(XScript::Utils::s2ws(d));
	return compileScriptFile(XScript::Utils::s2ws(filename), XScript::Utils::s2ws(out), wdefines);
}


bool decompileScriptFile(const std::wstring& filename, const std::wstring& output, bool useNamespace)
{
	if (!g_scriptData)
		throw std::exception("Unable to Decompile script, No game data loaded");

	XScript::ScriptRead reader(g_scriptData);
	reader.setUseNamespace(useNamespace);

	bool success = reader.read(filename);
	if (success)
		success = reader.write(output);
	return success;
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
